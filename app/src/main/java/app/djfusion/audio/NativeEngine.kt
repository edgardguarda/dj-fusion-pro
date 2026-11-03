package app.djfusion.audio

import java.nio.ByteBuffer

/**
 * Ponte para o motor C++ (libdjfusion.so). Os nomes e tipos daqui precisam ser idênticos aos de
 * app/src/main/cpp/native_bridge.cpp. O teste tests/jni/JniSmoke.java confere essa ponte.
 */
object NativeEngine {
    init {
        System.loadLibrary("djfusion")
    }

    /** Todas as faixas são decodificadas nesta taxa. */
    const val SAMPLE_RATE = 48_000
    const val PEAKS_PER_SECOND = 150
    const val MAX_TRACK_SECONDS = 15 * 60

    // saída de áudio
    @JvmStatic external fun nativeStart(): Boolean
    @JvmStatic external fun nativeStop()
    /** [taxa real, quadros por rajada, tamanho do buffer em quadros, 1 se exclusivo] */
    @JvmStatic external fun nativeStreamInfo(out: IntArray)

    // construção de faixas
    @JvmStatic external fun nativeBeginTrack(srcRate: Int, maxFrames: Long): Long
    @JvmStatic external fun nativeAppend(
        handle: Long, buffer: ByteBuffer, offset: Int, size: Int, channels: Int, isFloat: Boolean,
    )
    @JvmStatic external fun nativeBuilderTooLong(handle: Long): Boolean
    /** 0 = vazia, -1 = longa demais, senão o identificador da faixa pronta. */
    @JvmStatic external fun nativeFinishTrack(handle: Long): Long
    @JvmStatic external fun nativeAbortTrack(handle: Long)
    @JvmStatic external fun nativeTrackFrames(track: Long): Long
    @JvmStatic external fun nativeTrackPeaks(track: Long): ByteArray
    @JvmStatic external fun nativeFreeTrack(track: Long)

    // decks
    @JvmStatic external fun nativeLoad(deck: Int, track: Long): Boolean
    @JvmStatic external fun nativeUnload(deck: Int)
    @JvmStatic external fun nativePlay(deck: Int)
    @JvmStatic external fun nativePause(deck: Int)
    @JvmStatic external fun nativeCue(deck: Int)
    @JvmStatic external fun nativeSeek(deck: Int, frame: Long)
    @JvmStatic external fun nativeSetVolume(deck: Int, volume: Float)
    @JvmStatic external fun nativeSetCrossfader(x: Float)
    @JvmStatic external fun nativeSetMaster(volume: Float)

    /** [bit0 carregada | bit1 tocando, posição, cue, tamanho], em quadros. */
    @JvmStatic external fun nativeDeckState(deck: Int, out: LongArray)
    @JvmStatic external fun nativeDeckPeak(deck: Int): Float
    @JvmStatic external fun nativeMasterPeak(): Float
    @JvmStatic external fun nativeCollectGarbage()
}
