// Ponte JNI entre o Kotlin (app.djfusion.audio.NativeEngine) e o motor C++.
#include <jni.h>

#include <cstring>
#include <memory>
#include <vector>

#include "audio_host.h"
#include "core/track.h"

namespace {

std::shared_ptr<djf::AudioHost>& hostPtr() {
  static std::shared_ptr<djf::AudioHost> host = std::make_shared<djf::AudioHost>();
  return host;
}
djf::Engine& engine() { return hostPtr()->engine(); }

using TrackPtr = std::shared_ptr<const djf::Track>;

}  // namespace

extern "C" {

#define JNI(ret, name) JNIEXPORT ret JNICALL Java_app_djfusion_audio_NativeEngine_##name

// ---------------------------------------------------------------- saída de áudio
JNI(jboolean, nativeStart)(JNIEnv*, jclass) { return hostPtr()->start() ? JNI_TRUE : JNI_FALSE; }

JNI(void, nativeStop)(JNIEnv*, jclass) { hostPtr()->stop(); }

JNI(void, nativeStreamInfo)(JNIEnv* env, jclass, jintArray out) {
  int32_t v[4];
  hostPtr()->info(v);
  env->SetIntArrayRegion(out, 0, 4, reinterpret_cast<const jint*>(v));
}

// ------------------------------------------------------ construção de faixas
JNI(jlong, nativeBeginTrack)(JNIEnv*, jclass, jint srcRate, jlong maxFrames) {
  if (srcRate < 8000 || srcRate > 384000) return 0;
  return reinterpret_cast<jlong>(new djf::TrackBuilder(srcRate, djf::kEngineRate, maxFrames));
}

JNI(void, nativeAppend)(JNIEnv* env, jclass, jlong handle, jobject buffer, jint offset, jint size,
                        jint channels, jboolean isFloat) {
  auto* b = reinterpret_cast<djf::TrackBuilder*>(handle);
  if (!b || channels < 1 || size <= 0) return;
  auto* base = static_cast<const uint8_t*>(env->GetDirectBufferAddress(buffer));
  if (!base) return;
  const uint8_t* p = base + offset;
  try {
  if (isFloat) {
    const size_t frames = static_cast<size_t>(size) / (sizeof(float) * channels);
    static thread_local std::vector<float> tmp;
    tmp.resize(frames * channels);
    std::memcpy(tmp.data(), p, tmp.size() * sizeof(float));  // cópia evita leitura desalinhada
    b->appendFloat(tmp.data(), frames, channels);
  } else {
    const size_t frames = static_cast<size_t>(size) / (sizeof(int16_t) * channels);
    static thread_local std::vector<int16_t> tmp;
    tmp.resize(frames * channels);
    std::memcpy(tmp.data(), p, tmp.size() * sizeof(int16_t));
    b->appendPcm16(tmp.data(), frames, channels);
  }
  } catch (...) {
    b->markFailed();  // sem memória: nunca deixar uma exceção C++ atravessar o JNI
  }
}

JNI(jboolean, nativeBuilderTooLong)(JNIEnv*, jclass, jlong handle) {
  auto* b = reinterpret_cast<djf::TrackBuilder*>(handle);
  return (b && b->tooLong()) ? JNI_TRUE : JNI_FALSE;
}

// 0 = faixa vazia, -1 = passou do limite, senão um identificador da faixa pronta.
JNI(jlong, nativeFinishTrack)(JNIEnv*, jclass, jlong handle) {
  std::unique_ptr<djf::TrackBuilder> b(reinterpret_cast<djf::TrackBuilder*>(handle));
  if (!b) return 0;
  try {
    std::shared_ptr<djf::Track> t = b->finish();
    if (b->tooLong()) return -1;
    if (t->frames <= 0) return 0;
    return reinterpret_cast<jlong>(new TrackPtr(std::move(t)));
  } catch (...) {
    return -1;
  }
}

JNI(void, nativeAbortTrack)(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<djf::TrackBuilder*>(handle);
}

JNI(jlong, nativeTrackFrames)(JNIEnv*, jclass, jlong track) {
  auto* t = reinterpret_cast<TrackPtr*>(track);
  return t ? (*t)->frames : 0;
}

JNI(jbyteArray, nativeTrackPeaks)(JNIEnv* env, jclass, jlong track) {
  auto* t = reinterpret_cast<TrackPtr*>(track);
  const size_t n = t ? (*t)->peaks.size() : 0;
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(n));
  if (arr && n > 0) {
    env->SetByteArrayRegion(arr, 0, static_cast<jsize>(n), reinterpret_cast<const jbyte*>((*t)->peaks.data()));
  }
  return arr;
}

// Solta a referência do lado Kotlin. O motor mantém a sua própria, se a faixa estiver carregada.
JNI(void, nativeFreeTrack)(JNIEnv*, jclass, jlong track) { delete reinterpret_cast<TrackPtr*>(track); }

// -------------------------------------------------------------- controle dos decks
JNI(jboolean, nativeLoad)(JNIEnv*, jclass, jint deck, jlong track) {
  auto* t = reinterpret_cast<TrackPtr*>(track);
  return (t && engine().loadTrack(deck, *t)) ? JNI_TRUE : JNI_FALSE;
}
JNI(void, nativeUnload)(JNIEnv*, jclass, jint deck) { engine().unload(deck); }
JNI(void, nativePlay)(JNIEnv*, jclass, jint deck) { engine().play(deck); }
JNI(void, nativePause)(JNIEnv*, jclass, jint deck) { engine().pause(deck); }
JNI(void, nativeCue)(JNIEnv*, jclass, jint deck) { engine().cue(deck); }
JNI(void, nativeSeek)(JNIEnv*, jclass, jint deck, jlong frame) { engine().seekFrames(deck, frame); }
JNI(void, nativeSetVolume)(JNIEnv*, jclass, jint deck, jfloat v) { engine().setVolume(deck, v); }
JNI(void, nativeSetCrossfader)(JNIEnv*, jclass, jfloat x) { engine().setCrossfader(x); }
JNI(void, nativeSetMaster)(JNIEnv*, jclass, jfloat v) { engine().setMaster(v); }

// out: [bit0 carregada | bit1 tocando, posição, cue, tamanho], tudo em quadros
JNI(void, nativeDeckState)(JNIEnv* env, jclass, jint deck, jlongArray out) {
  const djf::Engine::DeckInfo info = engine().deckInfo(deck);
  jlong v[4] = {(info.loaded ? 1 : 0) | (info.playing ? 2 : 0), info.positionFrames, info.cueFrames,
                info.lengthFrames};
  env->SetLongArrayRegion(out, 0, 4, v);
}
JNI(jfloat, nativeDeckPeak)(JNIEnv*, jclass, jint deck) { return engine().deckInfo(deck).peak; }
JNI(jfloat, nativeMasterPeak)(JNIEnv*, jclass) { return engine().masterPeak(); }
JNI(void, nativeCollectGarbage)(JNIEnv*, jclass) { engine().collectGarbage(); }

}  // extern "C"
