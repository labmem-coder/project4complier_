#!/usr/bin/env sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
NATIVE_BIN="$SCRIPT_DIR/.pascal_compiler_native"

if [ -n "${CXX:-}" ]; then
    CXX_CMD="$CXX"
elif command -v g++ >/dev/null 2>&1; then
    CXX_CMD="g++"
elif command -v c++ >/dev/null 2>&1; then
    CXX_CMD="c++"
elif command -v clang++ >/dev/null 2>&1; then
    CXX_CMD="clang++"
else
    echo "Error: no C++ compiler found (tried \$CXX, g++, c++, clang++)." >&2
    exit 1
fi

SOURCE_FILES="
$SCRIPT_DIR/main.cpp
$SCRIPT_DIR/lexer/lexer.cpp
$SCRIPT_DIR/parser/grammar.cpp
$SCRIPT_DIR/parser/parser.cpp
$SCRIPT_DIR/semantic/symbol_table.cpp
$SCRIPT_DIR/semantic/semantic_analyzer.cpp
$SCRIPT_DIR/codegen/codegen.cpp
"

needs_build=0
if [ ! -x "$NATIVE_BIN" ]; then
    needs_build=1
else
    for src in $SOURCE_FILES; do
        if [ "$src" -nt "$NATIVE_BIN" ]; then
            needs_build=1
            break
        fi
    done
fi

if [ "$needs_build" -eq 1 ]; then
    "$CXX_CMD" -std=c++17 \
        -I"$SCRIPT_DIR/lexer" \
        -I"$SCRIPT_DIR/parser" \
        -I"$SCRIPT_DIR/semantic" \
        -I"$SCRIPT_DIR/codegen" \
        -o "$NATIVE_BIN" \
        "$SCRIPT_DIR/main.cpp" \
        "$SCRIPT_DIR/lexer/lexer.cpp" \
        "$SCRIPT_DIR/parser/grammar.cpp" \
        "$SCRIPT_DIR/parser/parser.cpp" \
        "$SCRIPT_DIR/semantic/symbol_table.cpp" \
        "$SCRIPT_DIR/semantic/semantic_analyzer.cpp" \
        "$SCRIPT_DIR/codegen/codegen.cpp"
fi

exec "$NATIVE_BIN" "$@"
