#!/usr/bin/env bash
# Compila e roda os testes do motor de áudio no Linux. Precisa de g++ e nada mais.
set -euo pipefail
cd "$(dirname "$0")"
OUT="${TMPDIR:-/tmp}/djf-tests"
mkdir -p "$OUT"
CORE=../../app/src/main/cpp/core/engine.cpp
WARN="-std=c++17 -Wall -Wextra -Werror"

echo "== 1/4 lógica do motor (ASan + UBSan) =="
g++ $WARN -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    test_engine.cpp $CORE -o "$OUT/test_engine" -pthread
"$OUT/test_engine"

echo; echo "== 2/4 callback sem alocação (sem sanitizers) =="
g++ $WARN -O2 -DCOUNT_ALLOCS test_realtime.cpp $CORE -o "$OUT/test_noalloc" -pthread
"$OUT/test_noalloc"

echo; echo "== 3/4 corrida de threads (TSan) =="
g++ $WARN -O1 -g -fsanitize=thread -DTHREAD_TEST test_realtime.cpp $CORE -o "$OUT/test_threads" -pthread
"$OUT/test_threads"
echo; echo "== 4/4 testes extras: conversor de taxa, faixa e motor em escala (ASan + UBSan + TSan) =="
g++ $WARN -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    test_core_extra.cpp $CORE -o "$OUT/test_extra_asan" -pthread
"$OUT/test_extra_asan"
g++ $WARN -O1 -g -fsanitize=thread test_core_extra.cpp $CORE -o "$OUT/test_extra_tsan" -pthread
"$OUT/test_extra_tsan"
echo; echo "TUDO OK"
