# Componentes (Android, Jetpack Compose)

Regras gerais: alvo de toque mínimo de 48 dp; botões de ação 56 dp; pads 72 dp. Texto sempre com contraste AA ou melhor (relatório: 21 pares, 0 falhas). Todo controle tem rótulo de acessibilidade em português (TalkBack). Nenhum controle é desenhado antes de existir a função por trás dele.

Deck A usa o LED âmbar (`accent`) e deck B o azul gelo (`accent-b`). Cor nunca é o único sinal: o deck também tem a letra A ou B.

```
Botão de transporte (Play, Pause, Cue)
  variantes  play (grande, LED aceso quando tocando), cue (LED aceso ao segurar)
  tamanhos   56 dp, play 64 dp
  estados    normal, pressionado (borda interna escura), ativo (LED na cor do deck), desabilitado (sem faixa)
  tokens     placa metal-light a metal-dark, texto on-metal, raio md
  a11y       "Tocar deck A", "Pausar deck A", estado anunciado; vibração curta ao tocar
  usado em   S02

Fader vertical (volume do deck) e fader horizontal (crossfader)
  tamanhos   alça 64 dp, trilho 12 dp
  estados    normal, arrastando (LED na alça), desabilitado
  tokens     trilho surface-raised, alça metal, marcas border
  a11y       valor anunciado em porcentagem; ajuste fino por gesto de deslizar com dois dedos
  usado em   S02

Waveform
  estados    sem faixa (linha vazia), carregando (placeholder), pronta, tocando (cabeça de leitura na cor do deck)
  gestos     toque posiciona, arrasto desloca, dois dedos dá zoom (zoom entra no marco M4)
  tokens     onda na cor do deck, fundo bg, cabeça text
  a11y       descrição "posição 1:23 de 4:05"; controle alternativo por botões de avançar e voltar
  usado em   S02, S04

Prato virtual (jog) com estroboscópio
  motivo     anel de metal e 60 pontos em volta, pontos giram com a velocidade da faixa
  estados    parado, girando, tocado (scratch no marco M5)
  tokens     ring (gradiente metálico), pontos text-muted
  usado em   S02 (só aparece quando o scratch existir)

Knob de EQ (Graves, Médios, Agudos)
  marco      M4 (não aparece na v1)
  gestos     arrastar na vertical, toque duplo volta ao centro
  a11y       valor em dB anunciado

Item de faixa (biblioteca)
  conteúdo   título, artista, duração, BPM e tom quando analisados
  estados    normal, tocando em A ou em B, arquivo ausente (ícone de aviso), analisando
  ações      carregar no deck A, carregar no deck B (dois botões de 48 dp)
  usado em   S03

Campo de busca
  tamanho    48 dp, borda border-input
  estados    vazio, com texto, sem resultados ("Nenhuma faixa encontrada")
  usado em   S03

Medidor de nível (VU)
  segmentos  verde, amarelo e vermelho (success, warning, danger)
  a11y       decorativo, sem leitura por voz
  usado em   S02

Diálogo de permissão
  usado em   S01
  texto      explica para que serve cada permissão, em português, e leva ao seletor de arquivos

Barra de modos (DJ Live, Studio Timeline, Biblioteca)
  tipo       barra no topo com 3 abas, 24 dp de altura no layout paisagem (a área de toque de cada aba chega a 48 dp)
  regra      "Studio Timeline" fica visível mas desabilitado, com a legenda "em breve", até o marco M6. Nada de tela vazia fingindo funcionar.
```

Estados que todo componente precisa ter: normal, pressionado, foco por teclado ou controle (anel de 2 dp na cor `accent`), desabilitado, erro quando houver.

Ícones: conjunto aberto (Material Symbols, Apache 2.0, ou Lucide). Nada de ícones nem arte tirados dos três apps de referência. Sons de interface: nenhum na v1.

## Layout profissional (substitui o layout em retrato do primeiro esboço)

Padrões comuns a todo software de DJ, sem copiar o desenho de nenhum deles:
- Paisagem. Duas ondas roladas na horizontal ocupam a largura toda no topo, uma sobre a outra, cada uma com grade de batidas, marcador de cue, região de loop, visão geral e tempo restante. A cabeça de leitura fica no centro.
- Decks nas laterais (A à esquerda, B à direita), mixer no centro com agudos, médios e graves por canal, fader de canal, medidor e crossfader.
- Cada deck: título, BPM em fonte mono, pitch, tom, prato, PLAY, CUE, SYNC, 4 hot cues, loops e fader de pitch.
- Biblioteca e Timeline são modos separados (barra do topo), porque não cabem junto dos decks num celular.
- Alvos de toque: play, cue e sync 56 x 48 dp ou mais; pads 48 dp; crossfader 40 dp de altura da alça (a área de toque vai até 48 dp). Os knobs de EQ têm 28 dp visíveis e área de toque de 40 dp, a única exceção à regra de 48 dp, por falta de espaço.
- A prévia mostra o alvo final. Na v1 só entram os controles que funcionam: sem EQ, pitch, sync, hot cues e loops até seus marcos (M3 a M5). O resto fica oculto, não decorativo.

## Versão 3: ideias das referências do usuário (vinil, pads coloridos, espectro)

Tomamos só os conceitos, nunca a arte, os nomes ou os textos dos apps mostrados:
- Prato de vinil grande com sulcos e reflexo, anel de alumínio e braço. O braço anda de fora para dentro conforme a faixa toca (posição real). O selo central mostra a capa da faixa quando existir; sem capa, um selo na cor do deck com uma nota musical.
- Ondas coloridas por faixa de frequência: graves em laranja, médios em amarelo, agudos em azul claro, uma sobre a outra, como nos programas profissionais.
- Analisador de espectro em arco-íris no centro do mixer (FFT do som master, entra no marco M4 junto com o EQ).
- Pads coloridos de hot cue 1 a 4, na mesma ordem esquerda-direita nos dois decks. Cores: vermelho, amarelo, verde, azul no deck A; roxo, laranja, verde, vermelho no deck B.
- Pads de sampler (sons curtos): ficam para o marco M5. Os sons serão sintetizados por nós, originais, sem usar nenhum áudio dos apps de referência.
- Transporte: PLAY e CUE com 52 dp de altura, SYNC com 48 dp (a regra de 56 dp passa a 52 dp aqui, por espaço).
- Metal continua nos botões, knobs, faders e no aro do prato.
