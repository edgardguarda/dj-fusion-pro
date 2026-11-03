# Arquitetura: DJ FUSION PRO (Android nativo)

Lê `replica/recon.md` e `replica/features.csv`. O esquema do banco está em `replica/schema.sql` (9 tabelas, testado no SQLite).
O skill sugere web e Postgres por padrão. Aqui não serve: o app precisa de áudio de baixa latência e MIDI USB no aparelho, então tudo é nativo e local. Não há servidor na v1.

## 1. Aparelho alvo

POCO X7 Pro: Dimensity 8400-Ultra, 8 ou 12 GB de RAM, USB-C, HyperOS 2 (fonte: página oficial da Xiaomi).
A página não lista a versão do Android, o suporte a OTG nem entrada de fone de 3,5 mm. Trato os três como "a confirmar no aparelho". Como o celular provavelmente não tem entrada P2, a pré-escuta em fones muda de plano (seção 6).

## 2. Stack

| camada | escolha | por quê |
| --- | --- | --- |
| Interface | Kotlin + Jetpack Compose | toque, gestos e animações de waveform sem XML |
| Motor de áudio | C++ (NDK) + Oboe, Apache 2.0 | recomendado pelo Google para áudio de baixa latência; escolhe AAudio ou OpenSL sozinho |
| Time-stretch e keylock | Signalsmith Stretch, MIT | licença permissiva, 0,75x a 1,5x soa bem, cobre pitch de DJ |
| Decodificação | MediaExtractor e MediaCodec | MP3, AAC, FLAC, WAV já vêm no Android, sem licença extra |
| BPM, grade e tom | implementação própria | as bibliotecas comuns são GPL ou AGPL, que não servem a um produto fechado (conferir antes de adotar qualquer uma) |
| Banco | Room (SQLite) | local, simples, migrações versionadas |
| Controladoras | `android.media.midi` (API 23 ou superior) | MIDI USB class-compliant sem driver |
| Arquivos | Storage Access Framework | acesso só a arquivos que você escolher |
| Exportação | WAV e AAC (m4a) | o Android não traz encoder de MP3, ficaria para depois |
| Build | Gradle + GitHub Actions | APK sem computador |
| minSdk | 27 | AAudio disponível; o X7 Pro é bem mais novo |

Pontos a confirmar: o Signalsmith Stretch depende de outra biblioteca da mesma autoria (Signalsmith Linear), cuja licença ainda não li. E a versão atual do Oboe precisa ser fixada no Gradle na fase de construção.
Stems (fase 7): modelos de separação rodam offline e pesam. A licença do código e dos pesos do modelo precisa ser lida antes.

## 3. Módulos

```
app/            Compose, navegação, ViewModels (S01 a S09)
audio-engine/   C++ + Oboe: decks, mixer, EQ, crossfader, stretch, medidores
decode/         Kotlin: arquivo -> PCM no cache + picos da waveform
analysis/       BPM, grade, tom, intro e outro (fase 4)
data/           Room: tracks, cues, playlists, projects
midi/           detectar USB, perfis JSON, MIDI Learn, monitor
```

Regras do motor de áudio (quebrar uma delas gera estalos):
- O callback do Oboe não aloca memória, não usa mutex, não faz I/O nem chama JNI.
- Comandos da interface e do MIDI entram por fila sem trava (um produtor, um consumidor) e o callback os aplica no início de cada bloco.
- Medidores e posição voltam para a interface por valores atômicos lidos a ~60 vezes por segundo.
- O PCM de cada faixa fica em arquivo mapeado em memória: 5 min estéreo a 48 kHz em 16 bits = cerca de 58 MB por faixa no disco, sem ocupar a RAM.
- Todas as faixas são convertidas para a taxa de saída do aparelho na decodificação (normalmente 48 kHz).

## 4. Banco de dados

Resumo (detalhes e restrições em `replica/schema.sql`):

| tabela | para quê | fase |
| --- | --- | --- |
| tracks | faixa, BPM, grade, tom, intro e outro, estado da análise | v1 |
| cues | cue, hot cues e loops por faixa | 3 |
| playlists, playlist_items | playlists e sets | v1 |
| projects, lanes, clips, automation_points | timeline e automação | 5 |
| controller_profiles | perfil MIDI por controladora, em JSON | 2 |

Acesso: um único usuário dono do aparelho, sem conta. Cada faixa é um `content://` autorizado; apagar uma faixa da biblioteca apaga seus cues e itens de playlist, mas é bloqueado se um clip da timeline ainda a usa.

## 5. Interfaces internas (no lugar de uma API de servidor)

| interface | o que faz | quem chama | thread |
| --- | --- | --- | --- |
| `Engine.start/stop` | abre e fecha o fluxo Oboe | app | principal |
| `Deck.load(trackId)` | prepara PCM e waveform no deck | UI, MIDI | decodificação |
| `Deck.play/pause/cue/seek` | transporte | UI, MIDI | fila para o áudio |
| `Deck.setPitch(%)`, `setKeylock` | tempo e tom | UI, MIDI | fila |
| `Deck.setEq(low,mid,high)`, `setGain` | mixagem | UI, MIDI | fila |
| `Mixer.setCrossfader`, `setMaster`, `setCurve` | mistura entre decks | UI, MIDI | fila |
| `Deck.getPosition`, `getLevel` | posição e medidor | UI | leitura atômica |
| `Library.importFiles(uris)` | inclui faixas e dispara a análise | UI | segundo plano |
| `Analysis.run(trackId)` | BPM, grade, tom | serviço | segundo plano |
| `Midi.onDevice`, `Midi.onMessage` | detecta e lê a controladora | sistema | thread MIDI |
| `Midi.learn(control)` | associa o próximo movimento a um controle | S07 | thread MIDI |
| `Audio.routeTest()` | mede o que o aparelho consegue de saída | S08 | principal |
| `Export.render(projectId)` | renderiza a timeline em WAV ou AAC | S06 | segundo plano |

Perfil MIDI: formato próprio em JSON com `{controle do app -> tipo, canal, número}`, gerado pelo MIDI Learn. Não vou escrever números de nenhuma controladora de memória, porque estariam errados; eles saem do seu aparelho.

## 6. Partes difíceis

1. **Pré-escuta (cue) em fones.** O Android documenta áudio USB só mono ou estéreo, e o X7 Pro provavelmente não tem P2. Opções, em ordem de preferência, todas a testar na tela S08:
   - A) A controladora entrega master e fones a partir de um fluxo estéreo (depende de como a DJ2GO2 Touch roteia o fone; o manual não diz se há canais separados).
   - B) Split mono: cue de um lado e master do outro, com cabo separador. Serve para treino com fones, mas o público ouviria só um lado do master.
   - C) Cue por Bluetooth: tem latência alta, ruim para casar batidas.
   - D) Sem cue, usando só o master (mais simples, e a v1 já funciona assim).
2. **Latência e estabilidade do áudio.** Serviço em primeiro plano, modo de baixa latência, tratamento de perda de foco de áudio, e pedido de isenção de bateria para o HyperOS não matar o app.
3. **Energia e USB.** A controladora é alimentada pelo USB, e o manual pede adaptador alimentado. O app precisa tratar desconexão sem travar o áudio.
4. **Time-stretch.** Dois decks com stretch consomem CPU. Meta: manter o pitch dentro de ±16% e medir o uso no seu aparelho.
5. **Análise de BPM.** Música com tempo variável e intro sem batida enganam o detector, então precisa de correção manual da grade (S04).
6. **Armazenamento.** Com `content://` do seletor de arquivos, guardar permissão persistente; arquivo apagado ou movido precisa marcar a faixa como "ausente".
7. **Sincronia entre duas saídas.** Se o master vai pela USB e o cue por outro dispositivo, os relógios divergem e a latência é diferente. Só será oferecido se o teste mostrar que é estável.

## 7. Ordem de construção

| marco | conteúdo | telas | tabelas |
| --- | --- | --- | --- |
| M0 | projeto Gradle, tela inicial, APK por GitHub Actions | S01 | nenhuma |
| M1 | fatia vertical: importar um arquivo, tocar em um deck pelo Oboe, waveform | S01, S02 (1 deck), S03 | tracks |
| M2 | **v1**: 2 decks simultâneos, crossfader, volumes, cue, biblioteca com busca, visual metálico | S02, S03 | tracks, playlists, playlist_items |
| M3 | controladoras: monitor MIDI, MIDI Learn, perfil da DJ2GO2 Touch, teste de saída | S07, S08 | controller_profiles |
| M4 | EQ 3 bandas, pitch, sync, análise de BPM, tom | S04, S02 | tracks (campos de análise) |
| M5 | hot cues, loops, sampler, efeitos, jog como scratch | S02 | cues |
| M6 | Studio Timeline, transições, automação, exportar | S05, S06 | projects, lanes, clips, automation_points |
| M7 | automix, sugestões harmônicas, stems (se viável) | S03, S05 | tracks |

Os recursos de cada marco só entram em `features.csv` como `yes` depois de rodar e passar nos testes de `/replica-test`.

## 8. APK por GitHub Actions

- A cada envio para `main`: compilar em um computador virtual do GitHub e guardar o APK como artefato.
- A cada tag de versão: criar uma Release com o APK anexado, que dá para baixar direto no celular.
- Assinatura: chave de depuração enquanto testa. Para publicar, uma chave de lançamento guardada nos segredos do GitHub (nunca no repositório).
- Primeira instalação pede permitir "instalar apps desconhecidos" para o navegador ou gerenciador de arquivos.

## 9. Licenças e originalidade

Código próprio mais Oboe (Apache 2.0), Signalsmith Stretch (MIT), Room, Compose e Kotlin (Apache 2.0). Sem código, imagens, sons ou textos dos apps de referência, e os APKs que você mandou não foram abertos.

## 10. Riscos, em ordem

1. Pré-escuta com a DJ2GO2 Touch no Android (seção 6.1).
2. Latência e estabilidade do áudio no HyperOS.
3. Qualidade do time-stretch e da análise de BPM.

Próximo passo: `/replica-design`.
