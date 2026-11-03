package app.djfusion.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.displayCutout
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.disabled
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import app.djfusion.ui.theme.Metal
import kotlinx.coroutines.delay

private enum class Screen { LIVE, LIBRARY }

@Composable
fun AppRoot(vm: AppViewModel) {
    var screen by rememberSaveable { mutableStateOf(Screen.LIVE) }
    val message by vm.message.collectAsStateWithLifecycle()
    val status by vm.audioStatus.collectAsStateWithLifecycle()

    Surface(
        Modifier.fillMaxSize(),
        color = MaterialTheme.colorScheme.background,
        contentColor = MaterialTheme.colorScheme.onBackground,
    ) {
        Column(Modifier.fillMaxSize().windowInsetsPadding(WindowInsets.displayCutout)) {
            Row(
                Modifier.fillMaxWidth().height(44.dp).background(Color(0xFF16191D)),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Tab("DJ LIVE", selected = screen == Screen.LIVE, enabled = true) { screen = Screen.LIVE }
                Tab("BIBLIOTECA", selected = screen == Screen.LIBRARY, enabled = true) { screen = Screen.LIBRARY }
                // O editor de timeline ainda não existe: fica visível, mas desligado e avisando.
                Tab("TIMELINE · em breve", selected = false, enabled = false) {}
                Box(Modifier.weight(1f))
                Text(status, color = Metal.TextMuted, fontSize = 12.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.padding(end = 14.dp))
            }
            Box(Modifier.weight(1f).fillMaxWidth()) {
                when (screen) {
                    Screen.LIVE -> DjLiveScreen(vm)
                    Screen.LIBRARY -> LibraryScreen(vm)
                }
                val text = message
                if (text != null) {
                    LaunchedEffect(text) {
                        delay(3500)
                        vm.clearMessage(text)
                    }
                    Text(
                        text,
                        color = Metal.Text, fontSize = 14.sp, fontWeight = FontWeight.SemiBold,
                        modifier = Modifier.align(Alignment.TopCenter).padding(top = 8.dp)
                            .background(Color(0xF2262B31), RoundedCornerShape(10.dp))
                            .padding(horizontal = 16.dp, vertical = 10.dp),
                    )
                }
            }
        }
    }
}

@Composable
private fun Tab(label: String, selected: Boolean, enabled: Boolean, onClick: () -> Unit) {
    val color = when {
        !enabled -> Color(0xFF6B747D)
        selected -> Metal.DeckA
        else -> Metal.TextMuted
    }
    Box(
        Modifier.heightIn(min = 44.dp)
            .semantics {
                role = Role.Tab
                contentDescription = if (enabled) label else "Timeline, ainda não disponível"
                if (!enabled) disabled()
            }
            .clickable(enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(label, color = color, fontSize = 13.sp, fontWeight = FontWeight.Bold, letterSpacing = 1.sp)
        if (selected) {
            Box(Modifier.align(Alignment.BottomCenter).fillMaxWidth().height(3.dp).background(Metal.DeckA))
        }
    }
}
