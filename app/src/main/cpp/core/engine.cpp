#include "engine.h"

#include <algorithm>
#include <cmath>

namespace djf {

namespace {
constexpr float kHalfPi = 1.57079632679489662f;
inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
}  // namespace

Engine::Engine(int sampleRate, int maxBlockFrames) : sr_(sampleRate), maxBlock_(maxBlockFrames) {
  rampStep_ = 1.0f / (0.004f * static_cast<float>(sr_));
  smoothCoef_ = 1.0f - std::exp(-1.0f / (0.010f * static_cast<float>(sr_)));
  peakDecayPerFrame_ = 1.0f / (0.8f * static_cast<float>(sr_));
}

// ---------------------------------------------------------------- controle

bool Engine::send(const Cmd& c) {
  std::lock_guard<std::mutex> lock(producerMutex_);
  return queue_.push(c);
}

bool Engine::loadTrack(int deck, std::shared_ptr<const Track> track) {
  if (deck < 0 || deck >= kDecks || !track || track->sampleRate != sr_) return false;
  std::lock_guard<std::mutex> lock(producerMutex_);
  Cmd c;
  c.type = CmdType::Load;
  c.deck = static_cast<uint8_t>(deck);
  c.track = track.get();
  c.seq = issued_[deck] + 1;
  if (!queue_.push(c)) return false;
  issued_[deck] = c.seq;
  if (current_[deck]) retired_.push_back({current_[deck], deck, c.seq});
  current_[deck] = std::move(track);
  return true;
}

void Engine::unload(int deck) {
  if (deck < 0 || deck >= kDecks) return;
  std::lock_guard<std::mutex> lock(producerMutex_);
  Cmd c;
  c.type = CmdType::Load;
  c.deck = static_cast<uint8_t>(deck);
  c.track = nullptr;
  c.seq = issued_[deck] + 1;
  if (!queue_.push(c)) return;
  issued_[deck] = c.seq;
  if (current_[deck]) retired_.push_back({current_[deck], deck, c.seq});
  current_[deck].reset();
}

void Engine::play(int deck) {
  if (deck < 0 || deck >= kDecks) return;
  Cmd c; c.type = CmdType::Play; c.deck = static_cast<uint8_t>(deck);
  send(c);
}

void Engine::pause(int deck) {
  if (deck < 0 || deck >= kDecks) return;
  Cmd c; c.type = CmdType::Pause; c.deck = static_cast<uint8_t>(deck);
  send(c);
}

void Engine::cue(int deck) {
  if (deck < 0 || deck >= kDecks) return;
  Cmd c; c.type = CmdType::Cue; c.deck = static_cast<uint8_t>(deck);
  send(c);
}

void Engine::seekFrames(int deck, int64_t frame) {
  if (deck < 0 || deck >= kDecks) return;
  Cmd c; c.type = CmdType::Seek; c.deck = static_cast<uint8_t>(deck); c.i = frame;
  send(c);
}

void Engine::setVolume(int deck, float v) {
  if (deck < 0 || deck >= kDecks) return;
  Cmd c; c.type = CmdType::Volume; c.deck = static_cast<uint8_t>(deck); c.f = clamp01(v);
  send(c);
}

void Engine::setCrossfader(float x) {
  Cmd c; c.type = CmdType::Crossfader; c.f = clamp01(x);
  send(c);
}

void Engine::setMaster(float v) {
  Cmd c; c.type = CmdType::Master; c.f = clamp01(v);
  send(c);
}

Engine::DeckInfo Engine::deckInfo(int deck) const {
  DeckInfo info;
  if (deck < 0 || deck >= kDecks) return info;
  const DeckPub& p = pub_[deck];
  info.loaded = p.loaded.load(std::memory_order_relaxed);
  info.playing = p.playing.load(std::memory_order_relaxed);
  info.positionFrames = p.pos.load(std::memory_order_relaxed);
  info.cueFrames = p.cue.load(std::memory_order_relaxed);
  info.lengthFrames = p.length.load(std::memory_order_relaxed);
  info.peak = p.peak.load(std::memory_order_relaxed);
  return info;
}

void Engine::collectGarbage() {
  std::lock_guard<std::mutex> lock(producerMutex_);
  retired_.erase(
      std::remove_if(retired_.begin(), retired_.end(),
                     [this](const Retired& r) {
                       return pub_[r.deck].appliedSeq.load(std::memory_order_acquire) >= r.seq;
                     }),
      retired_.end());
}

// ------------------------------------------------------------------- áudio

void Engine::drain() {
  Cmd c;
  while (queue_.pop(c)) {
    if (c.type == CmdType::Crossfader) { xTarget_ = c.f; continue; }
    if (c.type == CmdType::Master) { masterTarget_ = c.f; continue; }
    DeckAudio& D = d_[c.deck];
    DeckPub& P = pub_[c.deck];
    switch (c.type) {
      case CmdType::Load:
        D.track = c.track;
        D.pos = 0;
        D.cue = 0;
        D.pendingSeek = -1;
        D.wantPlay = false;
        D.playGain = 0.f;
        P.loaded.store(c.track != nullptr, std::memory_order_relaxed);
        P.length.store(c.track ? c.track->frames : 0, std::memory_order_relaxed);
        P.pos.store(0, std::memory_order_relaxed);
        P.cue.store(0, std::memory_order_relaxed);
        P.playing.store(false, std::memory_order_relaxed);
        P.appliedSeq.store(c.seq, std::memory_order_release);
        break;
      case CmdType::Play:
        if (!D.track) break;
        if (D.pos >= D.track->frames) D.pendingSeek = 0;  // fim da faixa: recomeça do início
        D.wantPlay = true;
        P.playing.store(true, std::memory_order_relaxed);
        break;
      case CmdType::Pause:
        D.wantPlay = false;
        P.playing.store(false, std::memory_order_relaxed);
        break;
      case CmdType::Cue:
        if (!D.track) break;
        if (D.wantPlay) {
          D.pendingSeek = D.cue;
          D.wantPlay = false;
          P.playing.store(false, std::memory_order_relaxed);
        } else {
          D.cue = std::min(D.pos, D.track->frames);
          P.cue.store(D.cue, std::memory_order_relaxed);
        }
        break;
      case CmdType::Seek:
        if (!D.track) break;
        D.pendingSeek = std::max<int64_t>(0, std::min(c.i, D.track->frames));
        break;
      case CmdType::Volume:
        D.vol = c.f;
        break;
      default:
        break;
    }
  }
}

void Engine::render(float* out, int frames) {
  drain();
  while (frames > 0) {
    const int n = std::min(frames, maxBlock_);
    renderBlock(out, n);
    out += 2 * n;
    frames -= n;
  }
}

void Engine::renderBlock(float* out, int n) {
  float blockPeak[kDecks] = {0.f, 0.f};
  float masterBlockPeak = 0.f;
  const float k16 = 1.0f / 32768.0f;

  for (int i = 0; i < n; ++i) {
    xSm_ += (xTarget_ - xSm_) * smoothCoef_;
    masterSm_ += (masterTarget_ - masterSm_) * smoothCoef_;
    const float xg[kDecks] = {std::cos(xSm_ * kHalfPi), std::sin(xSm_ * kHalfPi)};

    float mixL = 0.f, mixR = 0.f;
    for (int d = 0; d < kDecks; ++d) {
      DeckAudio& D = d_[d];
      const bool run = (D.track != nullptr) && D.wantPlay && D.pendingSeek < 0;
      const float target = run ? 1.f : 0.f;
      if (D.playGain < target) D.playGain = std::min(target, D.playGain + rampStep_);
      else if (D.playGain > target) D.playGain = std::max(target, D.playGain - rampStep_);

      if (D.pendingSeek >= 0 && D.playGain <= 0.f) {
        D.pos = D.track ? std::min(D.pendingSeek, D.track->frames) : 0;
        D.pendingSeek = -1;
      }

      float l = 0.f, r = 0.f;
      if (D.track && D.playGain > 0.f) {
        if (D.pos < D.track->frames) {
          const int16_t* s = &D.track->pcm[static_cast<size_t>(D.pos) * 2];
          l = s[0] * k16 * D.playGain;
          r = s[1] * k16 * D.playGain;
          ++D.pos;
        }
        if (D.pos >= D.track->frames) {  // acabou: para sem rampa (a faixa termina em silêncio)
          D.wantPlay = false;
          D.playGain = 0.f;
          pub_[d].playing.store(false, std::memory_order_relaxed);
        }
      }

      D.volSm += (D.vol - D.volSm) * smoothCoef_;
      l *= D.volSm;
      r *= D.volSm;
      blockPeak[d] = std::max(blockPeak[d], std::max(std::fabs(l), std::fabs(r)));
      mixL += l * xg[d];
      mixR += r * xg[d];
    }

    mixL = std::max(-1.f, std::min(1.f, mixL * masterSm_));
    mixR = std::max(-1.f, std::min(1.f, mixR * masterSm_));
    out[2 * i] = mixL;
    out[2 * i + 1] = mixR;
    masterBlockPeak = std::max(masterBlockPeak, std::max(std::fabs(mixL), std::fabs(mixR)));
  }

  const float decay = peakDecayPerFrame_ * static_cast<float>(n);
  for (int d = 0; d < kDecks; ++d) {
    DeckAudio& D = d_[d];
    D.peak = std::max(blockPeak[d], D.peak - decay);
    pub_[d].peak.store(D.peak, std::memory_order_relaxed);
    pub_[d].pos.store(D.pos, std::memory_order_relaxed);
    pub_[d].cue.store(D.cue, std::memory_order_relaxed);
  }
  masterPeakLocal_ = std::max(masterBlockPeak, masterPeakLocal_ - decay);
  masterPeak_.store(masterPeakLocal_, std::memory_order_relaxed);
}

}  // namespace djf
