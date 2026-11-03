// Motor de DJ: dois decks, crossfader e master. C++ puro, sem Android, para ser testado no Linux.
//
// Threads:
//  - controle (UI, MIDI): chama os métodos públicos. Vários produtores são serializados por um mutex
//    (fora do tempo real) e entregam comandos à thread de áudio por uma fila sem trava.
//  - áudio: chama render(). Não aloca, não trava, não faz I/O.
// Toda descontinuidade (pausar, buscar, cue) passa por uma rampa de ganho de ~4 ms até zero, sem estalo.
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "spsc_queue.h"
#include "track.h"

namespace djf {

class Engine {
 public:
  static constexpr int kDecks = 2;

  struct DeckInfo {
    bool loaded = false;
    bool playing = false;       // intenção de tocar (o áudio sai com rampa de 4 ms)
    int64_t positionFrames = 0;
    int64_t cueFrames = 0;
    int64_t lengthFrames = 0;
    float peak = 0.f;           // 0 a 1, com queda gradual
  };

  Engine(int sampleRate, int maxBlockFrames);

  // ---- thread de controle ----
  // Falha se o deck for inválido ou se a faixa não estiver na taxa do motor. Carregar sobre um deck
  // tocando corta o som na hora: a interface deve impedir isso.
  bool loadTrack(int deck, std::shared_ptr<const Track> track);
  void unload(int deck);
  void play(int deck);
  void pause(int deck);
  // Tocando: volta ao cue e para. Parado: grava a posição atual como cue.
  void cue(int deck);
  void seekFrames(int deck, int64_t frame);
  void setVolume(int deck, float v);      // 0 a 1
  void setCrossfader(float x);            // 0 = só A, 1 = só B, potência constante
  void setMaster(float v);                // 0 a 1

  DeckInfo deckInfo(int deck) const;
  float masterPeak() const { return masterPeak_.load(std::memory_order_relaxed); }
  // Libera faixas antigas já substituídas pela thread de áudio.
  void collectGarbage();
  int sampleRate() const { return sr_; }

  // ---- thread de áudio ----
  // `out`: estéreo intercalado, float, `frames` quadros.
  void render(float* out, int frames);

 private:
  enum class CmdType : uint8_t { Play, Pause, Cue, Seek, Volume, Crossfader, Master, Load };
  struct Cmd {
    CmdType type = CmdType::Play;
    uint8_t deck = 0;
    float f = 0.f;
    int64_t i = 0;
    const Track* track = nullptr;
    uint32_t seq = 0;
  };

  struct DeckAudio {
    const Track* track = nullptr;
    int64_t pos = 0;
    int64_t cue = 0;
    int64_t pendingSeek = -1;
    bool wantPlay = false;
    float playGain = 0.f;
    float vol = 1.f;
    float volSm = 1.f;
    float peak = 0.f;
  };

  struct DeckPub {
    std::atomic<bool> loaded{false};
    std::atomic<bool> playing{false};
    std::atomic<int64_t> pos{0};
    std::atomic<int64_t> cue{0};
    std::atomic<int64_t> length{0};
    std::atomic<float> peak{0.f};
    std::atomic<uint32_t> appliedSeq{0};
  };

  struct Retired {
    std::shared_ptr<const Track> track;
    int deck;
    uint32_t seq;
  };

  bool send(const Cmd& c);
  void drain();
  void renderBlock(float* out, int n);

  const int sr_;
  const int maxBlock_;
  float rampStep_;
  float smoothCoef_;
  float peakDecayPerFrame_;

  // lado do áudio
  DeckAudio d_[kDecks];
  float xTarget_ = 0.5f, xSm_ = 0.5f;
  float masterTarget_ = 0.9f, masterSm_ = 0.9f;
  float masterPeakLocal_ = 0.f;

  // publicado para a interface
  DeckPub pub_[kDecks];
  std::atomic<float> masterPeak_{0.f};

  // lado do controle
  SpscQueue<Cmd, 1024> queue_;
  std::mutex producerMutex_;
  std::shared_ptr<const Track> current_[kDecks];
  uint32_t issued_[kDecks] = {0, 0};
  std::vector<Retired> retired_;
};

}  // namespace djf
