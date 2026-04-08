#!/bin/bash
# =============================================================================
# Pascal-S Unified Test Script
# Unified entry point:
#   1. Run consolidated integration tests (lexer + parser + semantic + symbol
#      table + codegen comparison)
#   2. Run compiler corpus tests over tests/correct_test, error_test_bison,
#      and error_test_lex
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

COMPILER="$ROOT_DIR/src/pascal_compiler.exe"
INTEGRATION_TEST="$ROOT_DIR/tests/test_integration.exe"

PASS=0
FAIL=0
TOTAL=0
FAILURES=""

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

record_result() {
    local ok="$1"
    local label="$2"
    local category="$3"
    local reason="$4"
    local output="$5"

    TOTAL=$((TOTAL + 1))
    if [ "$ok" = "yes" ]; then
        PASS=$((PASS + 1))
        printf "  ${GREEN}PASS${NC}  %-25s [%s]\n" "$label" "$category"
    else
        FAIL=$((FAIL + 1))
        printf "  ${RED}FAIL${NC}  %-25s [%s] - %s\n" "$label" "$category" "$reason"
        FAILURES="${FAILURES}\n--- ${category}/${label} ---\n${reason}\nOutput:\n${output}\n"
    fi
}

require_binary() {
    local path="$1"
    local label="$2"
    if [ ! -f "$path" ]; then
        echo -e "${RED}Missing required binary:${NC} $path"
        echo "Build $label first, then re-run this script."
        exit 1
    fi
}

run_executable_test() {
    local path="$1"
    local label="$2"
    local category="$3"

    output=$("$path" 2>&1)
    exit_code=$?

    if [ "$exit_code" -eq 0 ]; then
        record_result "yes" "$label" "$category" "" "$output"
    else
        record_result "no" "$label" "$category" "test executable returned non-zero exit code" "$output"
    fi
}

run_corpus_test() {
    local file="$1"
    local expect_success="$2"  # yes / no
    local category="$3"
    local basename
    basename=$(basename "$file")

    output=$("$COMPILER" "$file" 2>&1)
    exit_code=$?

    if [ "$exit_code" -eq 0 ]; then
        run_ok="yes"
    else
        run_ok="no"
    fi

    if [ "$expect_success" = "$run_ok" ]; then
        record_result "yes" "$basename" "$category" "" "$output"
    else
        if [ "$expect_success" = "yes" ]; then
            reason="expected SUCCESS but got FAILURE"
        else
            reason="expected FAILURE but got SUCCESS"
        fi
        record_result "no" "$basename" "$category" "$reason" "$output"
    fi
}

echo "=============================================="
echo "  Pascal-S Unified Test Suite"
echo "=============================================="
echo ""

require_binary "$INTEGRATION_TEST" "test_integration.exe"
require_binary "$COMPILER" "pascal_compiler.exe"

echo -e "${YELLOW}[1/2] Consolidated executable tests${NC}"
run_executable_test "$INTEGRATION_TEST" "test_integration.exe" "executable"
echo ""

echo -e "${YELLOW}[2/2] Compiler corpus tests${NC}"

echo -e "${YELLOW}  - correct_test/ (expect: full pipeline success)${NC}"
for f in "$ROOT_DIR"/tests/correct_test/*.pas; do
    run_corpus_test "$f" "yes" "correct_test"
done
echo ""

echo -e "${YELLOW}  - error_test_bison/ (expect: compiler failure)${NC}"
for f in "$ROOT_DIR"/tests/error_test_bison/*.pas; do
    run_corpus_test "$f" "no" "error_test_bison"
done
echo ""

echo -e "${YELLOW}  - error_test_lex/ (expect: compiler failure)${NC}"
for f in "$ROOT_DIR"/tests/error_test_lex/*.pas; do
    run_corpus_test "$f" "no" "error_test_lex"
done
echo ""

echo "=============================================="
printf "  Total: %d  |  ${GREEN}Passed: %d${NC}  |  ${RED}Failed: %d${NC}\n" "$TOTAL" "$PASS" "$FAIL"
echo "=============================================="

if [ "$FAIL" -gt 0 ]; then
    echo ""
    echo -e "${RED}========== FAILURE DETAILS ==========${NC}"
    echo -e "$FAILURES"
    echo -e "${RED}=====================================${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}All tests passed!${NC}"
exit 0
