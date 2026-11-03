#!/usr/bin/env bash
# Roda a ponte JNI de verdade numa JVM do Linux, com um Oboe de mentira (FakeOboe.h) no lugar do áudio.
# Valida: ponte Kotlin <-> C++, construção de faixas, decks, crossfader, limites. Não valida o Oboe real.
set -euo pipefail
cd "$(dirname "$0")"
ROOT="$(cd ../.. && pwd)"
OUT="${TMPDIR:-/tmp}/djf-jni"
rm -rf "$OUT"; mkdir -p "$OUT/inc/oboe" "$OUT/inc/android" "$OUT/java" "$OUT/classes"

python3 -I check_bridge.py "$ROOT/app/src/main/java/app/djfusion/audio/NativeEngine.kt" \
        "$ROOT/app/src/main/cpp/native_bridge.cpp" "$OUT/java"

cp FakeOboe.h "$OUT/inc/oboe/Oboe.h"
cp fake_android_log.h "$OUT/inc/android/log.h"

JH="$(dirname "$(dirname "$(readlink -f "$(which javac)")")")"
javac -d "$OUT/classes" "$OUT/java/app/djfusion/audio/NativeEngine.java" JniSmoke.java

g++ -std=c++17 -O2 -Wall -Wextra -fPIC -shared -pthread \
    -I"$OUT/inc" -I"$JH/include" -I"$JH/include/linux" -I"$ROOT/app/src/main/cpp" \
    "$ROOT/app/src/main/cpp/native_bridge.cpp" "$ROOT/app/src/main/cpp/core/engine.cpp" \
    -o "$OUT/libdjfusion.so"

java -Xcheck:jni -Djava.library.path="$OUT" -cp "$OUT/classes" JniSmoke
