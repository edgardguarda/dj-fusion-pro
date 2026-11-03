package app.djfusion.ui

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import app.djfusion.data.SortOrder
import app.djfusion.ui.theme.Metal

@Composable
fun LibraryScreen(vm: AppViewModel) {
    val tracks by vm.tracks.collectAsStateWithLifecycle()
    val search by vm.search.collectAsStateWithLifecycle()
    val sort by vm.sort.collectAsStateWithLifecycle()
    val decks by vm.decks.collectAsStateWithLifecycle()

    val picker = rememberLauncherForActivityResult(ActivityResultContracts.OpenMultipleDocuments()) { uris ->
        vm.importUris(uris)
    }

    Column(Modifier.fillMaxSize().background(Metal.Bg).padding(horizontal = 14.dp, vertical = 8.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            OutlinedTextField(
                value = search,
                onValueChange = vm::setSearch,
                singleLine = true,
                placeholder = { Text("Buscar por título ou artista") },
                modifier = Modifier.weight(1f),
            )
            Spacer(Modifier.width(10.dp))
            MetalButton(
                description = "Importar músicas do aparelho",
                onPress = { picker.launch(arrayOf("audio/*")) },
                modifier = Modifier.width(132.dp).height(56.dp),
            ) {
                Text("IMPORTAR", color = Metal.OnMetal, fontSize = 14.sp, fontWeight = FontWeight.ExtraBold)
            }
        }
        Row(Modifier.padding(vertical = 8.dp), horizontalArrangement = Arrangement.spacedBy(8.dp), verticalAlignment = Alignment.CenterVertically) {
            Text("Ordenar:", color = Metal.TextMuted, fontSize = 13.sp)
            SortChip("Recentes", sort == SortOrder.RECENT) { vm.setSort(SortOrder.RECENT) }
            SortChip("Título", sort == SortOrder.TITLE) { vm.setSort(SortOrder.TITLE) }
            SortChip("Artista", sort == SortOrder.ARTIST) { vm.setSort(SortOrder.ARTIST) }
            Spacer(Modifier.weight(1f))
            Text("${tracks.size} faixa(s)", color = Metal.TextMuted, fontSize = 13.sp)
        }

        if (tracks.isEmpty()) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text(
                    if (search.isBlank()) {
                        "Sua biblioteca está vazia.\nToque em IMPORTAR e escolha músicas do aparelho (MP3, WAV, FLAC, AAC, OGG)."
                    } else {
                        "Nenhuma faixa encontrada para \"$search\"."
                    },
                    color = Metal.TextMuted, fontSize = 15.sp,
                )
            }
        } else {
            LazyColumn(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                items(tracks, key = { it.id }) { row ->
                    Row(
                        Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp)).background(Metal.Surface)
                            .padding(horizontal = 12.dp, vertical = 6.dp).heightIn(min = 56.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Column(Modifier.weight(1f)) {
                            Text(row.title, color = Metal.Text, fontSize = 15.sp, fontWeight = FontWeight.SemiBold, maxLines = 1, overflow = TextOverflow.Ellipsis)
                            Text(
                                if (row.artist.isEmpty()) "Artista desconhecido" else row.artist,
                                color = Metal.TextMuted, fontSize = 12.sp, maxLines = 1, overflow = TextOverflow.Ellipsis,
                            )
                        }
                        Text(formatTime(row.durationMs), color = Metal.TextMuted, fontSize = 13.sp, modifier = Modifier.padding(horizontal = 12.dp))
                        for (deck in 0..1) {
                            MetalButton(
                                description = "Carregar ${row.title} no deck ${AppViewModel.deckName(deck)}",
                                onPress = { vm.loadTrack(deck, row) },
                                fireOnPress = false,
                                led = deckColor(deck), ledOn = decks[deck].trackId == row.id && decks[deck].loaded,
                                modifier = Modifier.size(width = 56.dp, height = 48.dp),
                            ) {
                                Text(AppViewModel.deckName(deck), color = Metal.OnMetal, fontSize = 18.sp, fontWeight = FontWeight.ExtraBold)
                            }
                            Spacer(Modifier.width(8.dp))
                        }
                        Box(
                            Modifier.size(48.dp).clip(RoundedCornerShape(10.dp))
                                .semantics {
                                    contentDescription = "Remover ${row.title} da biblioteca. O arquivo não é apagado."
                                    role = Role.Button
                                }
                                .clickable { vm.removeFromLibrary(row) },
                            contentAlignment = Alignment.Center,
                        ) {
                            Text("✕", color = Metal.TextMuted, fontSize = 18.sp)
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun SortChip(label: String, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(50)
    Box(
        Modifier.heightIn(min = 48.dp).clip(shape)
            .semantics { role = Role.Button; contentDescription = "Ordenar por $label" }
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Box(
            Modifier.clip(shape)
                .background(if (selected) Metal.DeckA else Metal.SurfaceRaised)
                .padding(horizontal = 14.dp, vertical = 6.dp),
        ) {
            Text(label, color = if (selected) Metal.OnDeckA else Metal.Text, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
        }
    }
}
