#!/bin/bash
# =============================================================================
# Pascal-S Compiler Test Script
# Tests three folders:
#   correct_test/     — expect parse SUCCESS
#   error_test_bison/ — expect parse FAILURE (syntax errors)
#   error_test_lex/   — expect LEXER errors
# =============================================================================

COMPILER="src/pascal_compiler.exe"
PASS=0
FAIL=0
TOTAL=0
FAILURES=""

# Colors (works in Git Bash / MSYS2 on Windows)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

run_test() {
    local file="$1"
    local expect_success="$2"  # "yes" or "no"
    local category="$3"
    local basename=$(basename "$file")
    TOTAL=$((TOTAL + 1))

    # Run compiler, capture stdout+stderr
    output=$("$COMPILER" "$file" 2>&1)
    exit_code=$?

    # Check if parse was successful
    if echo "$output" | grep -q "PARSE SUCCESSFUL"; then
        parse_ok="yes"
    else
        parse_ok="no"
    fi

    if [ "$expect_success" = "$parse_ok" ]; then
        PASS=$((PASS + 1))
        printf "  ${GREEN}PASS${NC}  %-25s [%s]\n" "$basename" "$category"
    else
        FAIL=$((FAIL + 1))
        if [ "$expect_success" = "yes" ]; then
            reason="expected SUCCESS but got FAILURE"
        else
            reason="expected FAILURE but got SUCCESS"
        fi
        printf "  ${RED}FAIL${NC}  %-25s [%s] — %s\n" "$basename" "$category" "$reason"
        FAILURES="${FAILURES}\n--- ${category}/${basename} ---\n${reason}\nOutput:\n${output}\n"
    fi
}

echo "=============================================="
echo "  Pascal-S Compiler Test Suite"
echo "=============================================="
echo ""

# --- 1. correct_test: all should parse successfully ---
echo -e "${YELLOW}[1/3] correct_test/ (expect: PASS)${NC}"
for f in tests/correct_test/*.pas; do
    run_test "$f" "yes" "correct_test"
done
echo ""

# --- 2. error_test_bison: all should have parse errors ---
echo -e "${YELLOW}[2/3] error_test_bison/ (expect: FAIL with syntax errors)${NC}"
for f in tests/error_test_bison/*.pas; do
    run_test "$f" "no" "error_test_bison"
done
echo ""

# --- 3. error_test_lex: all should have lexer errors ---
echo -e "${YELLOW}[3/3] error_test_lex/ (expect: FAIL with lexer errors)${NC}"
for f in tests/error_test_lex/*.pas; do
    run_test "$f" "no" "error_test_lex"
done
echo ""

# --- Summary ---
echo "=============================================="
printf "  Total: %d  |  ${GREEN}Passed: %d${NC}  |  ${RED}Failed: %d${NC}\n" "$TOTAL" "$PASS" "$FAIL"
echo "=============================================="

# --- Failure details ---
if [ "$FAIL" -gt 0 ]; then
    echo ""
    echo -e "${RED}========== FAILURE DETAILS ==========${NC}"
    echo -e "$FAILURES"
    echo -e "${RED}=====================================${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
