package app.djfusion.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import app.djfusion.ui.theme.Metal
import kotlin.math.min

internal fun deckColor(deck: Int): Color = if (deck == 0) Metal.DeckA else Metal.DeckB
private fun onDeckColor(deck: Int): Color = if (deck == 0) Metal.OnDeckA else Metal.OnDeckB

/**
 * Tela DJ Live da v1. Só aparece o que funciona: play, cue, volume, crossfader, master, busca pela
 * onda, vinil e medidores. EQ, pitch, sync, hot cues, loops e efeitos entram nos marcos seguintes.
 */
@Composable
fun DjLiveScreen(vm: AppViewModel) {
    val decks by vm.decks.collectAsStateWithLifecycle()
    Column(Modifier.fillMaxSize().background(Metal.Bg)) {
        WaveRow(vm, 0, decks[0])
        WaveRow(vm, 1, decks[1])
        Row(Modifier.fillMaxWidth().weight(1f)) {
            DeckPanel(vm, 0, decks[0], Modifier.weight(1f).fillMaxHeight())
            MixerPanel(vm, Modifier.width(240.dp).fillMaxHeight())
            DeckPanel(vm, 1, decks[1], Modifier.weight(1f).fillMaxHeight())
        }
    }
}

@Composable
private fun WaveRow(vm: AppViewModel, deck: Int, ui: DeckUi) {
    val live by vm.live.collectAsStateWithLifecycle()
    val l = live.decks[deck]
    val name = AppViewModel.deckName(deck)
    Box(Modifier.fillMaxWidth().height(46.dp)) {
        WaveformStrip(
            peaks = ui.wave,
            positionFrames = l.positionFrames,
            lengthFrames = ui.lengthFrames,
            cueFrames = l.cueFrames,
            deckColor = deckColor(deck),
            description = "Forma de onda do deck $name. Toque para ir a outro ponto da música.",
            onSeek = { vm.seekFraction(deck, it) },
            modifier = Modifier.fillMaxSize(),
        )
        val overlay = when {
            ui.loading -> "Carregando deck $name… ${(ui.loadProgress * 100).toInt()}%"
            !ui.loaded -> "Deck $name vazio"
            else -> null
        }
        if (overlay != null) {
            Text(
                overlay, color = Metal.TextMuted, fontSize = 12.sp, fontWeight = FontWeight.SemiBold,
                modifier = Modifier.align(Alignment.Center),
            )
        } else {
            Text(
                "-" + formatTimeTenths(framesToMs(ui.lengthFrames - l.positionFrames)),
                color = Metal.Text, fontSize = 12.sp, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.Bold,
                modifier = Modifier.align(Alignment.TopEnd).padding(end = 8.dp, top = 2.dp)
                    .background(Color(0xCC0E1013), RoundedCornerShape(4.dp)).padding(horizontal = 4.dp),
            )
        }
    }
}

@Composable
private fun DeckPanel(vm: AppViewModel, deck: Int, ui: DeckUi, modifier: Modifier) {
    val live by vm.live.collectAsStateWithLifecycle()
    val l = live.decks[deck]
    val color = deckColor(deck)
    val name = AppViewModel.deckName(deck)
    Column(modifier.background(Metal.Surface)) {
        Box(Modifier.fillMaxWidth().height(2.dp).background(color))
        Row(
            Modifier.fillMaxWidth().padding(horizontal = 10.dp, vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Box(
                Modifier.size(36.dp).clip(RoundedCornerShape(8.dp)).background(color),
                contentAlignment = Alignment.Center,
            ) {
                Text(name, color = onDeckColor(deck), fontSize = 20.sp, fontWeight = FontWeight.ExtraBold)
            }
            Column(Modifier.weight(1f).padding(horizontal = 10.dp)) {
                Text(
                    if (ui.title.isNotEmpty()) ui.title else "Sem música",
                    color = Metal.Text, fontSize = 14.sp, fontWeight = FontWeight.Bold, maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                Text(
                    if (ui.loaded || ui.loading) ui.artist else "Abra a Biblioteca e toque em $name",
                    color = Metal.TextMuted, fontSize = 12.sp, maxLines = 1, overflow = TextOverflow.Ellipsis,
                )
            }
            Text(
                if (ui.loaded) formatTimeTenths(framesToMs(l.positionFrames)) else "--:--.-",
                color = color, fontSize = 20.sp, fontFamily = FontFamily.Monospace, fontWeight = FontWeight.Bold,
            )
        }
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth().padding(horizontal = 10.dp, vertical = 6.dp)) {
            val buttonsWidth = 84.dp
            val side = min(maxHeight.value, maxWidth.value - buttonsWidth.value - 12f).coerceAtLeast(60f).dp
            Row(Modifier.fillMaxSize(), verticalAlignment = Alignment.CenterVertically) {
                val platter = @Composable {
                    Platter(
                        deckColor = color,
                        positionFrames = l.positionFrames,
                        lengthFrames = ui.lengthFrames,
                        armOnRight = deck == 1,
                        modifier = Modifier.size(side),
                    )
                }
                val controls = @Composable {
                    Column(
                        Modifier.width(buttonsWidth),
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                    ) {
                        MetalButton(
                            description = if (ui.playing) "Pausar deck $name" else "Tocar deck $name",
                            onPress = { vm.togglePlay(deck) },
                            enabled = ui.loaded && !ui.loading,
                            led = color, ledOn = ui.playing,
                            modifier = Modifier.fillMaxWidth().height(64.dp),
                        ) {
                            TransportGlyph(ui.playing, Modifier.size(28.dp))
                        }
                        MetalButton(
                            description = "Cue do deck $name. Parado: marca o ponto. Tocando: volta ao ponto e para.",
                            onPress = { vm.cue(deck) },
                            enabled = ui.loaded && !ui.loading,
                            modifier = Modifier.fillMaxWidth().height(56.dp),
                        ) {
                            Text("CUE", color = Metal.OnMetal, fontSize = 16.sp, fontWeight = FontWeight.ExtraBold)
                        }
                    }
                }
                if (deck == 0) {
                    platter()
                    Spacer(Modifier.width(12.dp))
                    controls()
                } else {
                    controls()
                    Spacer(Modifier.width(12.dp))
                    platter()
                }
            }
        }
    }
}

@Composable
private fun MixerPanel(vm: AppViewModel, modifier: Modifier) {
    val volumes by vm.volumes.collectAsStateWithLifecycle()
    val cross by vm.crossfader.collectAsStateWithLifecycle()
    val master by vm.master.collectAsStateWithLifecycle()
    val live by vm.live.collectAsStateWithLifecycle()
    val darkHandle = Color(0xFF14171B)
    Column(
        modifier.background(Color(0xFF13161A)).padding(horizontal = 10.dp, vertical = 6.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Text("MASTER", color = Metal.TextMuted, fontSize = 11.sp, fontWeight = FontWeight.Bold, letterSpacing = 2.sp)
        MetalFader(
            value = master, onChange = vm::setMaster, accent = darkHandle, description = "Volume master",
            modifier = Modifier.fillMaxWidth().height(44.dp), vertical = false,
        )
        LevelMeter(live.masterPeak, Modifier.fillMaxWidth().height(8.dp), vertical = false, segments = 24)
        Row(
            Modifier.weight(1f).fillMaxWidth().padding(vertical = 8.dp),
            horizontalArrangement = Arrangement.SpaceEvenly,
        ) {
            ChannelStrip(vm, 0, volumes[0], live.decks[0].peak)
            ChannelStrip(vm, 1, volumes[1], live.decks[1].peak)
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text("A", color = Metal.DeckA, fontSize = 13.sp, fontWeight = FontWeight.ExtraBold)
            Text("CROSSFADER", color = Metal.TextMuted, fontSize = 10.sp, fontWeight = FontWeight.Bold, letterSpacing = 1.sp, textAlign = TextAlign.Center)
            Text("B", color = Metal.DeckB, fontSize = 13.sp, fontWeight = FontWeight.ExtraBold)
        }
        MetalFader(
            value = cross, onChange = vm::setCrossfader, accent = darkHandle,
            description = "Crossfader. Esquerda toca só o deck A, direita só o deck B.",
            modifier = Modifier.fillMaxWidth().height(52.dp), vertical = false,
        )
    }
}

@Composable
private fun ChannelStrip(vm: AppViewModel, deck: Int, volume: Float, peak: Float) {
    val name = AppViewModel.deckName(deck)
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Row(Modifier.weight(1f), horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            MetalFader(
                value = volume, onChange = { vm.setVolume(deck, it) }, accent = deckColor(deck),
                description = "Volume do deck $name", modifier = Modifier.width(56.dp).fillMaxHeight(),
            )
            LevelMeter(peak, Modifier.width(12.dp).fillMaxHeight().padding(vertical = 6.dp))
        }
        Text(name, color = deckColor(deck), fontSize = 14.sp, fontWeight = FontWeight.ExtraBold)
    }
}
