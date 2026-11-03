// Testes do motor de áudio. Compilar com ASan e UBSan: veja run_tests.sh.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "../../app/src/main/cpp/core/engine.h"
#include "../../app/src/main/cpp/core/resampler.h"
#include "../../app/src/main/cpp/core/spsc_queue.h"
#include "../../app/src/main/cpp/core/track.h"

using namespace djf;

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { ++g_checks; if (!(c)) { ++g_fail; std::printf("  FALHOU %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_NEAR(a, b, tol) do { ++g_checks; double _a=(a), _b=(b); if (!(std::fabs(_a-_b) <= (tol))) { ++g_fail; std::printf("  FALHOU %s:%d  %s=%g esperado %g (tol %g)\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); } } while (0)

static const double kPi = 3.14159265358979323846;
static const int kSr = 48000;

static std::shared_ptr<Track> makeConst(int64_t frames, float v) {
  auto t = std::make_shared<Track>();
  t->sampleRate = kSr; t->frames = frames;
  t->pcm.assign(static_cast<size_t>(frames) * 2, static_cast<int16_t>(v * 32767.f));
  return t;
}
static std::shared_ptr<Track> makeSine(int64_t frames, double hz, float amp) {
  auto t = std::make_shared<Track>();
  t->sampleRate = kSr; t->frames = frames;
  t->pcm.resize(static_cast<size_t>(frames) * 2);
  for (int64_t i = 0; i < frames; ++i) {
    int16_t s = static_cast<int16_t>(amp * 32767.0 * std::sin(2 * kPi * hz * i / kSr));
    t->pcm[2 * i] = s; t->pcm[2 * i + 1] = s;
  }
  return t;
}
static std::vector<float> renderN(Engine& e, int frames, int chunk = 512) {
  std::vector<float> out(static_cast<size_t>(frames) * 2);
  for (int done = 0; done < frames; done += chunk) {
    int n = std::min(chunk, frames - done);
    e.render(&out[static_cast<size_t>(done) * 2], n);
  }
  return out;
}
static void settle(Engine& e) { renderN(e, 12000); }  // 250 ms: suavizações convergem

// ------------------------------------------------------------------ fila
static void testQueue() {
  std::printf("fila sem trava\n");
  SpscQueue<int, 8> q;
  int v = 0;
  CHECK(!q.pop(v));
  for (int i = 0; i < 7; ++i) CHECK(q.push(i));
  CHECK(!q.push(99));  // capacidade útil = 7
  for (int i = 0; i < 7; ++i) { CHECK(q.pop(v)); CHECK(v == i); }
  CHECK(!q.pop(v));

  SpscQueue<int, 256> big;
  const int N = 300000;
  std::thread prod([&] { for (int i = 0; i < N;) if (big.push(i)) ++i; });
  int expected = 0; bool ordered = true;
  while (expected < N) { int x; if (big.pop(x)) { if (x != expected) ordered = false; ++expected; } }
  prod.join();
  CHECK(ordered);
}

// ----------------------------------------------------------- reamostragem
static double goertzelAmp(const std::vector<float>& x, int start, int len, double freq, double sr, int ch) {
  double re = 0, im = 0;
  for (int i = 0; i < len; ++i) {
    double w = 0.5 - 0.5 * std::cos(2 * kPi * i / (len - 1));
    double v = x[static_cast<size_t>(start + i) * 2 + ch] * w;
    re += v * std::cos(2 * kPi * freq * (start + i) / sr);
    im += v * std::sin(2 * kPi * freq * (start + i) / sr);
  }
  return 2.0 * std::sqrt(re * re + im * im) / (0.5 * len);
}

static void testResampler() {
  std::printf("reamostragem\n");
  {  // 44,1 kHz -> 48 kHz, seno de 1 kHz na esquerda e 2 kHz na direita
    const int inN = 44100;
    std::vector<float> in(static_cast<size_t>(inN) * 2);
    for (int i = 0; i < inN; ++i) {
      in[2 * i] = static_cast<float>(0.5 * std::sin(2 * kPi * 1000.0 * i / 44100.0));
      in[2 * i + 1] = static_cast<float>(0.5 * std::sin(2 * kPi * 2000.0 * i / 44100.0));
    }
    StreamResampler rs(44100, 48000);
    std::vector<float> out;
    rs.process(in.data(), inN, out);
    rs.flush(out);
    const int outN = static_cast<int>(out.size() / 2);
    CHECK(std::abs(outN - 48000) <= 2);
    double maxErr = 0;
    for (int n = 200; n < outN - 200; ++n) {
      maxErr = std::max(maxErr, std::fabs(out[2 * n] - 0.5 * std::sin(2 * kPi * 1000.0 * n / 48000.0)));
      maxErr = std::max(maxErr, std::fabs(out[2 * n + 1] - 0.5 * std::sin(2 * kPi * 2000.0 * n / 48000.0)));
    }
    std::printf("  erro máximo 44,1->48 kHz: %.5f\n", maxErr);
    CHECK(maxErr < 0.004);
  }
  {  // 96 kHz -> 48 kHz: 5 kHz passa, 40 kHz (que viraria 8 kHz) é cortado
    const int inN = 96000;
    std::vector<float> in(static_cast<size_t>(inN) * 2);
    for (int i = 0; i < inN; ++i) {
      float v = static_cast<float>(0.4 * std::sin(2 * kPi * 5000.0 * i / 96000.0) +
                                   0.4 * std::sin(2 * kPi * 40000.0 * i / 96000.0));
      in[2 * i] = v; in[2 * i + 1] = v;
    }
    StreamResampler rs(96000, 48000);
    std::vector<float> out;
    rs.process(in.data(), inN, out);
    rs.flush(out);
    const int outN = static_cast<int>(out.size() / 2);
    CHECK(std::abs(outN - 48000) <= 2);
    double a5 = goertzelAmp(out, 2000, 40000, 5000.0, 48000.0, 0);
    double a8 = goertzelAmp(out, 2000, 40000, 8000.0, 48000.0, 0);
    std::printf("  5 kHz: %.4f (esperado 0.4)   alias em 8 kHz: %.5f\n", a5, a8);
    CHECK_NEAR(a5, 0.4, 0.01);
    CHECK(a8 < 0.01);  // pelo menos 32 dB abaixo
  }
  {  // sinal contínuo (DC) mantém o nível
    std::vector<float> in(2 * 10000, 0.3f);
    StreamResampler rs(44100, 48000);
    std::vector<float> out;
    rs.process(in.data(), 10000, out);
    rs.flush(out);
    CHECK_NEAR(out[out.size() / 2], 0.3, 1e-4);
  }
  {  // taxa igual: cópia exata
    std::vector<float> in = {0.1f, -0.2f, 0.3f, -0.4f};
    StreamResampler rs(48000, 48000);
    std::vector<float> out;
    rs.process(in.data(), 2, out);
    rs.flush(out);
    CHECK(out == in);
  }
}

// ---------------------------------------------------------------- faixa
static void testBuilder() {
  std::printf("construtor de faixa\n");
  const int inN = 44100;
  std::vector<int16_t> mono(inN);
  for (int i = 0; i < inN; ++i) mono[i] = static_cast<int16_t>(20000 * std::sin(2 * kPi * 100.0 * i / 44100.0));

  TrackBuilder one(44100, kSr, 48000 * 600);
  one.appendPcm16(mono.data(), inN, 1);
  auto a = one.finish();
  CHECK(std::abs(a->frames - 48000) <= 2);
  CHECK(a->pcm.size() == static_cast<size_t>(a->frames) * 2);
  CHECK(a->pcm[2000] == a->pcm[2001]);  // mono duplicado nos dois lados
  CHECK(std::abs(a->peakBins() - 150) <= 2);

  // o resultado não pode depender de como os pedaços chegam
  TrackBuilder chunked(44100, kSr, 48000 * 600);
  std::mt19937 rng(1);
  for (int pos = 0; pos < inN;) {
    int n = std::min<int>(1 + static_cast<int>(rng() % 3000), inN - pos);
    chunked.appendPcm16(mono.data() + pos, n, 1);
    pos += n;
  }
  auto b = chunked.finish();
  CHECK(a->frames == b->frames);
  CHECK(a->pcm == b->pcm);
  CHECK(a->peaks == b->peaks);

  // bandas: 100 Hz cai nos graves, 8 kHz nos agudos
  auto dominant = [](const Track& t) {
    long s[3] = {0, 0, 0};
    for (size_t i = 0; i < t.peaks.size(); ++i) s[i % 3] += t.peaks[i];
    return s[0] > s[1] && s[0] > s[2] ? 0 : (s[2] > s[1] ? 2 : 1);
  };
  CHECK(dominant(*a) == 0);
  std::vector<int16_t> hi(inN);
  for (int i = 0; i < inN; ++i) hi[i] = static_cast<int16_t>(20000 * std::sin(2 * kPi * 8000.0 * i / 44100.0));
  TrackBuilder hb(44100, kSr, 48000 * 600);
  hb.appendPcm16(hi.data(), inN, 1);
  CHECK(dominant(*hb.finish()) == 2);

  // limite de tamanho
  TrackBuilder lim(44100, kSr, 1000);
  lim.appendPcm16(mono.data(), inN, 1);
  auto c = lim.finish();
  CHECK(lim.tooLong());
  CHECK(c->frames == 1000);

  // estéreo e canais extras: usa os dois primeiros
  std::vector<int16_t> st(300 * 3);
  for (int i = 0; i < 300; ++i) { st[3 * i] = 1000; st[3 * i + 1] = -2000; st[3 * i + 2] = 5000; }
  TrackBuilder sb(48000, kSr, 100000);
  sb.appendPcm16(st.data(), 300, 3);
  auto d = sb.finish();
  CHECK(d->pcm[200] == 1000 && d->pcm[201] == -2000);
}

// ---------------------------------------------------------------- motor
static void testPlayback() {
  std::printf("motor: reprodução, pausa e fim\n");
  Engine e(kSr, 256);
  auto t = makeConst(kSr * 2, 0.5f);
  CHECK(e.loadTrack(0, t));
  e.setCrossfader(0.f); e.setMaster(1.f);
  settle(e);
  auto info = e.deckInfo(0);
  CHECK(info.loaded && !info.playing && info.lengthFrames == kSr * 2);

  e.play(0);
  auto out = renderN(e, 1000);
  CHECK(e.deckInfo(0).playing);
  CHECK(e.deckInfo(0).positionFrames == 1000);
  CHECK_NEAR(out[0], 0.0, 0.01);                        // começa de zero (rampa)
  bool mono = true; for (int i = 1; i < 190; ++i) if (out[2 * i] < out[2 * (i - 1)] - 1e-6f) mono = false;
  CHECK(mono);                                          // rampa só sobe
  CHECK_NEAR(out[2 * 500], 0.5, 1e-3);
  CHECK_NEAR(out[2 * 500 + 1], 0.5, 1e-3);
  CHECK(e.deckInfo(0).peak > 0.45f);

  e.pause(0);
  renderN(e, 1000);
  int64_t p = e.deckInfo(0).positionFrames;
  CHECK(p >= 1000 && p < 1000 + 260);                   // andou só durante a rampa de saída
  auto after = renderN(e, 1000);
  CHECK(e.deckInfo(0).positionFrames == p);
  CHECK_NEAR(after[1000], 0.0, 1e-6);
  CHECK(!e.deckInfo(0).playing);

  // fim da faixa
  Engine e2(kSr, 256);
  e2.loadTrack(0, makeConst(1000, 0.5f));
  e2.setCrossfader(0.f);
  settle(e2);
  e2.play(0);
  renderN(e2, 3000);
  CHECK(!e2.deckInfo(0).playing);
  CHECK(e2.deckInfo(0).positionFrames == 1000);
  e2.play(0);                                           // tocar de novo no fim recomeça
  renderN(e2, 100);
  // um quadro é gasto aplicando o salto ao início (20 microssegundos)
  CHECK(e2.deckInfo(0).positionFrames == 99);
}

static void testMixer() {
  std::printf("motor: crossfader, volume e master\n");
  auto level = [&](float x, float volA, float volB, float master) {
    Engine e(kSr, 256);
    e.loadTrack(0, makeConst(kSr * 3, 0.5f));
    e.loadTrack(1, makeConst(kSr * 3, 0.5f));
    e.setCrossfader(x); e.setVolume(0, volA); e.setVolume(1, volB); e.setMaster(master);
    settle(e);
    e.play(0); e.play(1);
    renderN(e, 1000);
    auto o = renderN(e, 1000);
    return static_cast<double>(o[1000]);
  };
  CHECK_NEAR(level(0.f, 1, 1, 1), 0.5, 2e-3);                       // só A
  CHECK_NEAR(level(1.f, 1, 1, 1), 0.5, 2e-3);                       // só B
  CHECK_NEAR(level(0.5f, 1, 1, 1), 0.5 * 2 * std::sqrt(0.5), 2e-3); // potência constante
  CHECK_NEAR(level(0.f, 0.5f, 1, 1), 0.25, 2e-3);                   // volume do deck
  CHECK_NEAR(level(0.f, 1, 1, 0.5f), 0.25, 2e-3);                   // master
  CHECK_NEAR(level(0.f, 0.f, 1, 1), 0.0, 1e-4);                     // volume zero
  CHECK_NEAR(level(0.5f, 1, 1, 1), 0.7071, 2e-3);

  // os dois decks no máximo somam acima de 1: o limite impede passar
  Engine e(kSr, 256);
  e.loadTrack(0, makeConst(kSr, 0.95f)); e.loadTrack(1, makeConst(kSr, 0.95f));
  e.setCrossfader(0.5f); e.setMaster(1.f);
  settle(e); e.play(0); e.play(1);
  auto o = renderN(e, 4000);
  float mx = 0; for (float v : o) mx = std::max(mx, std::fabs(v));
  CHECK(mx <= 1.0f);
  bool finite = true; for (float v : o) if (!std::isfinite(v)) finite = false;
  CHECK(finite);
}

static void testCueAndSeek() {
  std::printf("motor: cue e busca\n");
  Engine e(kSr, 256);
  e.loadTrack(0, makeConst(kSr * 5, 0.4f));
  e.setCrossfader(0.f);
  settle(e);
  e.play(0);
  renderN(e, 20000);
  e.pause(0);
  renderN(e, 1000);
  int64_t where = e.deckInfo(0).positionFrames;
  e.cue(0);                               // parado: grava o cue aqui
  renderN(e, 256);
  CHECK(e.deckInfo(0).cueFrames == where);
  e.play(0);
  renderN(e, 15000);
  CHECK(e.deckInfo(0).positionFrames > where + 10000);
  e.cue(0);                               // tocando: volta ao cue e para
  renderN(e, 2000);
  CHECK(!e.deckInfo(0).playing);
  CHECK(e.deckInfo(0).positionFrames == where);

  e.seekFrames(0, 100000);
  renderN(e, 512);
  CHECK(e.deckInfo(0).positionFrames == 100000);
  e.seekFrames(0, 99999999);              // além do fim: limita
  renderN(e, 512);
  CHECK(e.deckInfo(0).positionFrames == kSr * 5);
  e.seekFrames(0, -5);
  renderN(e, 512);
  CHECK(e.deckInfo(0).positionFrames == 0);

  // buscar durante a reprodução não pode estalar: seno de 100 Hz, salto entre amostras seguidas
  Engine s(kSr, 256);
  s.loadTrack(0, makeSine(kSr * 4, 100.0, 0.5f));
  s.setCrossfader(0.f); s.setMaster(1.f);
  settle(s);
  s.play(0);
  auto first = renderN(s, 5000);
  s.seekFrames(0, 77777);                 // posição com fase bem diferente
  auto second = renderN(s, 5000);
  std::vector<float> all = first; all.insert(all.end(), second.begin(), second.end());
  double maxJump = 0;
  for (size_t i = 1; i < all.size() / 2; ++i) maxJump = std::max<double>(maxJump, std::fabs(all[2 * i] - all[2 * (i - 1)]));
  std::printf("  maior salto entre amostras com busca: %.4f (seno puro: 0.0065)\n", maxJump);
  CHECK(maxJump < 0.03);
  CHECK(s.deckInfo(0).playing);
  CHECK(s.deckInfo(0).positionFrames > 77777);
}

static void testLoadAndGarbage() {
  std::printf("motor: carregar e liberar memória\n");
  Engine e(kSr, 256);
  auto t1 = makeConst(1000, 0.1f);
  std::weak_ptr<const Track> w1 = t1;
  CHECK(e.loadTrack(0, t1));
  t1.reset();
  CHECK(!w1.expired());                  // o motor ainda segura
  auto t2 = makeConst(2000, 0.2f);
  CHECK(e.loadTrack(0, t2));
  e.collectGarbage();
  CHECK(!w1.expired());                  // o áudio ainda não aplicou a troca
  renderN(e, 256);
  e.collectGarbage();
  CHECK(w1.expired());                   // agora liberou a antiga
  CHECK(e.deckInfo(0).lengthFrames == 2000);

  auto wrong = std::make_shared<Track>();
  wrong->sampleRate = 44100; wrong->frames = 10; wrong->pcm.assign(20, 0);
  CHECK(!e.loadTrack(0, wrong));         // taxa diferente da do motor
  CHECK(!e.loadTrack(5, makeConst(10, 0.f)));
  CHECK(!e.loadTrack(0, nullptr));
  e.play(7); e.setVolume(-1, 1.f);       // deck inválido: ignora sem quebrar
  renderN(e, 256);

  e.unload(0);
  renderN(e, 256);
  CHECK(!e.deckInfo(0).loaded);
  e.play(0);                             // sem faixa: nada acontece
  auto o = renderN(e, 256);
  CHECK(!e.deckInfo(0).playing);
  CHECK_NEAR(o[10], 0.0, 1e-9);
}

static void testMeterDecay() {
  std::printf("motor: medidor de nível\n");
  Engine e(kSr, 256);
  e.loadTrack(0, makeConst(kSr * 3, 0.5f));
  e.setCrossfader(0.f); e.setMaster(1.f);
  settle(e); e.play(0);
  renderN(e, 4000);
  CHECK(e.deckInfo(0).peak > 0.45f);
  CHECK(e.masterPeak() > 0.45f);
  e.pause(0);
  renderN(e, 60000);                     // 1,25 s
  CHECK(e.deckInfo(0).peak < 0.01f);
}

int main() {
  testQueue();
  testResampler();
  testBuilder();
  testPlayback();
  testMixer();
  testCueAndSeek();
  testLoadAndGarbage();
  testMeterDecay();
  std::printf("\n%d verificações, %d falhas\n", g_checks, g_fail);
  return g_fail == 0 ? 0 : 1;
}
