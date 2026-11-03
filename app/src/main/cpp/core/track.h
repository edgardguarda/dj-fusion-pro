// Faixa decodificada na taxa do motor (estéreo, 16 bits) e o construtor que a produz aos poucos.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "resampler.h"

namespace djf {

struct Track {
  std::vector<int16_t> pcm;      // estéreo intercalado
  int64_t frames = 0;
  int sampleRate = 48000;
  int peaksPerSecond = 150;
  std::vector<uint8_t> peaks;    // [bin][3]: graves, médios, agudos, 0 a 255

  double durationSec() const { return sampleRate > 0 ? static_cast<double>(frames) / sampleRate : 0.0; }
  int64_t peakBins() const { return static_cast<int64_t>(peaks.size() / 3); }
};

class TrackBuilder {
 public:
  // maxFrames limita o tamanho final (proteção de memória).
  TrackBuilder(int srcRate, int outRate, int64_t maxFrames, int peaksPerSecond = 150)
      : outRate_(outRate), maxFrames_(maxFrames), pps_(peaksPerSecond), rs_(srcRate, outRate) {
    const double pi = 3.14159265358979323846;
    a1_ = 1.0 - std::exp(-2.0 * pi * 250.0 / outRate);
    a2_ = 1.0 - std::exp(-2.0 * pi * 2500.0 / outRate);
    track_ = std::make_shared<Track>();
    track_->sampleRate = outRate;
    track_->peaksPerSecond = peaksPerSecond;
  }

  // channels: 1 duplica nos dois lados; 2 ou mais usa os dois primeiros canais.
  void appendPcm16(const int16_t* data, size_t frames, int channels) {
    if (tooLong_ || channels < 1) return;
    tmp_.resize(frames * 2);
    const float k = 1.0f / 32768.0f;
    for (size_t i = 0; i < frames; ++i) {
      const float l = data[i * channels] * k;
      const float r = channels > 1 ? data[i * channels + 1] * k : l;
      tmp_[2 * i] = l;
      tmp_[2 * i + 1] = r;
    }
    run(frames);
  }

  void appendFloat(const float* data, size_t frames, int channels) {
    if (tooLong_ || channels < 1) return;
    tmp_.resize(frames * 2);
    for (size_t i = 0; i < frames; ++i) {
      const float l = data[i * channels];
      const float r = channels > 1 ? data[i * channels + 1] : l;
      tmp_[2 * i] = l;
      tmp_[2 * i + 1] = r;
    }
    run(frames);
  }

  bool tooLong() const { return tooLong_; }
  // Falta de memória ou erro interno: a faixa é abandonada e o chamador a vê como longa demais.
  void markFailed() { tooLong_ = true; }
  int64_t framesSoFar() const { return track_->frames; }

  std::shared_ptr<Track> finish() {
    out_.clear();
    rs_.flush(out_);
    consume();
    float maxv = 0.f;
    for (float v : bandPeaks_) maxv = std::max(maxv, v);
    track_->peaks.resize(bandPeaks_.size());
    for (size_t i = 0; i < bandPeaks_.size(); ++i) {
      const float n = maxv > 0.f ? bandPeaks_[i] / maxv : 0.f;
      track_->peaks[i] = static_cast<uint8_t>(std::min(255.f, n * 255.f + 0.5f));
    }
    return track_;
  }

 private:
  void run(size_t frames) {
    out_.clear();
    rs_.process(tmp_.data(), frames, out_);
    consume();
  }

  void consume() {
    const size_t n = out_.size() / 2;
    for (size_t i = 0; i < n; ++i) {
      if (track_->frames >= maxFrames_) {
        tooLong_ = true;
        break;
      }
      const float l = out_[2 * i];
      const float r = out_[2 * i + 1];
      track_->pcm.push_back(toS16(l));
      track_->pcm.push_back(toS16(r));

      const double m = 0.5 * (static_cast<double>(l) + r);
      lp1_ += a1_ * (m - lp1_);
      lp2_ += a2_ * (m - lp2_);
      const float low = static_cast<float>(std::fabs(lp1_));
      const float mid = static_cast<float>(std::fabs(lp2_ - lp1_));
      const float high = static_cast<float>(std::fabs(m - lp2_));
      const size_t bin = static_cast<size_t>(track_->frames * pps_ / outRate_);
      while (bandPeaks_.size() < (bin + 1) * 3) bandPeaks_.push_back(0.f);
      bandPeaks_[bin * 3 + 0] = std::max(bandPeaks_[bin * 3 + 0], low);
      bandPeaks_[bin * 3 + 1] = std::max(bandPeaks_[bin * 3 + 1], mid);
      bandPeaks_[bin * 3 + 2] = std::max(bandPeaks_[bin * 3 + 2], high);
      ++track_->frames;
    }
    out_.clear();
  }

  static int16_t toS16(float x) {
    const float c = std::max(-1.0f, std::min(1.0f, x));
    return static_cast<int16_t>(std::lrintf(c * 32767.0f));
  }

  int outRate_;
  int64_t maxFrames_;
  int pps_;
  StreamResampler rs_;
  double a1_ = 0, a2_ = 0, lp1_ = 0, lp2_ = 0;
  bool tooLong_ = false;
  std::shared_ptr<Track> track_;
  std::vector<float> tmp_, out_, bandPeaks_;
};

}  // namespace djf
