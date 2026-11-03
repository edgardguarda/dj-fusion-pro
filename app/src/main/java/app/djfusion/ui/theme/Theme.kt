package app.djfusion.ui.theme

// Gerado a partir de replica/design/tokens.json. Mudou o token, mude aqui.
// O nome do pacote é provisório e muda no /replica-brand.

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

object Metal {
    val Bg = Color(0xFF121417)
    val Surface = Color(0xFF1C2025)
    val SurfaceRaised = Color(0xFF262B31)
    val Border = Color(0xFF3A4149)
    val BorderInput = Color(0xFF7C8791)
    val Text = Color(0xFFE6E9EC)
    val TextMuted = Color(0xFF9AA4AE)
    val DeckA = Color(0xFFFFA726)       // LED âmbar
    val OnDeckA = Color(0xFF1A1206)
    val DeckB = Color(0xFF5CC8FF)       // LED azul gelo
    val OnDeckB = Color(0xFF04141F)
    val MetalLight = Color(0xFFD5DADF)
    val MetalMid = Color(0xFFA9B1B9)
    val MetalDark = Color(0xFF6B747D)
    val OnMetal = Color(0xFF14171B)
    val Danger = Color(0xFFFF6B6B)
    val Success = Color(0xFF4CD08A)
    val Warning = Color(0xFFFFD166)
    val WaveLow = Color(0xFFFF6A3D)
    val WaveMid = Color(0xFFFFD166)
    val WaveHigh = Color(0xFF8FE3FF)

    /** Alumínio escovado: faixas horizontais claras e escuras. A textura fina é desenhada no Canvas. */
    val BrushedPlate = Brush.verticalGradient(
        listOf(
            Color(0xFFD5DADF), Color(0xFFA9B1B9), Color(0xFFC4CAD0),
            Color(0xFF8F979F), Color(0xFFB8BFC6),
        )
    )
    val GraphitePanel = Brush.verticalGradient(
        listOf(Color(0xFF2B3036), Color(0xFF1C2025), Color(0xFF262B31))
    )
}

object Dim {
    val TouchMin = 48.dp
    val Button = 56.dp
    val Pad = 72.dp
    val FaderHandle = 64.dp
    val Sm = 6.dp
    val Md = 10.dp
    val Lg = 16.dp
}

val LocalDeckColors = staticCompositionLocalOf { Metal.DeckA to Metal.DeckB }

private val Scheme = darkColorScheme(
    background = Metal.Bg,
    surface = Metal.Surface,
    surfaceVariant = Metal.SurfaceRaised,
    outline = Metal.BorderInput,
    outlineVariant = Metal.Border,
    onBackground = Metal.Text,
    onSurface = Metal.Text,
    onSurfaceVariant = Metal.TextMuted,
    primary = Metal.DeckA,
    onPrimary = Metal.OnDeckA,
    secondary = Metal.DeckB,
    onSecondary = Metal.OnDeckB,
    error = Metal.Danger,
)

// Fontes Inter e JetBrains Mono (OFL) entram em res/font na fase de construção.
private val Type = Typography(
    labelSmall = TextStyle(fontSize = 12.sp, lineHeight = 16.sp, fontWeight = FontWeight.SemiBold),
    bodyMedium = TextStyle(fontSize = 14.sp, lineHeight = 20.sp, fontWeight = FontWeight.Normal),
    titleMedium = TextStyle(fontSize = 16.sp, lineHeight = 22.sp, fontWeight = FontWeight.SemiBold),
    headlineSmall = TextStyle(fontSize = 20.sp, lineHeight = 26.sp, fontWeight = FontWeight.Bold),
    headlineMedium = TextStyle(fontSize = 28.sp, lineHeight = 32.sp, fontWeight = FontWeight.Bold),
    displaySmall = TextStyle(fontSize = 40.sp, lineHeight = 44.sp, fontWeight = FontWeight.Bold),
)

/** O app é sempre escuro: o metal brilha melhor sobre grafite e a cena de DJ costuma ser escura. */
@Composable
fun DjFusionTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = Scheme, typography = Type, content = content)
}
