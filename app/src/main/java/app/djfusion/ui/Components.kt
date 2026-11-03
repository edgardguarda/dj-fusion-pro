package app.djfusion.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.semantics.ProgressBarRangeInfo
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.progressBarRangeInfo
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.setProgress
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.dp
import app.djfusion.ui.theme.Metal
import kotlin.math.PI
import kotlin.math.ceil
import kotlin.math.cos
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sin
import kotlin.math.sqrt

// ------------------------------------------------------------------ metal

/** Placa de alumínio escovado: degradê vertical mais fios finos horizontais. */
internal fun DrawScope.drawMetalPlate(pressed: Boolean, enabled: Boolean, corner: Float) {
    val top = if (pressed) Metal.MetalMid else Metal.MetalLight
    val mid = if (pressed) Metal.MetalDark else Metal.MetalMid
    val bottom = if (pressed) Metal.MetalDark else Color(0xFF8F979F)
    drawRoundRect(
        brush = Brush.verticalGradient(listOf(top, mid, bottom)),
        cornerRadius = CornerRadius(corner),
        alpha = if (enabled) 1f else 0.35f,
    )
    val step = 3.dp.toPx()
    var y = 0f
    var flip = false
    while (y < size.height) {
        drawLine(
            color = if (flip) Color.Black.copy(alpha = 0.07f) else Color.White.copy(alpha = 0.10f),
            start = Offset(0f, y), end = Offset(size.width, y), strokeWidth = 1f,
        )
        flip = !flip
        y += step
    }
}

/**
 * Botão metálico. Por padrão dispara no instante do toque (não ao soltar), como um botão de equipamento de DJ.
 * Com [fireOnPress] falso só dispara no toque completo, para não agir quando o dedo começa uma rolagem.
 * [led] acende um ponto na cor do deck quando [ledOn] é verdadeiro.
 */
@Composable
fun MetalButton(
    description: String,
    onPress: () -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    led: Color? = null,
    ledOn: Boolean = false,
    fireOnPress: Boolean = true,
    content: @Composable BoxScope.() -> Unit,
) {
    var pressed by remember { mutableStateOf(false) }
    val latest = rememberUpdatedState(onPress)
    val shape = RoundedCornerShape(10.dp)
    Box(
        modifier = modifier
            .semantics {
                contentDescription = description
                role = Role.Button
            }
            .clip(shape)
            .drawBehind { drawMetalPlate(pressed, enabled, 10.dp.toPx()) }
            .pointerInput(enabled, fireOnPress) {
                detectTapGestures(
                    onPress = {
                        if (enabled) {
                            pressed = true
                            if (fireOnPress) latest.value()
                            tryAwaitRelease()
                            pressed = false
                        }
                    },
                    onTap = { if (enabled && !fireOnPress) latest.value() },
                )
            },
        contentAlignment = Alignment.Center,
    ) {
        content()
        if (led != null) {
            Box(
                Modifier
                    .align(Alignment.TopEnd)
                    .padding(6.dp)
                    .size(8.dp)
                    .background(if (ledOn) led else Color(0xFF4A525A), CircleShape),
            )
        }
    }
}

/** Triângulo de play ou duas barras de pausa, na cor de contraste do metal. */
@Composable
fun TransportGlyph(playing: Boolean, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val c = Metal.OnMetal
        val w = size.width
        val h = size.height
        if (playing) {
            drawRoundRect(c, Offset(w * 0.22f, h * 0.15f), Size(w * 0.20f, h * 0.70f), CornerRadius(w * 0.03f))
            drawRoundRect(c, Offset(w * 0.58f, h * 0.15f), Size(w * 0.20f, h * 0.70f), CornerRadius(w * 0.03f))
        } else {
            val p = Path().apply {
                moveTo(w * 0.25f, h * 0.12f)
                lineTo(w * 0.25f, h * 0.88f)
                lineTo(w * 0.85f, h * 0.50f)
                close()
            }
            drawPath(p, c)
        }
    }
}

// ----------------------------------------------------------------- faders

/**
 * Fader de alumínio. Vertical: 0 embaixo, 1 em cima. Horizontal: 0 à esquerda, 1 à direita.
 * Responde ao toque no ponto exato e ao arrasto. Também aceita ajuste por leitor de tela.
 */
@Composable
fun MetalFader(
    value: Float,
    onChange: (Float) -> Unit,
    accent: Color,
    description: String,
    modifier: Modifier = Modifier,
    vertical: Boolean = true,
) {
    val latest = rememberUpdatedState(onChange)
    Canvas(
        modifier
            .semantics {
                contentDescription = description
                stateDescription = "${(value * 100).roundToInt()} por cento"
                progressBarRangeInfo = ProgressBarRangeInfo(value, 0f..1f)
                setProgress { v ->
                    latest.value(v.coerceIn(0f, 1f))
                    true
                }
            }
            .pointerInput(vertical) {
                awaitEachGesture {
                    val handleLen = (if (vertical) 34.dp else 48.dp).toPx()
                    fun toValue(p: Offset): Float = if (vertical) {
                        1f - ((p.y - handleLen / 2f) / (size.height - handleLen)).coerceIn(0f, 1f)
                    } else {
                        ((p.x - handleLen / 2f) / (size.width - handleLen)).coerceIn(0f, 1f)
                    }
                    val down = awaitFirstDown()
                    down.consume()
                    latest.value(toValue(down.position))
                    do {
                        val event = awaitPointerEvent()
                        val change = event.changes.first()
                        if (change.pressed) {
                            latest.value(toValue(change.position))
                            change.consume()
                        }
                    } while (event.changes.any { it.pressed })
                }
            },
    ) {
        val track = 10.dp.toPx()
        val handleLong = (if (vertical) 34.dp else 48.dp).toPx()
        val handleShort = (if (vertical) 46.dp else 34.dp).toPx()
        val v = value.coerceIn(0f, 1f)
        if (vertical) {
            val x = size.width / 2f
            drawRoundRect(Color(0xFF262B31), Offset(x - track / 2, 0f), Size(track, size.height), CornerRadius(track / 2))
            drawRoundRect(
                Metal.Border, Offset(x - track / 2, 0f), Size(track, size.height), CornerRadius(track / 2),
                style = Stroke(1f),
            )
            for (i in 0..8) {
                val ty = handleLong / 2 + (size.height - handleLong) * i / 8f
                drawLine(Metal.MetalDark, Offset(x - track * 1.6f, ty), Offset(x - track * 0.9f, ty), 1.5f)
                drawLine(Metal.MetalDark, Offset(x + track * 0.9f, ty), Offset(x + track * 1.6f, ty), 1.5f)
            }
            val hy = (1f - v) * (size.height - handleLong)
            drawHandle(Offset(x - handleShort / 2, hy), Size(handleShort, handleLong), accent, vertical = true)
        } else {
            val y = size.height / 2f
            drawRoundRect(Color(0xFF262B31), Offset(0f, y - track / 2), Size(size.width, track), CornerRadius(track / 2))
            drawRoundRect(
                Metal.Border, Offset(0f, y - track / 2), Size(size.width, track), CornerRadius(track / 2),
                style = Stroke(1f),
            )
            val hx = v * (size.width - handleLong)
            drawHandle(Offset(hx, y - handleShort / 2), Size(handleLong, handleShort), accent, vertical = false)
        }
    }
}

private fun DrawScope.drawHandle(topLeft: Offset, size: Size, accent: Color, vertical: Boolean) {
    val corner = 7.dp.toPx()
    drawRoundRect(
        brush = Brush.verticalGradient(
            listOf(Metal.MetalLight, Metal.MetalMid, Color(0xFF8F979F)), startY = topLeft.y, endY = topLeft.y + size.height,
        ),
        topLeft = topLeft, size = size, cornerRadius = CornerRadius(corner),
    )
    var y = topLeft.y + 2f
    while (y < topLeft.y + size.height) {
        drawLine(Color.White.copy(alpha = 0.10f), Offset(topLeft.x, y), Offset(topLeft.x + size.width, y), 1f)
        y += 3.dp.toPx()
    }
    // linha na cor do deck no meio da alça
    if (vertical) {
        val cy = topLeft.y + size.height / 2
        drawLine(accent, Offset(topLeft.x + 6.dp.toPx(), cy), Offset(topLeft.x + size.width - 6.dp.toPx(), cy), 3.dp.toPx())
    } else {
        val cx = topLeft.x + size.width / 2
        drawLine(accent, Offset(cx, topLeft.y + 5.dp.toPx()), Offset(cx, topLeft.y + size.height - 5.dp.toPx()), 3.dp.toPx())
    }
}

// ----------------------------------------------------------------- medidor

/** Medidor de nível de segmentos: escala em decibéis, de -48 dB a 0 dB. */
@Composable
fun LevelMeter(peak: Float, modifier: Modifier = Modifier, vertical: Boolean = true, segments: Int = 12) {
    Canvas(modifier) {
        val db = 20f * log10(max(peak, 0.0001f))
        val lit = ceil(((db + 48f) / 48f).coerceIn(0f, 1f) * segments).toInt()
        val gap = 2.dp.toPx()
        for (i in 0 until segments) {
            val f = (i + 1f) / segments
            val on = i < lit
            val base = when {
                f <= 0.6f -> Metal.Success
                f <= 0.8f -> Metal.Warning
                f <= 0.92f -> Color(0xFFFF8A3D)
                else -> Metal.Danger
            }
            val color = if (on) base else base.copy(alpha = 0.16f)
            if (vertical) {
                val h = (size.height - gap * (segments - 1)) / segments
                val y = size.height - (i + 1) * h - i * gap
                drawRoundRect(color, Offset(0f, y), Size(size.width, h), CornerRadius(2.dp.toPx()))
            } else {
                val w = (size.width - gap * (segments - 1)) / segments
                drawRoundRect(color, Offset(i * (w + gap), 0f), Size(w, size.height), CornerRadius(2.dp.toPx()))
            }
        }
    }
}

// --------------------------------------------------------------- waveform

private fun columns(peaks: ByteArray, width: Int): FloatArray {
    // 3 valores (graves, médios, agudos) por coluna de pixel, de 0 a 1
    val out = FloatArray(width * 3)
    val bins = peaks.size / 3
    if (bins == 0 || width == 0) return out
    for (x in 0 until width) {
        val from = (x.toLong() * bins / width).toInt()
        val to = max(from + 1, ((x + 1).toLong() * bins / width).toInt()).coerceAtMost(bins)
        for (band in 0..2) {
            var m = 0
            for (b in from until to) {
                val v = peaks[b * 3 + band].toInt() and 0xFF
                if (v > m) m = v
            }
            out[x * 3 + band] = sqrt(m / 255f)
        }
    }
    return out
}

/**
 * Onda da faixa inteira, colorida por frequência (graves laranja, médios amarelo, agudos azul claro).
 * A parte já tocada fica mais clara. Toque ou arraste para buscar.
 */
@Composable
fun WaveformStrip(
    peaks: ByteArray?,
    positionFrames: Long,
    lengthFrames: Long,
    cueFrames: Long,
    deckColor: Color,
    description: String,
    onSeek: (Float) -> Unit,
    modifier: Modifier = Modifier,
) {
    val latest = rememberUpdatedState(onSeek)
    BoxWithConstraints(
        modifier
            .background(Color(0xFF0B0D10))
            .semantics { contentDescription = description }
            .pointerInput(Unit) {
                awaitEachGesture {
                    val down = awaitFirstDown()
                    down.consume()
                    latest.value(down.position.x / size.width)
                    do {
                        val event = awaitPointerEvent()
                        val change = event.changes.first()
                        if (change.pressed) {
                            latest.value(change.position.x / size.width)
                            change.consume()
                        }
                    } while (event.changes.any { it.pressed })
                }
            },
    ) {
        val widthPx = with(LocalDensity.current) { maxWidth.roundToPx() }
        val cols = remember(peaks, widthPx) { if (peaks == null) FloatArray(0) else columns(peaks, widthPx) }
        Canvas(Modifier.fillMaxSize()) {
            val w = size.width
            val h = size.height
            val mid = h / 2f
            if (peaks == null || lengthFrames <= 0 || cols.isEmpty()) {
                drawLine(Metal.Border, Offset(0f, mid), Offset(w, mid), 1.dp.toPx())
                return@Canvas
            }
            val playedX = w * (positionFrames.toFloat() / lengthFrames).coerceIn(0f, 1f)
            val colors = listOf(Metal.WaveLow, Metal.WaveMid, Metal.WaveHigh)
            val colCount = cols.size / 3
            for (x in 0 until colCount) {
                val alpha = if (x < playedX) 1f else 0.5f
                for (band in 0..2) {
                    val amp = cols[x * 3 + band] * mid * 0.92f
                    if (amp > 0.5f) drawLine(colors[band].copy(alpha = alpha), Offset(x + 0.5f, mid - amp), Offset(x + 0.5f, mid + amp), 1f)
                }
            }
            // marcador de cue
            val cueX = w * (cueFrames.toFloat() / lengthFrames).coerceIn(0f, 1f)
            if (cueFrames > 0) {
                drawLine(Metal.Danger, Offset(cueX, 0f), Offset(cueX, h), 1.5f.dp.toPx())
                val tri = Path().apply {
                    moveTo(cueX, 0f)
                    lineTo(cueX + 8.dp.toPx(), 0f)
                    lineTo(cueX, 9.dp.toPx())
                    close()
                }
                drawPath(tri, Metal.Danger)
            }
            // cabeça de leitura
            drawLine(Color.White, Offset(playedX, 0f), Offset(playedX, h), 2.dp.toPx())
            drawRect(deckColor, Offset(0f, h - 2.dp.toPx()), Size(playedX, 2.dp.toPx()))
        }
    }
}

// ------------------------------------------------------------------ vinil

/**
 * Prato de vinil. Gira a 33⅓ rpm só enquanto toca e para junto com a música; o ângulo vem da posição
 * real da faixa. O braço caminha de fora para dentro com o progresso. Não é um controle (na v1 não
 * responde ao toque).
 */
@Composable
fun Platter(
    deckColor: Color,
    positionFrames: Long,
    lengthFrames: Long,
    armOnRight: Boolean,
    modifier: Modifier = Modifier,
) {
    Canvas(modifier) {
        val r = min(size.width, size.height) / 2f
        val c = Offset(size.width / 2f, size.height / 2f)
        drawCircle(
            brush = Brush.sweepGradient(
                listOf(Color(0xFFF1F3F5), Color(0xFF7D868F), Color(0xFFD5DADF), Color(0xFF4A525A), Color(0xFFF1F3F5)),
                center = c,
            ),
            radius = r, center = c,
        )
        drawCircle(Color(0xFF0B0C0E), radius = r * 0.94f, center = c)
        var gr = r * 0.40f
        var n = 0
        while (gr < r * 0.92f) {
            drawCircle(Color.White.copy(alpha = if (n % 3 == 0) 0.07f else 0.03f), radius = gr, center = c, style = Stroke(1f))
            gr += r * 0.03f
            n++
        }
        val angle = (((positionFrames / 48000.0) * (100.0 / 3.0 / 60.0) * 360.0) % 360.0).toFloat()
        rotate(angle, pivot = c) {
            // reflexos (giram com o disco)
            val sheen = Path().apply {
                moveTo(c.x, c.y)
                lineTo(c.x - r * 0.80f, c.y - r * 0.45f)
                lineTo(c.x - r * 0.45f, c.y - r * 0.80f)
                close()
            }
            drawPath(sheen, Color.White.copy(alpha = 0.06f))
            drawCircle(
                brush = Brush.radialGradient(listOf(deckColor.copy(alpha = 0.95f), deckColor.copy(alpha = 0.55f)), c, r * 0.30f),
                radius = r * 0.30f, center = c,
            )
            drawCircle(deckColor, radius = r * 0.30f, center = c, style = Stroke(2.dp.toPx()))
            drawLine(Color(0xFF14171B), Offset(c.x, c.y - r * 0.12f), Offset(c.x, c.y - r * 0.27f), 3.dp.toPx(), StrokeCap.Round)
            drawCircle(deckColor, radius = r * 0.035f, center = Offset(c.x, c.y - r * 0.85f))
        }
        drawCircle(Color(0xFF0B0C0E), radius = r * 0.03f, center = c)

        // braço
        val progress = if (lengthFrames > 0) (positionFrames.toFloat() / lengthFrames).coerceIn(0f, 1f) else 0f
        val dir = if (armOnRight) 1f else -1f
        val pivot = Offset(c.x + dir * r * 1.02f, c.y - r * 0.86f)
        val rad = r * (0.90f - 0.50f * progress)
        val a = (PI / 4).toFloat()
        val tip = Offset(c.x + dir * rad * cos(a), c.y - rad * sin(a))
        drawLine(Color(0xFFC4CAD0), pivot, tip, 4.dp.toPx(), StrokeCap.Round)
        drawLine(Color(0xFF14171B), pivot, tip, 1.dp.toPx(), StrokeCap.Round)
        drawCircle(brush = Brush.radialGradient(listOf(Metal.MetalLight, Metal.MetalDark), pivot, 11.dp.toPx()), radius = 10.dp.toPx(), center = pivot)
        drawCircle(Color(0xFF14171B), radius = 3.dp.toPx(), center = pivot)
        drawRoundRect(Color(0xFFE6E9EC), Offset(tip.x - 5.dp.toPx(), tip.y - 4.dp.toPx()), Size(10.dp.toPx(), 8.dp.toPx()), CornerRadius(2.dp.toPx()))
    }
}
