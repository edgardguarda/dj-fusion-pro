package app.djfusion.audio

import android.content.Context
import android.media.AudioFormat
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.net.Uri
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.withContext

sealed interface DecodeResult {
    /** [trackHandle] precisa ser liberado com NativeEngine.nativeFreeTrack depois de carregar no deck. */
    data class Ready(val trackHandle: Long) : DecodeResult

    data class Failed(val reason: String) : DecodeResult
}

/** Decodifica um arquivo de áudio com os decodificadores do próprio Android (MP3, AAC, FLAC, WAV, OGG...). */
object TrackDecoder {
    private const val TIMEOUT_US = 10_000L

    suspend fun decode(context: Context, uri: Uri, onProgress: (Float) -> Unit): DecodeResult {
        // Se o job for cancelado depois de a faixa ficar pronta, withContext descarta o resultado.
        // Guardamos o identificador aqui para liberar a memória nativa nesse caso.
        var pending = 0L
        try {
        return withContext(Dispatchers.Default) {
            val extractor = MediaExtractor()
            var codec: MediaCodec? = null
            var builder = 0L
            try {
                extractor.setDataSource(context, uri, null)
                var audioIndex = -1
                for (i in 0 until extractor.trackCount) {
                    val mime = extractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME)
                    if (mime != null && mime.startsWith("audio/")) {
                        audioIndex = i
                        break
                    }
                }
                if (audioIndex < 0) return@withContext DecodeResult.Failed("Este arquivo não tem áudio.")

                extractor.selectTrack(audioIndex)
                val inFormat = extractor.getTrackFormat(audioIndex)
                val mime = inFormat.getString(MediaFormat.KEY_MIME) ?: return@withContext DecodeResult.Failed(
                    "Formato de áudio desconhecido.",
                )
                val durationUs =
                    if (inFormat.containsKey(MediaFormat.KEY_DURATION)) inFormat.getLong(MediaFormat.KEY_DURATION) else 0L
                if (durationUs > NativeEngine.MAX_TRACK_SECONDS * 1_000_000L) {
                    return@withContext DecodeResult.Failed("Faixa maior que 15 minutos. O limite da v1 é 15 minutos.")
                }

                val c = MediaCodec.createDecoderByType(mime)
                codec = c
                c.configure(inFormat, null, null, 0)
                c.start()

                val info = MediaCodec.BufferInfo()
                var inputDone = false
                var outputDone = false
                var channels = 2
                var floatOutput = false

                while (!outputDone) {
                    ensureActive()

                    if (!inputDone) {
                        val inIndex = c.dequeueInputBuffer(TIMEOUT_US)
                        if (inIndex >= 0) {
                            val inBuf = c.getInputBuffer(inIndex)!!
                            val n = extractor.readSampleData(inBuf, 0)
                            if (n < 0) {
                                c.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                                inputDone = true
                            } else {
                                c.queueInputBuffer(inIndex, 0, n, extractor.sampleTime, 0)
                                extractor.advance()
                            }
                        }
                    }

                    val outIndex = c.dequeueOutputBuffer(info, TIMEOUT_US)
                    if (outIndex >= 0) {
                        if (info.size > 0) {
                            if (builder == 0L) {
                                // O formato de saída só é confiável depois do primeiro bloco decodificado
                                // (AAC com SBR, por exemplo, sai com o dobro da taxa de entrada).
                                val of = c.outputFormat
                                val rate = of.getInteger(MediaFormat.KEY_SAMPLE_RATE)
                                channels = of.getInteger(MediaFormat.KEY_CHANNEL_COUNT)
                                val encoding = if (of.containsKey(MediaFormat.KEY_PCM_ENCODING)) {
                                    of.getInteger(MediaFormat.KEY_PCM_ENCODING)
                                } else {
                                    AudioFormat.ENCODING_PCM_16BIT
                                }
                                if (encoding != AudioFormat.ENCODING_PCM_16BIT && encoding != AudioFormat.ENCODING_PCM_FLOAT) {
                                    c.releaseOutputBuffer(outIndex, false)
                                    return@withContext DecodeResult.Failed("Formato de áudio não suportado.")
                                }
                                floatOutput = encoding == AudioFormat.ENCODING_PCM_FLOAT
                                builder = NativeEngine.nativeBeginTrack(
                                    rate,
                                    NativeEngine.SAMPLE_RATE.toLong() * NativeEngine.MAX_TRACK_SECONDS,
                                )
                                if (builder == 0L) {
                                    c.releaseOutputBuffer(outIndex, false)
                                    return@withContext DecodeResult.Failed("Taxa de amostragem não suportada.")
                                }
                            }
                            val outBuf = c.getOutputBuffer(outIndex)!!
                            NativeEngine.nativeAppend(builder, outBuf, info.offset, info.size, channels, floatOutput)
                            if (durationUs > 0) {
                                onProgress((info.presentationTimeUs.toFloat() / durationUs).coerceIn(0f, 1f))
                            }
                        }
                        c.releaseOutputBuffer(outIndex, false)
                        if ((info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) outputDone = true
                    }
                    if (builder != 0L && NativeEngine.nativeBuilderTooLong(builder)) {
                        return@withContext DecodeResult.Failed("Faixa maior que 15 minutos. O limite da v1 é 15 minutos.")
                    }
                }

                if (builder == 0L) return@withContext DecodeResult.Failed("Não encontrei áudio para tocar neste arquivo.")
                val handle = NativeEngine.nativeFinishTrack(builder)
                builder = 0L
                when {
                    handle == -1L -> DecodeResult.Failed("Faixa maior que 15 minutos. O limite da v1 é 15 minutos.")
                    handle <= 0L -> DecodeResult.Failed("O arquivo não tem áudio audível.")
                    else -> {
                        pending = handle
                        DecodeResult.Ready(handle)
                    }
                }
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                DecodeResult.Failed("Não consegui ler este arquivo. Ele pode ter sido movido ou ter formato não suportado.")
            } finally {
                if (builder != 0L) NativeEngine.nativeAbortTrack(builder)
                try {
                    codec?.stop()
                } catch (_: Exception) {
                }
                codec?.release()
                extractor.release()
            }
        }
        } catch (e: CancellationException) {
            if (pending != 0L) NativeEngine.nativeFreeTrack(pending)
            throw e
        }
    }
}
