// STUB só para checar a sintaxe do nosso código. Imita, de memória, a API do Oboe 1.9.
#pragma once
#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
namespace oboe {
enum class Result { OK = 0, ErrorDisconnected = -899 };
enum class Direction { Output = 0, Input = 1 };
enum class PerformanceMode { LowLatency = 12 };
enum class SharingMode { Exclusive = 0, Shared = 1 };
enum class AudioFormat { Float = 5 };
enum ChannelCount : int32_t { Unspecified = 0, Mono = 1, Stereo = 2 };
enum class SampleRateConversionQuality { Medium = 2 };
enum class Usage { Media = 1 };
enum class ContentType { Music = 2 };
enum class DataCallbackResult { Continue = 0, Stop = 1 };
inline const char* convertToText(Result) { return "x"; }
class AudioStream;
class AudioStreamDataCallback { public: virtual ~AudioStreamDataCallback() = default;
  virtual DataCallbackResult onAudioReady(AudioStream*, void*, int32_t) = 0; };
class AudioStreamErrorCallback { public: virtual ~AudioStreamErrorCallback() = default;
  virtual bool onError(AudioStream*, Result) { return false; }
  virtual void onErrorBeforeClose(AudioStream*, Result) {}
  virtual void onErrorAfterClose(AudioStream*, Result) {} };
class AudioStream { public:
  std::shared_ptr<AudioStreamDataCallback> cb; std::thread th; std::atomic<bool> run{false};
  ~AudioStream() { run = false; if (th.joinable()) th.join(); }
  Result requestStart() { run = true; th = std::thread([this] { std::vector<float> buf(2 * 480);
      while (run) { cb->onAudioReady(this, buf.data(), 480); std::this_thread::sleep_for(std::chrono::milliseconds(10)); } }); return Result::OK; }
  Result requestStop() { run = false; if (th.joinable()) th.join(); return Result::OK; } Result close() { return Result::OK; }
  int32_t getSampleRate() const { return 48000; } int32_t getFramesPerBurst() const { return 192; }
  int32_t getBufferSizeInFrames() const { return 384; } SharingMode getSharingMode() const { return SharingMode::Exclusive; }
  int setBufferSizeInFrames(int32_t) { return 0; } };
class AudioStreamBuilder { public:
  AudioStreamBuilder* setDirection(Direction) { return this; } AudioStreamBuilder* setPerformanceMode(PerformanceMode) { return this; }
  AudioStreamBuilder* setSharingMode(SharingMode) { return this; } AudioStreamBuilder* setFormat(AudioFormat) { return this; }
  AudioStreamBuilder* setChannelCount(int32_t) { return this; } AudioStreamBuilder* setSampleRate(int32_t) { return this; }
  AudioStreamBuilder* setSampleRateConversionQuality(SampleRateConversionQuality) { return this; }
  AudioStreamBuilder* setUsage(Usage) { return this; } AudioStreamBuilder* setContentType(ContentType) { return this; }
  std::shared_ptr<AudioStreamDataCallback> cb_; AudioStreamBuilder* setDataCallback(std::shared_ptr<AudioStreamDataCallback> c) { cb_ = c; return this; }
  AudioStreamBuilder* setErrorCallback(std::shared_ptr<AudioStreamErrorCallback>) { return this; }
  Result openStream(std::shared_ptr<AudioStream>& s) { s = std::make_shared<AudioStream>(); s->cb = cb_; return Result::OK; } };
}
