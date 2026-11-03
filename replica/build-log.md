# Registro de construção

Regra deste projeto: um recurso só vira `yes` em `features.csv` depois de rodar de verdade no aparelho. Por enquanto nenhum está assim, porque o app ainda não foi instalado em nenhum celular.

## Marco M0 e M1: base e primeira versão (código escrito)

| parte | estado | como foi verificado |
| --- | --- | --- |
| Motor de áudio C++ (2 decks, crossfader, volumes, master, cue, seek, fim da faixa) | feito | 94 testes do motor + 225.622 verificações extras, com AddressSanitizer, UBSan e ThreadSanitizer |
| Callback de áudio sem alocar memória | feito | teste que conta alocações em 2.000 chamadas de render: zero |
| Conversor de taxa 44,1/96 kHz para 48 kHz | feito | erro máximo de 0,00004 em 44,1 para 48 kHz; tom de 40 kHz some em 96 para 48 kHz |
| Picos da waveform por faixa de frequência | feito | graves, médios e agudos separados em testes com tons de 60 Hz, 1 kHz e 9 kHz |
| Ponte JNI Kotlin e C++ | feita | 24 funções com nomes e tipos conferidos por script, e teste numa JVM real com `-Xcheck:jni` |
| Saída de áudio com Oboe | escrita | **não testada**: só compilada contra um Oboe falso que imita a API de memória |
| Decodificador (MediaCodec) | escrito | **não testado**: precisa de um Android |
| Telas Compose, ViewModel, biblioteca (SQLite) | escritas | **não compiladas nem rodadas**: sem Android SDK neste ambiente |
| Projeto Gradle, manifesto, ícone, CI | escritos | **não compilados**; o primeiro APK vai mostrar erros que só o compilador acha |

## Problemas achados e corrigidos nesta etapa

- **Carregar uma faixa era quadrático.** `reserve()` a cada bloco copiava o áudio inteiro de novo: 27,8 s para 4 minutos, e perto do limite de 15 minutos o app cairia. Agora 0,65 s. Há teste de regressão.
- **Falta de memória atravessando o JNI** derrubaria o app. Agora vira "faixa grande demais".
- **Memória nativa vazava** se o carregamento fosse cancelado no último instante.
- **Estado velho do motor** sobrevivia se a tela fosse fechada e reaberta com o processo vivo.
- **Tocar durante o carregamento** podia cortar o som; agora play e cue ficam desabilitados.
- **Falha de carregamento** apagava título e artista do deck; agora restaura.
- **Botões A e B da biblioteca** disparavam ao encostar, atrapalhando a rolagem; agora só no toque completo.
- **Sem serviço em primeiro plano**, o app agora pausa os decks e fecha o áudio ao sair da tela, e reabre ao voltar.

## Decisões

- A biblioteca da v1 usa o SQLite do próprio Android (tabela `tracks`), sem Room. O restante do `schema.sql` entra quando for preciso (playlists, timeline).
- O motor guarda cada faixa em 16 bits estéreo a 48 kHz, com limite de 15 minutos (cerca de 173 MB por faixa).
- Revisão do Kotlin feita por leitura independente, porque não há compilador aqui. Ela não achou erro de compilação, mas isso não garante nada até o primeiro build.

## O que falta para a v1 estar pronta de verdade

1. Compilar o APK (o ambiente daqui não alcança os servidores do Google).
2. Instalar no POCO X7 Pro e ouvir: importar, tocar nos dois decks, crossfader, cue, volume.
3. Medir latência e estabilidade do áudio no HyperOS.
4. Só então marcar os recursos como `yes`.
