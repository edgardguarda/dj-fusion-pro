// Duas garantias do callback de áudio:
//  1) nenhuma alocação de memória durante render() (alocar no callback causa falhas de áudio);
//  2) nenhuma corrida de dados quando a interface manda comandos enquanto o áudio toca (use -fsanitize=thread).
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <thread>
#include <vector>

#include "../../app/src/main/cpp/core/engine.h"

#ifdef COUNT_ALLOCS
static thread_local bool g_counting = false;
static thread_local long g_allocs = 0;
void* operator new(std::size_t n) {
  if (g_counting) ++g_allocs;
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
#endif

using namespace djf;

static std::shared_ptr<Track> sine(int64_t frames, double hz) {
  auto t = std::make_shared<Track>();
  t->sampleRate = 48000; t->frames = frames; t->pcm.resize(frames * 2);
  for (int64_t i = 0; i < frames; ++i) {
    int16_t s = static_cast<int16_t>(12000 * std::sin(6.283185307 * hz * i / 48000.0));
    t->pcm[2 * i] = s; t->pcm[2 * i + 1] = s;
  }
  return t;
}

int main() {
  int failures = 0;
  Engine e(48000, 480);
  e.loadTrack(0, sine(48000 * 30, 220)); e.loadTrack(1, sine(48000 * 30, 330));
  std::vector<float> out(2 * 480);

#ifdef COUNT_ALLOCS
  {
    e.play(0); e.play(1); e.setCrossfader(0.3f); e.seekFrames(0, 5000); e.cue(1);
    g_counting = true;
    for (int i = 0; i < 2000; ++i) {
      if (i % 50 == 0) { e.setVolume(0, (i % 100) ? 0.4f : 1.f); e.seekFrames(1, i * 7); }
      e.render(out.data(), 480);
    }
    g_counting = false;
    std::printf("alocações durante 2000 chamadas de render(): %ld\n", g_allocs);
    if (g_allocs != 0) { std::printf("FALHOU: o callback de áudio alocou memória\n"); ++failures; }
  }
#endif

#ifdef THREAD_TEST
  {
    std::atomic<bool> stop{false};
    std::atomic<long> blocks{0};
    bool finiteOk = true;
    std::thread audio([&] {
      std::vector<float> o(2 * 480);
      while (!stop.load()) {
        e.render(o.data(), 480);
        for (float v : o) if (!std::isfinite(v) || std::fabs(v) > 1.0f) finiteOk = false;
        blocks.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::microseconds(200));
      }
    });
    auto controller = [&](unsigned seed) {
      std::mt19937 rng(seed);
      for (int i = 0; i < 20000; ++i) {
        int d = rng() % 2;
        switch (rng() % 9) {
          case 0: e.play(d); break;
          case 1: e.pause(d); break;
          case 2: e.cue(d); break;
          case 3: e.seekFrames(d, rng() % (48000 * 30)); break;
          case 4: e.setVolume(d, (rng() % 101) / 100.f); break;
          case 5: e.setCrossfader((rng() % 101) / 100.f); break;
          case 6: e.setMaster((rng() % 101) / 100.f); break;
          case 7: e.loadTrack(d, sine(2000 + rng() % 4000, 100 + rng() % 500)); break;
          default: { auto info = e.deckInfo(d); (void)info; e.collectGarbage(); }
        }
        if (i % 64 == 0) std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    };
    std::thread c1(controller, 1), c2(controller, 2);  // dois produtores: interface e MIDI
    c1.join(); c2.join();
    stop = true; audio.join();
    std::printf("corrida de threads: %ld blocos renderizados com 2 produtores, saída finita e limitada: %s\n",
                blocks.load(), finiteOk ? "sim" : "NAO");
    if (!finiteOk || blocks.load() < 100) ++failures;
  }
#endif

  std::printf(failures ? "FALHOU\n" : "OK\n");
  return failures ? 1 : 0;
}
