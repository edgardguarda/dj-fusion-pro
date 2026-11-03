# Recon: DJ FUSION PRO (nome provisório) — Android

Mapa de reconhecimento de DJ.Studio, djay (Android) e Cross DJ (Android), mais suporte a controladoras Numark.
Método: só fontes públicas, clean-room. Nada de código, marcas ou recursos dos originais.

## 0. Escopo assumido (confirme ou corrija)

- Plataforma: Android (celular), nativo onde o áudio exige.
- Núcleo do produto: DJ ao vivo com 2 decks + biblioteca local. Depois, timeline estilo DJ.Studio.
- Para quem: uso próprio do dono do projeto, com possibilidade de virar produto.
- Pedidos novos: visual metálico (ideia de toca-discos clássico) e funcionamento real com Numark DJ2GO2 Touch e outras da marca.

## 1. Fontes e limites da pesquisa

| fonte | o que deu | URL |
| --- | --- | --- |
| DJ.Studio, blog de recursos | timeline, transições, automix, Camelot, stems, export | https://dj.studio/blog/dj-mixing-software-features |
| djay para Android (site oficial) | Automix AI, looper, sampler, streaming, controladoras | https://algoriddim.com/djay-android |
| djay, lista de hardware | Numark Party Mix III e MixTrack Go com Android | https://www.algoriddim.com/hardware |
| Cross DJ Android, controladoras | MIDI só na versão Pro, exige OTG e hub, cue limitado | https://djworx.com/mixvibes-cross-dj-android-controllers |
| Numark DJ2GO2 Touch, manual | USB class-compliant, 44,1 kHz 16 bits, saída master e fones | https://cdn.inmusicbrands.com/Numark/DJ2GO2%20Touch%20-%20User%20Guide%20-%20v1.1.pdf |
| Android, áudio USB | em modo host só mono ou estéreo | https://source.android.com/docs/core/audio/usb |
| Android, áudio de alto desempenho | recomenda Oboe ou AAudio | https://developer.android.com/ndk/guides/audio |
| Fórum Mixvibes | cue em fones com controladora 4 canais falha no Android | https://forum.mixvibes.com/viewtopic.php?p=213310 |

Limites desta pesquisa:
- As páginas da Play Store e a home do DJ.Studio estavam bloqueadas nesta sessão. Usei sites oficiais, manuais e imprensa.
- Parte do que sei do Cross DJ vem de artigos de 2014 a 2016. A versão atual pode ter mais recursos. Marque como "a confirmar".
- O mapa MIDI da DJ2GO2 Touch não está em fonte oficial que eu pude ler. O mapeamento do Mixxx é GPL, então não será copiado.
- Não confirmei se a DJ2GO2 Touch funciona com djay ou Cross DJ no Android. O manual dela cita djay no celular, mas as páginas do djay que li só listam Party Mix III e MixTrack Go como Numark com Android.

## 2. Comparação dos três apps

| área | DJ.Studio | djay (Android) | Cross DJ (Android) |
| --- | --- | --- | --- |
| Plataforma | Mac e Windows. Sem versão Android encontrada | Android, iOS, desktop | Android, iOS |
| Estilo | Timeline no estilo DAW, sem decks | Decks ao vivo | Decks ao vivo |
| Decks e mixer | não | sim | sim |
| Loops e sampler | loops na timeline | Looper até 8 loops, sampler com quantização | sampler, keylock, grades de batida |
| Automix | sim, a partir de playlist | Automix AI, acha intro e outro | a confirmar |
| Timeline multifaixa | sim, com automação de FX, EQ, volume e tom | não | não |
| BPM e tom | Camelot, correção da grade | BPM e sync | BPM e grades |
| Stems | sim, extensão separada | não citado na página do Android | não citado |
| Streaming | Beatport e Beatsource para descoberta | Spotify, Apple Music, TIDAL, SoundCloud, Beatport | a confirmar |
| Controladoras | foca em preparar a mix e tocar em outro software | seleção: DDJ-WeGO4, Reloop Mixtour, Beatpad 2, Mixon4, Numark Party Mix III e MixTrack Go | MIDI só no Pro: DDJ-SB, DDJ-WeGO2, U-Mix Control |
| Exportar | WAV, MP3, vídeo, projeto Ableton, playlists | gravar mix | a confirmar |
| Preço | edições pagas | grátis com assinatura | grátis com compra do Pro |

Lacuna que o DJ FUSION PRO ocupa: ninguém junta decks ao vivo e editor de timeline no mesmo app Android.

## 3. Lista unificada de recursos

Está em `replica/features.csv` (45 linhas: 37 contam na pontuação, 4 são skip e 4 são pedidos seus que os apps originais não têm, como o visual metálico, o português e a DJ2GO2 Touch; a ferramenta de paridade não pontua essas 4, então acompanhe-as à parte). Resumo por área:

- Mixer ao vivo: 2 decks, waveform, play, cue, hot cues, EQ 3 bandas, crossfader, pitch, sync, loops, sampler, efeitos, scratch, pré-escuta.
- Biblioteca: importar arquivos locais, busca, playlists, guardar cues e BPM.
- Análise: BPM com grade, tonalidade, pontos de entrada e saída.
- Editor: timeline multifaixa, cortes, transições, automação, projetos salvos, exportar.
- Inteligente: automix, sugestão harmônica, ordem das faixas, stems (fase avançada).
- Hardware: Numark DJ2GO2 Touch e outros modelos Numark, mapeamento MIDI com aprender.
- Interface: três modos, português, tema escuro metálico.

## 4. Telas

| ID | tela | propósito | estados |
| --- | --- | --- | --- |
| S01 | Início e permissões | pedir acesso a áudio e USB | sem permissão, negado |
| S02 | DJ Live | 2 decks, crossfader, EQ, volumes | sem faixa, carregando, tocando, erro de áudio |
| S03 | Biblioteca | lista, busca, playlists | vazia, importando, com faixas |
| S04 | Detalhe da faixa | BPM, tom, cues, grade | analisando, corrigido à mão |
| S05 | Studio Timeline | faixas, cortes, transições, automação | projeto vazio, com faixas |
| S06 | Exportar | formato e progresso | renderizando, concluído, erro |
| S07 | Controladoras | detectar, perfil, MIDI Learn | sem USB, detectada, sem perfil |
| S08 | Roteamento de áudio | master e cue, teste de fones | só estéreo, cue separado |
| S09 | Ajustes | latência, buffer, tema | |

## 5. Fluxos

- F01 Tocar e mixar: S03 importa, S02 carrega nos decks, ajusta pitch e sync, move o crossfader.
  Casos de borda: arquivo corrompido, formato sem decodificador, faixa sem BPM.
- F02 Ligar a controladora: S01 USB, S07 detecta, aplica perfil, S08 testa a saída.
  Casos de borda: sem energia suficiente, controladora desconectada tocando, modelo sem perfil.
- F03 Montar uma mix: S03, S05 arrasta faixas, ajusta transições e automação, S06 exporta.
- F04 Automix de playlist: S03 escolhe playlist, S05 gera a sequência, revisão manual, S06.

## 6. Componentes

Deck (capa, waveform, botões), waveform com zoom, jog virtual, fader vertical e horizontal, knob de EQ, pad de hot cue, medidor de nível, lista de faixas, régua da timeline, faixa de automação, seletor de arquivos, diálogos de permissão, indicador de controladora.

## 7. Modelo de dados (inferido, confiança média)

- Track: id, uri, título, artista, duração, bpm, grade de batidas, tom, pontos de entrada e saída. Evidência: telas de biblioteca e análise dos três apps.
- Cue: id, track_id, posição, tipo (cue, hot cue, loop), cor.
- Playlist e PlaylistItem: ordem das faixas.
- Project: id, nome, BPM mestre. TimelineClip: faixa, início, corte, ganho. Automation: alvo, pontos de curva. Evidência: DJ.Studio.
- ControllerProfile: modelo, mensagens MIDI para cada controle. Evidência: djay e Cross DJ pré-mapeiam modelos.

## 8. Hardware: Numark DJ2GO2 Touch e outras

O que o manual diz:
- Conexão mini-USB, class-compliant, sem driver. Alimentada pelo USB.
- Interface de áudio 44,1 kHz 16 bits, saída master e saída de fones, botão PFL/Cue.
- No celular, pede adaptador USB alimentado.
- Controles: 2 decks (load, play, cue, sync, PFL), jog sensível ao toque, browse, crossfader, pitch por deck, ganhos, 4 pads com modos hot cue, auto loop, loop manual e sampler.

O que o Android permite:
- MIDI USB via `android.media.midi` (API 23 ou superior). Dá para ler os botões, jogs e faders.
- Áudio: em modo host o Android só documenta saída mono ou estéreo, nos dois primeiros canais. Isso é o maior risco para o cue nos fones da controladora.
- No fórum da Mixvibes, uma controladora de 4 canais foi detectada mas o cue não funcionou sem mexer em configuração de sistema com root. Isso não é solução aceitável.

Estratégia proposta (a validar com a sua controladora real):
1. Master pela saída USB estéreo da controladora.
2. Cue pelo fone ligado no próprio celular (ou Bluetooth) em um segundo fluxo de áudio, se o aparelho aceitar duas saídas ao mesmo tempo.
3. Alternativa: split mono no mesmo estéreo (cue em um lado, master no outro), com cabo separador.
4. Teste de roteamento na tela S08 mostra o que o seu aparelho realmente consegue.

Mapeamento: sem copiar mapas de terceiros. A tela S07 terá um monitor MIDI e MIDI Learn. Você liga a sua DJ2GO2 Touch, mexe em cada controle, e o app gera o perfil. Depois repetimos para outros modelos Numark.

## 9. Arquitetura recomendada

- Interface: Kotlin com Jetpack Compose.
- Motor de áudio: C++ pelo NDK com Oboe (Apache 2.0), buffers de baixa latência, mixagem de 2 decks, EQ, crossfader, efeitos próprios.
- Decodificação: MediaExtractor e MediaCodec do Android (MP3, AAC, FLAC, WAV) para PCM em cache.
- Time-stretch e keylock: biblioteca de licença permissiva (por exemplo Signalsmith Stretch, MIT, a confirmar na fase de arquitetura). Evitar GPL.
- BPM, grade e tonalidade: implementação própria (detecção de ataques e perfil de cromas).
- Dados: Room (SQLite).
- Controladoras: `MidiManager` com motor de mapeamento em perfis JSON e MIDI Learn.
- Exportação: render offline para WAV primeiro. MP3 depende de encoder com licença adequada.
- Stems: só depois, processamento offline em segundo plano, com modelo de licença compatível.
- Build: GitHub Actions gera APK debug assinado, sem computador.

## 10. Plano por etapas

1. v1 (definição abaixo).
2. Controladoras: DJ2GO2 Touch com MIDI Learn e teste de roteamento de áudio.
3. Hot cues, loops, sampler, efeitos, jog como scratch.
4. Análise: BPM, grade, tonalidade, pontos de entrada e saída.
5. Studio Timeline com transições e automação, depois exportação.
6. Automix e sugestões harmônicas.
7. Stems (opcional, se viável no aparelho).

## 11. Primeira versão funcional (definição)

Só conta como pronta se funcionar em um celular Android real:
- Importar músicas locais pelo seletor de arquivos.
- Biblioteca com busca.
- Dois decks tocando ao mesmo tempo.
- Play, pause, cue, volume por deck, crossfader.
- Waveform por deck.
- Interface em português com o visual metálico.
- APK gerado por GitHub Actions.

Fica de fora da v1: EQ, pitch, sync, controladoras, loops, efeitos e timeline. Nenhum botão decorativo: o que não existir ainda não aparece na tela.

## 12. O que não será clonado

- Streaming (Spotify, Apple Music, TIDAL, SoundCloud, Beatport): exige API oficial e licença.
- Catálogos, pacotes de samples e serviços dos apps originais.
- Integrações de desktop (VST, rekordbox, Serato, Traktor).
- Marca, logotipo, textos e arte dos três apps.

## 13. Direção visual: "aço escovado" (inspiração em toca-discos clássicos)

- Superfícies de alumínio escovado e grafite escuro, mantendo o tema escuro.
- Detalhes de prato com anel refletivo e pontos de estroboscópio como motivo gráfico.
- Luzes de LED em âmbar (deck A) e azul gelo (deck B).
- Fader de pitch e botões grandes com relevo metálico, otimizados para toque.
- Sem usar nome, logotipo, fontes ou desenho exato de nenhum fabricante. Os tokens de cor e componentes saem em `/replica-design`.

## 14. Tamanho

- Telas: 9. Fluxos: 4. Entidades: 8.
- Partes difíceis: (1) cue em fones com a controladora no Android, (2) áudio de baixa latência estável em vários aparelhos, (3) time-stretch com qualidade e análise de BPM.
- Tamanho: L (um trimestre para o conjunto completo). A v1 sozinha é M (algumas semanas).
