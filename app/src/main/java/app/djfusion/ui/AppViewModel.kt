package app.djfusion.ui

import android.app.Application
import android.content.Intent
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.provider.OpenableColumns
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.djfusion.audio.DecodeResult
import app.djfusion.audio.NativeEngine
import app.djfusion.audio.TrackDecoder
import app.djfusion.data.LibraryDb
import app.djfusion.data.SortOrder
import app.djfusion.data.TrackRow
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Parte do deck que muda pouco (carregar, tocar, parar). */
data class DeckUi(
    val trackId: Long = -1,
    val title: String = "",
    val artist: String = "",
    val loaded: Boolean = false,
    val playing: Boolean = false,
    val loading: Boolean = false,
    val loadProgress: Float = 0f,
    val lengthFrames: Long = 0,
    val wave: ByteArray? = null,
)

/** Parte que muda ~30 vezes por segundo (posição e nível). Fica separada para não redesenhar tudo. */
data class DeckLive(
    val positionFrames: Long = 0,
    val cueFrames: Long = 0,
    val peak: Float = 0f,
)

data class LiveUi(
    val decks: List<DeckLive> = listOf(DeckLive(), DeckLive()),
    val masterPeak: Float = 0f,
)

class AppViewModel(app: Application) : AndroidViewModel(app) {
    private val db = LibraryDb(app)

    private val _decks = MutableStateFlow(listOf(DeckUi(), DeckUi()))
    val decks: StateFlow<List<DeckUi>> = _decks.asStateFlow()

    private val _live = MutableStateFlow(LiveUi())
    val live: StateFlow<LiveUi> = _live.asStateFlow()

    private val _volumes = MutableStateFlow(listOf(1f, 1f))
    val volumes: StateFlow<List<Float>> = _volumes.asStateFlow()
    private val _crossfader = MutableStateFlow(0.5f)
    val crossfader: StateFlow<Float> = _crossfader.asStateFlow()
    private val _master = MutableStateFlow(0.8f)
    val master: StateFlow<Float> = _master.asStateFlow()

    private val _message = MutableStateFlow<String?>(null)
    val message: StateFlow<String?> = _message.asStateFlow()

    private val _audioStatus = MutableStateFlow("Áudio desligado")
    val audioStatus: StateFlow<String> = _audioStatus.asStateFlow()

    val search = MutableStateFlow("")
    val sort = MutableStateFlow(SortOrder.RECENT)
    private val libraryVersion = MutableStateFlow(0)

    val tracks: StateFlow<List<TrackRow>> =
        combine(search, sort, libraryVersion) { s, o, _ -> s to o }
            .map { (s, o) -> withContext(Dispatchers.IO) { db.query(s, o) } }
            .stateIn(viewModelScope, SharingStarted.Eagerly, emptyList())

    private val decodeJobs = arrayOfNulls<Job>(2)

    private var foreground = true
    private var stopJob: Job? = null

    init {
        // O motor C++ vive no processo, não na Activity: se a tela foi fechada e reaberta sem o processo
        // morrer, ele ainda guarda as faixas e o estado de tocar da vez anterior. Começamos do zero.
        NativeEngine.nativeUnload(0)
        NativeEngine.nativeUnload(1)
        startPolling()
    }

    /**
     * Chamado pela Activity em onStart (true) e onStop (false). Sem serviço em primeiro plano o Android
     * não garante áudio em segundo plano, então ao sair o app pausa os decks e fecha a saída de áudio.
     */
    fun onForeground(on: Boolean) {
        foreground = on
        stopJob?.cancel()
        if (on) {
            startAudio()
        } else {
            NativeEngine.nativePause(0)
            NativeEngine.nativePause(1)
            stopJob = viewModelScope.launch(Dispatchers.Default) {
                delay(150) // deixa a rampa de 4 ms da pausa terminar antes de fechar o fluxo
                if (!foreground) NativeEngine.nativeStop()
            }
        }
    }

    private fun startAudio() {
        // Abrir o fluxo exclusivo do Oboe pode demorar: fora da thread principal.
        viewModelScope.launch(Dispatchers.Default) {
            val ok = NativeEngine.nativeStart()
            if (ok) {
                NativeEngine.nativeSetMaster(_master.value)
                NativeEngine.nativeSetCrossfader(_crossfader.value)
                NativeEngine.nativeSetVolume(0, _volumes.value[0])
                NativeEngine.nativeSetVolume(1, _volumes.value[1])
                val info = IntArray(4)
                NativeEngine.nativeStreamInfo(info)
                val bufferMs = if (info[0] > 0) info[2] * 1000f / info[0] else 0f
                _audioStatus.value = "%d Hz · buffer %.0f ms".format(java.util.Locale.ROOT, info[0], bufferMs)
            } else {
                _audioStatus.value = "Sem áudio"
                say("Não consegui abrir a saída de áudio do aparelho.")
            }
        }
    }

    private fun startPolling() {
        viewModelScope.launch {
            val buf = LongArray(4)
            var tick = 0
            while (isActive) {
                if (!foreground) {
                    delay(250)
                    continue
                }
                val states = ArrayList<LongArray>(2)
                val lives = ArrayList<DeckLive>(2)
                for (d in 0..1) {
                    NativeEngine.nativeDeckState(d, buf)
                    states.add(buf.copyOf())
                    lives.add(DeckLive(buf[1], buf[2], NativeEngine.nativeDeckPeak(d)))
                }
                _live.value = LiveUi(lives, NativeEngine.nativeMasterPeak())
                _decks.update { list ->
                    list.mapIndexed { i, d ->
                        val flags = states[i][0]
                        val playing = (flags and 2L) != 0L
                        val loaded = (flags and 1L) != 0L
                        if (d.playing == playing && d.loaded == loaded) d else d.copy(playing = playing, loaded = loaded)
                    }
                }
                if (++tick % 30 == 0) launch(Dispatchers.Default) { NativeEngine.nativeCollectGarbage() }
                delay(33)
            }
        }
    }

    // ----------------------------------------------------------------- mensagens

    fun say(text: String) {
        _message.value = text
    }

    fun clearMessage(text: String) {
        _message.update { if (it == text) null else it }
    }

    // ------------------------------------------------------------------- decks

    fun togglePlay(deck: Int) {
        if (_decks.value[deck].loading) {
            say("Espere a música terminar de carregar no deck ${deckName(deck)}.")
            return
        }
        val st = LongArray(4)
        NativeEngine.nativeDeckState(deck, st)
        if ((st[0] and 1L) == 0L) {
            say("Deck ${deckName(deck)} sem música. Abra a Biblioteca e toque em ${deckName(deck)}.")
            return
        }
        if ((st[0] and 2L) != 0L) NativeEngine.nativePause(deck) else NativeEngine.nativePlay(deck)
    }

    fun cue(deck: Int) = NativeEngine.nativeCue(deck)

    fun seekFraction(deck: Int, fraction: Float) {
        val length = _decks.value[deck].lengthFrames
        if (length <= 0) return
        NativeEngine.nativeSeek(deck, (fraction.coerceIn(0f, 1f) * length).toLong())
    }

    fun setVolume(deck: Int, v: Float) {
        _volumes.update { list -> list.mapIndexed { i, old -> if (i == deck) v else old } }
        NativeEngine.nativeSetVolume(deck, v)
    }

    fun setCrossfader(x: Float) {
        _crossfader.value = x
        NativeEngine.nativeSetCrossfader(x)
    }

    fun setMaster(v: Float) {
        _master.value = v
        NativeEngine.nativeSetMaster(v)
    }

    // ------------------------------------------------------------ carregar faixa

    fun loadTrack(deck: Int, row: TrackRow) {
        val current = _decks.value[deck]
        if (current.playing) {
            say("Pause o deck ${deckName(deck)} antes de carregar outra música.")
            return
        }
        decodeJobs[deck]?.cancel()
        setDeck(deck) { it.copy(loading = true, loadProgress = 0f, title = row.title, artist = row.artist) }
        say("Carregando no deck ${deckName(deck)}…")
        decodeJobs[deck] = viewModelScope.launch {
            val result = TrackDecoder.decode(getApplication(), Uri.parse(row.uri)) { p ->
                setDeck(deck) { it.copy(loadProgress = p) }
            }
            when (result) {
                is DecodeResult.Ready -> {
                    val frames = NativeEngine.nativeTrackFrames(result.trackHandle)
                    val wave = NativeEngine.nativeTrackPeaks(result.trackHandle)
                    val ok = NativeEngine.nativeLoad(deck, result.trackHandle)
                    NativeEngine.nativeFreeTrack(result.trackHandle)
                    if (ok) {
                        setDeck(deck) {
                            it.copy(
                                trackId = row.id, title = row.title, artist = row.artist, loading = false,
                                loadProgress = 1f, lengthFrames = frames, wave = wave,
                            )
                        }
                        say("${row.title} pronta no deck ${deckName(deck)}.")
                    } else {
                        setDeck(deck) { it.copy(loading = false, title = current.title, artist = current.artist) }
                        say("O motor de áudio recusou a faixa.")
                    }
                }
                is DecodeResult.Failed -> {
                    setDeck(deck) { it.copy(loading = false, title = current.title, artist = current.artist) }
                    say(result.reason)
                }
            }
        }
    }

    private fun setDeck(deck: Int, change: (DeckUi) -> DeckUi) {
        _decks.update { list -> list.mapIndexed { i, d -> if (i == deck) change(d) else d } }
    }

    // ---------------------------------------------------------------- biblioteca

    fun setSearch(text: String) {
        search.value = text
    }

    fun setSort(order: SortOrder) {
        sort.value = order
    }

    fun removeFromLibrary(row: TrackRow) {
        viewModelScope.launch(Dispatchers.IO) {
            db.delete(row.id)
            libraryVersion.update { it + 1 }
        }
    }

    fun importUris(uris: List<Uri>) {
        if (uris.isEmpty()) return
        viewModelScope.launch(Dispatchers.IO) {
            var added = 0
            var duplicated = 0
            var invalid = 0
            for (uri in uris) {
                try {
                    getApplication<Application>().contentResolver
                        .takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                } catch (_: Exception) {
                    // nem todo provedor oferece permissão persistente
                }
                val meta = readMetadata(uri)
                if (meta == null) {
                    invalid++
                } else if (db.insert(uri.toString(), meta.title, meta.artist, meta.durationMs)) {
                    added++
                } else {
                    duplicated++
                }
            }
            libraryVersion.update { it + 1 }
            val parts = ArrayList<String>()
            if (added > 0) parts.add("$added adicionada(s)")
            if (duplicated > 0) parts.add("$duplicated já estava(m) na biblioteca")
            if (invalid > 0) parts.add("$invalid não é/são áudio")
            say(parts.joinToString(" · "))
        }
    }

    private class Meta(val title: String, val artist: String, val durationMs: Long)

    private fun readMetadata(uri: Uri): Meta? {
        val ctx = getApplication<Application>()
        val r = MediaMetadataRetriever()
        try {
            r.setDataSource(ctx, uri)
            if (r.extractMetadata(MediaMetadataRetriever.METADATA_KEY_HAS_AUDIO) != "yes") return null
            val duration = r.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)?.toLongOrNull() ?: 0L
            val title = r.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE)?.trim().orEmpty()
            val artist = r.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST)?.trim().orEmpty()
            return Meta(title.ifEmpty { displayName(uri) }, artist, duration)
        } catch (_: Exception) {
            return null
        } finally {
            try {
                r.release()
            } catch (_: Exception) {
            }
        }
    }

    private fun displayName(uri: Uri): String {
        try {
            getApplication<Application>().contentResolver
                .query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { c ->
                    if (c.moveToFirst()) {
                        val name = c.getString(0)
                        if (!name.isNullOrBlank()) return name.substringBeforeLast('.')
                    }
                }
        } catch (_: Exception) {
        }
        return "Faixa sem título"
    }

    override fun onCleared() {
        decodeJobs.forEach { it?.cancel() }
        NativeEngine.nativeStop()
    }

    companion object {
        fun deckName(deck: Int) = if (deck == 0) "A" else "B"
    }
}
