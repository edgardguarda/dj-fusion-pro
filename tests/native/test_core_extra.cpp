// Testes do núcleo de áudio, rodam no Linux sem Android.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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
#define CHECK(c)                                                              \
  do {                                                                        \
    ++g_checks;                                                               \
    if (!(c)) { ++g_fail; std::printf("  FALHA %s:%d  %s\n", __FILE__, __LINE__, #c); } \
  } while (0)
#define CHECK_NEAR(a, b, tol)                                                 \
  do {                                                                        \
    ++g_checks;                                                               \
    const double _a = (a), _b = (b);                                          \
    if (!(std::fabs(_a - _b) <= (tol))) {                                     \
      ++g_fail;                                                               \
      std::printf("  FALHA %s:%d  %s=%g esperado %g (tol %g)\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); \
    }                                                                         \
  } while (0)

static const double kPi = 3.14159265358979323846;
static const int kSr = 48000;

// ---- utilidades ----
static std::shared_ptr<Track> makeTrack(int64_t frames, double (*fn)(int64_t, int), int sr = kSr) {
  auto t = std::make_shared<Track>();
  t->sampleRate = sr;
  t->frames = frames;
  t->pcm.resize(static_cast<size_t>(frames) * 2);
  for (int64_t i = 0; i < frames; ++i)
    for (int c = 0; c < 2; ++c)
      t->pcm[static_cast<size_t>(i) * 2 + c] = static_cast<int16_t>(std::lrint(fn(i, c) * 32767.0));
  return t;
}
static double dc05(int64_t, int) { return 0.5; }
static double sine100(int64_t i, int) { return 0.5 * std::sin(2 * kPi * 100.0 * i / kSr); }
static double full(int64_t, int) { return 1.0; }

// Renderiza n quadros e devolve só o canal esquerdo; também checa NaN.
static std::vector<float> renderL(Engine& e, int n, int block = 256) {
  std::vector<float> res;
  std::vector<float> buf(static_cast<size_t>(block) * 2);
  while (n > 0) {
    const int k = std::min(n, block);
    e.render(buf.data(), k);
    for (int i = 0; i < k; ++i) {
      CHECK(std::isfinite(buf[2 * i]) && std::isfinite(buf[2 * i + 1]));
      res.push_back(buf[2 * i]);
    }
    n -= k;
  }
  return res;
}
static void settle(Engine& e) { renderL(e, 10000); }  // deixa as suavizações convergirem

// ---- fila ----
static void test_queue() {
  std::puts("fila sem trava");
  SpscQueue<int, 8> q;
  int v;
  CHECK(!q.pop(v));
  for (int i = 0; i < 7; ++i) CHECK(q.push(i));
  CHECK(!q.push(99));  // cheia: capacidade 7
  for (int i = 0; i < 7; ++i) { CHECK(q.pop(v)); CHECK(v == i); }
  CHECK(!q.pop(v));

  SpscQueue<int, 256> q2;
  const int N = 500000;
  std::thread prod([&] { for (int i = 0; i < N; ++i) while (!q2.push(i)) std::this_thread::yield(); });
  bool ordem = true;
  for (int i = 0; i < N; ++i) {
    int x;
    while (!q2.pop(x)) std::this_thread::yield();
    if (x != i) { ordem = false; break; }
  }
  prod.join();
  CHECK(ordem);
}

// ---- conversor de taxa ----
static double goertzel(const std::vector<float>& x, size_t from, size_t to, double freq, double sr) {
  const double w = 2 * kPi * freq / sr;
  double re = 0, im = 0;
  for (size_t i = from; i < to; ++i) { re += x[i] * std::cos(w * (i - from)); im += x[i] * std::sin(w * (i - from)); }
  return 2.0 * std::sqrt(re * re + im * im) / (to - from);
}

static void test_resampler() {
  std::puts("conversor de taxa");
  {  // 44100 -> 48000, L=1 kHz, R=2 kHz
    const int inRate = 44100, n = 44100;
    std::vector<float> in(static_cast<size_t>(n) * 2);
    for (int i = 0; i < n; ++i) {
      in[2 * i] = 0.5f * std::sin(2 * kPi * 1000.0 * i / inRate);
      in[2 * i + 1] = 0.5f * std::sin(2 * kPi * 2000.0 * i / inRate);
    }
    StreamResampler rs(inRate, 48000);
    std::vector<float> out;
    rs.process(in.data(), n, out);
    rs.flush(out);
    const size_t m = out.size() / 2;
    CHECK(std::llabs(static_cast<long long>(m) - 48000) <= 2);
    double maxErr = 0;
    for (size_t i = 64; i + 64 < m; ++i) {
      maxErr = std::max(maxErr, std::fabs(out[2 * i] - 0.5 * std::sin(2 * kPi * 1000.0 * i / 48000.0)));
      maxErr = std::max(maxErr, std::fabs(out[2 * i + 1] - 0.5 * std::sin(2 * kPi * 2000.0 * i / 48000.0)));
    }
    std::printf("  erro máximo 44,1->48 kHz: %.5f\n", maxErr);
    CHECK(maxErr < 0.004);
  }
  {  // 96000 -> 48000: 5 kHz passa, 40 kHz (alias em 8 kHz) some
    const int inRate = 96000, n = 96000;
    std::vector<float> in(static_cast<size_t>(n) * 2);
    for (int i = 0; i < n; ++i) {
      const float v = 0.4f * std::sin(2 * kPi * 5000.0 * i / inRate) + 0.4f * std::sin(2 * kPi * 40000.0 * i / inRate);
      in[2 * i] = in[2 * i + 1] = v;
    }
    StreamResampler rs(inRate, 48000);
    std::vector<float> out;
    rs.process(in.data(), n, out);
    rs.flush(out);
    std::vector<float> L(out.size() / 2);
    for (size_t i = 0; i < L.size(); ++i) L[i] = out[2 * i];
    const double a5k = goertzel(L, 1000, 40000, 5000, 48000);
    const double alias = goertzel(L, 1000, 40000, 8000, 48000);
    std::printf("  5 kHz=%.4f  alias de 40 kHz em 8 kHz=%.5f\n", a5k, alias);
    CHECK_NEAR(a5k, 0.4, 0.01);
    CHECK(alias < 0.004);
  }
  {  // DC passa com ganho 1
    std::vector<float> in(2 * 5000, 0.25f), out;
    StreamResampler rs(22050, 48000);
    rs.process(in.data(), 5000, out);
    rs.flush(out);
    CHECK_NEAR(out[out.size() / 2], 0.25, 0.001);
  }
}

// ---- construtor de faixa ----
static std::vector<int16_t> sineS16(int rate, int n, double f, double amp) {
  std::vector<int16_t> v(n);
  for (int i = 0; i < n; ++i) v[i] = static_cast<int16_t>(std::lrint(amp * 32767.0 * std::sin(2 * kPi * f * i / rate)));
  return v;
}

static void test_builder() {
  std::puts("construtor de faixa");
  const auto sig = sineS16(44100, 44100, 440.0, 0.5);
  auto buildChunks = [&](const std::vector<size_t>& chunks) {
    TrackBuilder b(44100, 48000, 10 * 48000);
    size_t pos = 0, k = 0;
    while (pos < sig.size()) {
      const size_t c = std::min(chunks[k++ % chunks.size()], sig.size() - pos);
      b.appendPcm16(sig.data() + pos, c, 1);
      pos += c;
    }
    return b.finish();
  };
  auto a = buildChunks({44100});
  auto b = buildChunks({1, 7, 1000, 333, 4096, 13});
  CHECK(std::llabs(a->frames - 48000) <= 2);
  CHECK(a->frames == b->frames);
  CHECK(a->pcm == b->pcm);  // o resultado não depende do tamanho dos pedaços
  CHECK(a->pcm[2 * 5000] == a->pcm[2 * 5000 + 1]);  // mono vira dois lados iguais
  CHECK(a->peakBins() >= 149 && a->peakBins() <= 151);

  // graves x agudos
  auto lowT = [&](double f) {
    const auto s = sineS16(48000, 48000, f, 0.5);
    TrackBuilder tb(48000, 48000, 5 * 48000);
    tb.appendPcm16(s.data(), s.size(), 1);
    auto t = tb.finish();
    double lo = 0, mi = 0, hi = 0;
    for (int64_t i = 20; i < t->peakBins() - 20; ++i) { lo += t->peaks[i * 3]; mi += t->peaks[i * 3 + 1]; hi += t->peaks[i * 3 + 2]; }
    return std::array<double, 3>{lo, mi, hi};
  };
  auto g = lowT(60.0), m = lowT(1000.0), h = lowT(9000.0);
  CHECK(g[0] > g[1] * 2 && g[0] > g[2] * 5);
  CHECK(m[1] > m[0] && m[1] > m[2]);
  CHECK(h[2] > h[0] * 5 && h[2] > h[1] * 2);

  // estéreo com 2 canais usa L e R
  std::vector<int16_t> st(2 * 1000);
  for (int i = 0; i < 1000; ++i) { st[2 * i] = 1000; st[2 * i + 1] = -2000; }
  TrackBuilder sb(48000, 48000, 48000);
  sb.appendPcm16(st.data(), 1000, 2);
  auto stt = sb.finish();
  CHECK(stt->frames == 1000 && stt->pcm[500 * 2] == 1000 && stt->pcm[500 * 2 + 1] == -2000);

  // proteção de tamanho
  TrackBuilder lim(48000, 48000, 1000);
  const auto big = sineS16(48000, 5000, 100, 0.3);
  lim.appendPcm16(big.data(), big.size(), 1);
  auto lt = lim.finish();
  CHECK(lim.tooLong());
  CHECK(lt->frames == 1000);

  // áudio em float
  std::vector<float> fl(480, 0.5f);
  TrackBuilder fb(48000, 48000, 48000);
  fb.appendFloat(fl.data(), 480, 1);
  auto ft = fb.finish();
  CHECK(ft->frames == 480 && std::abs(ft->pcm[100] - 16384) <= 1);
}


// Regressão: carregar uma faixa longa em blocos pequenos (como o MediaCodec entrega) não pode ser quadrático.
static void test_builder_speed() {
  std::puts("construtor de faixa: velocidade com faixa longa");
  const int rate = 44100, seconds = 240;
  const auto chunk = sineS16(rate, 1152 * 2, 440.0, 0.4);  // 1152 quadros estéreo, bloco típico de MP3
  const auto t0 = std::chrono::steady_clock::now();
  TrackBuilder b(rate, 48000, 15LL * 60 * 48000);
  const int chunks = rate * seconds / 1152;
  for (int i = 0; i < chunks; ++i) b.appendPcm16(chunk.data(), 1152, 2);
  auto t = b.finish();
  const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("  %d s de áudio em %d blocos: %.2f s, %lld quadros\n", seconds, chunks, secs, (long long)t->frames);
  const long long esperado = (long long)chunks * 1152 * 48000 / rate;
  CHECK(std::llabs(t->frames - esperado) <= 2);
#if defined(__SANITIZE_THREAD__)
  CHECK(secs < 180.0);  // o TSan deixa tudo ~20x mais lento; a regressão de velocidade é pega nas outras execuções
#else
  CHECK(secs < 10.0);
#endif
}

// ---- motor ----
static void test_engine_playback() {
  std::puts("motor: reprodução");
  Engine e(kSr, 256);
  auto t = makeTrack(kSr * 4, dc05);
  CHECK(e.loadTrack(0, t));
  e.setCrossfader(0.f);
  e.setMaster(1.f);
  settle(e);
  CHECK(e.deckInfo(0).loaded);
  CHECK(!e.deckInfo(0).playing);

  // parado: silêncio
  auto s0 = renderL(e, 512);
  CHECK_NEAR(*std::max_element(s0.begin(), s0.end()), 0.0, 1e-9);

  e.play(0);
  auto s = renderL(e, 2000);
  CHECK(e.deckInfo(0).playing);
  CHECK_NEAR(e.deckInfo(0).positionFrames, 2000, 0);  // 1 quadro por quadro renderizado
  // fade de 4 ms (192 quadros): sobe sem saltos
  bool monotono = true;
  for (int i = 1; i < 190; ++i) if (s[i] < s[i - 1]) monotono = false;
  CHECK(monotono);
  CHECK(s[0] < 0.01f);
  CHECK_NEAR(s[1000], 0.5, 1e-3);
  CHECK_NEAR(s[1999], 0.5, 1e-3);

  // pausa: para sem corte seco e a posição congela
  e.pause(0);
  auto p = renderL(e, 600);
  CHECK(!e.deckInfo(0).playing);
  double maxStep = 0;
  for (size_t i = 1; i < p.size(); ++i) maxStep = std::max(maxStep, (double)std::fabs(p[i] - p[i - 1]));
  CHECK(maxStep < 0.01);  // sem clique
  CHECK_NEAR(p.back(), 0.0, 1e-9);
  const int64_t frozen = e.deckInfo(0).positionFrames;
  renderL(e, 1000);
  CHECK(e.deckInfo(0).positionFrames == frozen);
  CHECK(frozen > 2000 && frozen < 2000 + 200);
}

static void test_engine_mix() {
  std::puts("motor: crossfader e volumes");
  auto level = [](float x, float va, float vb, float master, bool a, bool b) {
    Engine e(kSr, 256);
    e.loadTrack(0, makeTrack(kSr * 3, dc05));
    e.loadTrack(1, makeTrack(kSr * 3, dc05));
    e.setCrossfader(x); e.setVolume(0, va); e.setVolume(1, vb); e.setMaster(master);
    settle(e);
    if (a) e.play(0);
    if (b) e.play(1);
    auto s = renderL(e, 4000);
    return s[3500];
  };
  CHECK_NEAR(level(0.f, 1, 1, 1, true, true), 0.5, 2e-3);        // só A
  CHECK_NEAR(level(1.f, 1, 1, 1, true, true), 0.5, 2e-3);        // só B
  CHECK_NEAR(level(0.5f, 1, 1, 1, true, true), 0.7071, 3e-3);    // potência constante: soma = 0,5*0,7071*2
  CHECK_NEAR(level(0.f, 1, 1, 1, false, true), 0.0, 1e-4);       // B tocando com crossfader em A: mudo
  CHECK_NEAR(level(0.f, 0.5f, 1, 1, true, false), 0.25, 2e-3);   // volume do deck
  CHECK_NEAR(level(0.f, 1, 1, 0.5f, true, false), 0.25, 2e-3);   // master
  CHECK_NEAR(level(0.f, 0.f, 1, 1, true, false), 0.0, 1e-4);     // volume zero

  // sem estouro: dois decks cheios no centro passam de 1 e são limitados
  Engine e(kSr, 256);
  e.loadTrack(0, makeTrack(kSr, full)); e.loadTrack(1, makeTrack(kSr, full));
  e.setCrossfader(0.5f); e.setMaster(1.f); settle(e);
  e.play(0); e.play(1);
  auto s = renderL(e, 3000);
  CHECK(*std::max_element(s.begin(), s.end()) <= 1.0f);
}

static void test_engine_cue_seek_end() {
  std::puts("motor: cue, seek e fim da faixa");
  Engine e(kSr, 256);
  e.loadTrack(0, makeTrack(kSr * 4, sine100));
  e.setCrossfader(0.f); e.setMaster(1.f); settle(e);

  // cue parado define o ponto
  e.play(0); renderL(e, 1234); e.pause(0); renderL(e, 1000);
  const int64_t paused = e.deckInfo(0).positionFrames;
  e.cue(0); renderL(e, 64);
  CHECK(e.deckInfo(0).cueFrames == paused);
  // tocar e apertar cue volta ao ponto e para
  e.play(0); renderL(e, 5000);
  CHECK(e.deckInfo(0).positionFrames > paused + 4000);
  e.cue(0); renderL(e, 2000);
  CHECK(!e.deckInfo(0).playing);
  CHECK(e.deckInfo(0).positionFrames == paused);

  // seek durante a reprodução, sem salto audível (seno de 100 Hz, passo máximo normal ~0,0065)
  e.play(0);
  std::vector<float> all;
  auto a = renderL(e, 3000); all.insert(all.end(), a.begin(), a.end());
  e.seekFrames(0, 100000);
  auto b = renderL(e, 3000); all.insert(all.end(), b.begin(), b.end());
  double maxStep = 0;
  for (size_t i = 1; i < all.size(); ++i) maxStep = std::max(maxStep, (double)std::fabs(all[i] - all[i - 1]));
  std::printf("  maior salto entre amostras com seek: %.4f\n", maxStep);
  CHECK(maxStep < 0.02);
  const int64_t after = e.deckInfo(0).positionFrames;
  CHECK(after > 100000 && after < 100000 + 3000);
  CHECK(e.deckInfo(0).playing);  // continuou tocando

  // seek parado
  e.pause(0); renderL(e, 600);
  e.seekFrames(0, 777); renderL(e, 64);
  CHECK(e.deckInfo(0).positionFrames == 777);
  e.seekFrames(0, -50); renderL(e, 64);
  CHECK(e.deckInfo(0).positionFrames == 0);
  e.seekFrames(0, 1LL << 40); renderL(e, 64);
  CHECK(e.deckInfo(0).positionFrames == kSr * 4);

  // fim da faixa: para sozinho; tocar de novo recomeça do início
  Engine f(kSr, 256);
  f.loadTrack(0, makeTrack(1000, dc05));
  f.setCrossfader(0.f); f.setMaster(1.f); settle(f);
  f.play(0);
  auto s = renderL(f, 3000);
  CHECK(!f.deckInfo(0).playing);
  CHECK(f.deckInfo(0).positionFrames == 1000);
  CHECK_NEAR(s.back(), 0.0, 1e-9);
  f.play(0); renderL(f, 300);
  CHECK(f.deckInfo(0).playing);
  CHECK(f.deckInfo(0).positionFrames > 0 && f.deckInfo(0).positionFrames <= 300);
}

static void test_engine_load_meter() {
  std::puts("motor: carregar, liberar memória e medidor");
  Engine e(kSr, 256);
  auto t1 = makeTrack(kSr, dc05);
  std::weak_ptr<Track> w1 = t1;
  CHECK(e.loadTrack(0, t1));
  t1.reset();
  renderL(e, 256);
  e.collectGarbage();
  CHECK(!w1.expired());  // ainda é a faixa atual do deck

  CHECK(e.loadTrack(0, makeTrack(kSr, dc05)));
  e.collectGarbage();
  CHECK(!w1.expired());  // a thread de áudio ainda não largou a faixa antiga
  renderL(e, 256);
  e.collectGarbage();
  CHECK(w1.expired());   // agora liberou

  CHECK(!e.loadTrack(0, makeTrack(1000, dc05, 44100)));  // taxa errada
  CHECK(!e.loadTrack(5, makeTrack(1000, dc05)));         // deck inválido
  CHECK(!e.loadTrack(0, nullptr));

  e.setCrossfader(0.f); e.setMaster(1.f); settle(e);
  e.play(0);
  renderL(e, 2000);
  CHECK_NEAR(e.deckInfo(0).peak, 0.5, 2e-3);
  CHECK_NEAR(e.masterPeak(), 0.5, 2e-3);
  e.pause(0);
  renderL(e, kSr);  // 1 s depois, a queda de 0,8 s zerou
  CHECK_NEAR(e.deckInfo(0).peak, 0.0, 1e-6);

  e.unload(0); renderL(e, 256);
  CHECK(!e.deckInfo(0).loaded);
  e.play(0); renderL(e, 256);
  CHECK(!e.deckInfo(0).playing);  // sem faixa, play não faz nada
}

// ---- tempo real ----
static void test_threads() {
  std::puts("motor: controle e áudio em threads separadas");
  Engine e(kSr, 256);
  e.loadTrack(0, makeTrack(kSr * 5, sine100));
  e.loadTrack(1, makeTrack(kSr * 5, sine100));
  std::atomic<bool> stop{false};
  std::atomic<long> bad{0};
  std::thread audio([&] {
    std::vector<float> buf(2 * 333);
    while (!stop.load()) {
      e.render(buf.data(), 333);
      for (float v : buf) if (!std::isfinite(v) || std::fabs(v) > 1.0f) ++bad;
    }
  });
  auto ctrl = [&](unsigned seed) {
    std::mt19937 rng(seed);
    for (int i = 0; i < 20000; ++i) {
      const int d = rng() % 2;
      switch (rng() % 9) {
        case 0: e.play(d); break;
        case 1: e.pause(d); break;
        case 2: e.cue(d); break;
        case 3: e.seekFrames(d, rng() % (kSr * 5)); break;
        case 4: e.setVolume(d, (rng() % 101) / 100.f); break;
        case 5: e.setCrossfader((rng() % 101) / 100.f); break;
        case 6: e.setMaster((rng() % 101) / 100.f); break;
        case 7: e.loadTrack(d, makeTrack(kSr / 2 + rng() % 1000, sine100)); break;
        case 8: e.collectGarbage(); (void)e.deckInfo(d); break;
      }
      if (i % 64 == 0) std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
  };
  std::thread c1(ctrl, 1), c2(ctrl, 2);
  c1.join(); c2.join();
  stop = true;
  audio.join();
  CHECK(bad.load() == 0);
}

int main() {
  test_queue();
  test_resampler();
  test_builder();
  test_builder_speed();
  test_engine_playback();
  test_engine_mix();
  test_engine_cue_seek_end();
  test_engine_load_meter();
  test_threads();
  std::printf("\n%d verificações, %d falhas\n", g_checks, g_fail);
  return g_fail == 0 ? 0 : 1;
}
