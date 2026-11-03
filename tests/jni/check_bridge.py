#!/usr/bin/env python3
"""Confere que NativeEngine.kt e native_bridge.cpp concordam em nomes, tipos e quantidade de parâmetros,
e gera o equivalente em Java (NativeEngine.java) para o teste de fumaça rodar numa JVM comum.

Uso: check_bridge.py <NativeEngine.kt> <native_bridge.cpp> <pasta de saída>
"""
import re
import sys
from pathlib import Path

KT_TO_JAVA = {"Int": "int", "Long": "long", "Float": "float", "Boolean": "boolean",
              "ByteBuffer": "ByteBuffer", "IntArray": "int[]", "LongArray": "long[]", "ByteArray": "byte[]",
              "Unit": "void"}
KT_TO_JNI = {"Int": "jint", "Long": "jlong", "Float": "jfloat", "Boolean": "jboolean",
             "ByteBuffer": "jobject", "IntArray": "jintArray", "LongArray": "jlongArray",
             "ByteArray": "jbyteArray", "Unit": "void"}


def parse_kotlin(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    out = []
    for m in re.finditer(r"@JvmStatic\s+external\s+fun\s+(\w+)\s*\((.*?)\)\s*(?::\s*(\w+))?\s*(?=@|\n\s*(?:/|\}|//|\w)|$)",
                         text, flags=re.S):
        name, params, ret = m.group(1), m.group(2), m.group(3) or "Unit"
        plist = []
        for p in [x.strip() for x in params.replace("\n", " ").split(",") if x.strip()]:
            pm = re.match(r"(\w+)\s*:\s*(\w+)", p)
            plist.append((pm.group(1), pm.group(2)))
        out.append((name, plist, ret))
    return out


def parse_cpp(text):
    out = {}
    for m in re.finditer(r"JNI\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*\((.*?)\)\s*\{", text, flags=re.S):
        ret, name, params = m.group(1), m.group(2), m.group(3)
        parts = [x.strip() for x in params.replace("\n", " ").split(",") if x.strip()]
        # os dois primeiros são JNIEnv* e jclass
        types = [re.match(r"(\w+)", p).group(1) for p in parts[2:]]
        out[name] = (ret, types)
    return out


def main():
    kt_path, cpp_path, outdir = sys.argv[1:4]
    kt = parse_kotlin(Path(kt_path).read_text())
    cpp = parse_cpp(Path(cpp_path).read_text())
    problems = []
    kt_names = {n for n, _, _ in kt}
    for name, params, ret in kt:
        if name not in cpp:
            problems.append(f"Kotlin declara {name}, mas o C++ não tem a função")
            continue
        cret, ctypes = cpp[name]
        want_ret = KT_TO_JNI[ret]
        want_params = [KT_TO_JNI[t] for _, t in params]
        if cret != want_ret:
            problems.append(f"{name}: retorno Kotlin {ret} -> {want_ret}, C++ tem {cret}")
        if ctypes != want_params:
            problems.append(f"{name}: parâmetros Kotlin {want_params}, C++ tem {ctypes}")
    for name in cpp:
        if name not in kt_names:
            problems.append(f"C++ define {name}, mas o Kotlin não declara")
    if len(kt) < 10:
        problems.append(f"só {len(kt)} funções lidas do Kotlin, o leitor deve estar errado")

    if problems:
        print("PONTE INCONSISTENTE:")
        for p in problems:
            print("  -", p)
        sys.exit(1)

    lines = ["package app.djfusion.audio;", "import java.nio.ByteBuffer;",
             "public final class NativeEngine {", '  static { System.loadLibrary("djfusion"); }']
    for name, params, ret in kt:
        args = ", ".join(f"{KT_TO_JAVA[t]} {n}" for n, t in params)
        lines.append(f"  public static native {KT_TO_JAVA[ret]} {name}({args});")
    lines.append("}")
    out = Path(outdir) / "app" / "djfusion" / "audio"
    out.mkdir(parents=True, exist_ok=True)
    (out / "NativeEngine.java").write_text("\n".join(lines) + "\n")
    print(f"ponte consistente: {len(kt)} funções, nomes, tipos e retornos iguais no Kotlin e no C++")


main()
