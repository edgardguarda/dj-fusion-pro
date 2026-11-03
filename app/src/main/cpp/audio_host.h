// Liga o motor (core/engine) a uma saída de áudio Oboe de baixa latência.
// Este é o único arquivo que depende do Oboe.
#pragma once
#include <oboe/Oboe.h>

#include <android/log.h>

#include <atomic>
#include <memory>
#include <mutex>

#include "core/engine.h"

namespace djf {

constexpr int kEngineRate = 48000;   // todas as faixas são decodificadas nesta taxa
constexpr int kMaxBlockFrames = 4096;

class AudioHost : public oboe::AudioStreamDataCallback,
                  public oboe::AudioStreamErrorCallback,
                  public std::enable_shared_from_this<AudioHost> {
 public:
  AudioHost() : engine_(kEngineRate, kMaxBlockFrames) {}

  Engine& engine() { return engine_; }

  bool start() {
    std::lock_guard<std::mutex> lock(mutex_);
    wanted_ = true;
    if (stream_) return true;
    return openLocked();
  }

  void stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    wanted_ = false;
    closeLocked();
  }

  // out: [taxa real, quadros por rajada, tamanho do buffer em quadros, 1 se exclusivo]
  void info(int32_t out[4]) const {
    for (int i = 0; i < 4; ++i) out[i] = info_[i].load(std::memory_order_relaxed);
  }

  oboe::DataCallbackResult onAudioReady(oboe::AudioStream*, void* audioData, int32_t numFrames) override {
    engine_.render(static_cast<float*>(audioData), numFrames);
    return oboe::DataCallbackResult::Continue;
  }

  // Fone desligado, USB removido ou rota trocada: o Oboe fecha o fluxo e avisa aqui. Reabrimos.
  void onErrorAfterClose(oboe::AudioStream*, oboe::Result error) override {
    __android_log_print(ANDROID_LOG_WARN, "DJFusion", "erro de áudio: %s", oboe::convertToText(error));
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) return;  // stop() está em andamento
    stream_.reset();
    if (wanted_) openLocked();
  }

 private:
  bool openLocked() {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        ->setSampleRate(kEngineRate)
        ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
        ->setUsage(oboe::Usage::Media)
        ->setContentType(oboe::ContentType::Music)
        ->setDataCallback(shared_from_this())
        ->setErrorCallback(shared_from_this());
    std::shared_ptr<oboe::AudioStream> stream;
    oboe::Result r = builder.openStream(stream);
    if (r != oboe::Result::OK) {
      __android_log_print(ANDROID_LOG_ERROR, "DJFusion", "não abriu o áudio: %s", oboe::convertToText(r));
      return false;
    }
    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    r = stream->requestStart();
    if (r != oboe::Result::OK) {
      __android_log_print(ANDROID_LOG_ERROR, "DJFusion", "não iniciou o áudio: %s", oboe::convertToText(r));
      stream->close();
      return false;
    }
    info_[0].store(stream->getSampleRate());
    info_[1].store(stream->getFramesPerBurst());
    info_[2].store(stream->getBufferSizeInFrames());
    info_[3].store(stream->getSharingMode() == oboe::SharingMode::Exclusive ? 1 : 0);
    stream_ = std::move(stream);
    return true;
  }

  void closeLocked() {
    if (stream_) {
      stream_->requestStop();
      stream_->close();
      stream_.reset();
    }
  }

  Engine engine_;
  std::mutex mutex_;
  std::shared_ptr<oboe::AudioStream> stream_;
  bool wanted_ = false;
  std::atomic<int32_t> info_[4] = {{0}, {0}, {0}, {0}};
};

}  // namespace djf
