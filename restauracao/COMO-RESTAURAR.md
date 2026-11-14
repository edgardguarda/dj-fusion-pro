# Ponto de restauração — Edgard Guarda DJ Studio 2.6.0 (14/11/2026)

Esta pasta guarda tudo o que foi feito até a versão 2.6.0, para voltar a ela a qualquer momento.

| Arquivo | O que é |
|---|---|
| `EdgardGuardaDJStudio-2.6.0.apk` | O app pronto para instalar (versão final, assinada com a chave fixa: instala por cima sem perder dados). |
| `projeto-v2.6.0-restauracao.zip` | O código-fonte completo da 2.6.0 + `historico-git.bundle` (todo o histórico de mudanças). |

## Para voltar o app do celular à 2.6.0
Instale o APK desta pasta. Músicas, cues, playlists e ajustes ficam (mesma assinatura).

## Para voltar o projeto à 2.6.0
1. Copie `projeto-v2.6.0-restauracao.zip` para a raiz do repositório com o nome `projeto.zip`.
2. O teste automático roda sozinho e publica o APK em "Teste no emulador (último)".

## Histórico completo
`git clone historico-git.bundle meu-projeto` recria o projeto com todas as versões (tag `v2.6.0`).

## Dados do DJ (biblioteca, cues, playlists)
No app: AJUSTES › BACKUP › SALVAR BACKUP (e o backup automático ao abrir, em Documentos/Edgard Guarda DJ Studio/backups). Para voltar: RESTAURAR.
