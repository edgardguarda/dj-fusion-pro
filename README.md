# DJ Fusion Pro (nome provisório)

App de DJ para Android: dois decks, crossfader e biblioteca de músicas locais, com motor de áudio próprio em C++ (Oboe) e interface em Jetpack Compose, em tema escuro com superfícies de alumínio escovado.

## Estado honesto

**Ainda não foi compilado nem instalado em nenhum celular.** O motor de áudio e a ponte com o Kotlin foram testados no Linux; a interface, o decodificador e a saída de áudio real ainda não rodaram. Veja `replica/build-log.md` para o detalhe, e `replica/features.csv` para o que cada recurso já tem. Nenhum recurso está marcado como pronto (`yes`) até rodar de verdade no aparelho.

## O que a versão 0.1 faz (quando compilar e passar nos testes)

- Importar músicas do celular pelo seletor de arquivos (MP3, AAC, FLAC, WAV, OGG, o que o Android decodificar). O app não usa internet nem pede permissões.
- Biblioteca com busca e ordenação.
- Dois decks tocando ao mesmo tempo, com play/pausa, cue, volume por deck e volume geral.
- Crossfader de potência constante.
- Ondas coloridas por frequência (graves, médios, agudos) e visão geral da faixa, onde dá para tocar para pular.
- Medidores de nível.

**Fica de fora da 0.1:** equalizador, pitch, sync, loops, hot cues, efeitos, scratch, controladoras Numark, timeline e recursos inteligentes. Eles só aparecem na tela quando funcionarem. A aba Timeline aparece desabilitada, com "em breve".

## Como gerar o APK sem computador

O build precisa de um servidor (o Android SDK é grande). Duas opções gratuitas:

1. **GitHub Actions** (`.github/workflows/android.yml`). Em repositório **público** é grátis e não gasta seus minutos. Em repositório privado, usa o limite mensal da conta.
2. **Codemagic** (`codemagic.yaml`). O plano gratuito pessoal traz 500 minutos de compilação por mês.

Depois do build, baixe o arquivo `app-debug.apk` pelo navegador do celular e abra. O Android vai pedir para permitir a instalação de apps desconhecidos para o navegador. O APK de depuração já vem assinado e instala direto.

Quando o build falhar (é provável no primeiro, porque o Kotlin nunca foi compilado), mande o registro do erro e a correção sai rápido.

## Testes

```bash
tests/native/run_tests.sh   # motor de áudio: ASan, UBSan, TSan, callback sem alocação
tests/jni/run_jni.sh        # ponte JNI numa JVM, com um Oboe de mentira
```

Os dois rodam no Linux com `g++`, `python3` e um JDK. O CI roda ambos antes de compilar o APK.

## Estrutura

```
app/src/main/cpp/core/     motor de áudio (C++ puro, sem Android): decks, mixer, conversor de taxa, faixas
app/src/main/cpp/          ponte JNI e saída de áudio Oboe
app/src/main/java/...      Kotlin: telas Compose, ViewModel, decodificador, biblioteca
replica/                   análise, arquitetura, design e registro de construção
tests/                     testes do motor e da ponte JNI
```

## Limites conhecidos

- Faixas de até 15 minutos (cerca de 173 MB de memória por faixa carregada).
- Sem serviço em primeiro plano: ao sair do app, os decks pausam e o áudio fecha.
- Pré-escuta nos fones da controladora é a parte mais incerta no Android (veja `replica/architecture.md`).

## Licenças e originalidade

Código próprio. Bibliotecas: Oboe (Apache 2.0), AndroidX e Jetpack Compose (Apache 2.0). Nada de código, imagens, sons, nomes ou textos dos apps de referência. O nome e o ícone são provisórios e mudam na etapa de marca.
