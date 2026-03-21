#include "grammar.h"
#include "parser.h"
#include "token.h"
#include "ast.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static int totalTests = 0;
static int passedTests = 0;

#define TEST(name)                                         \
    do {                                                   \
        ++totalTests;                                      \
        std::cout << "  [TEST] " << name << " ... ";      \
    } while (0)

#define PASS()                                             \
    do {                                                   \
        ++passedTests;                                     \
        std::cout << "PASS\n";                             \
    } while (0)

#define FAIL(msg)                                          \
    do {                                                   \
        std::cout << "FAIL: " << msg << "\n";              \
    } while (0)

#define EXPECT_TRUE(cond)                                  \
    do {                                                   \
        if (!(cond)) {                                     \
            FAIL(#cond " is false");                       \
            return;                                        \
        }                                                  \
    } while (0)

#define EXPECT_EQ(a, b)                                              \
    do {                                                             \
        if ((a) != (b)) {                                            \
            std::ostringstream _oss;                                  \
            _oss << "expected '" << (b) << "', got '" << (a) << "'"; \
            FAIL(_oss.str());                                        \
            return;                                                  \
        }                                                            \
    } while (0)

// Build the grammar once and reuse
static Grammar& getGrammar() {
    static Grammar g = []() {
        Grammar g = buildPascalSGrammar();
        g.transformToLL1();
        g.computeFirstSets();
        g.computeFollowSets();
        g.buildParseTable();
        return g;
    }();
    return g;
}

// Lex + Parse via Parser(grammar, source) — lexer integrated in parser
struct ParseResult {
    bool success;
    ProgramNodePtr ast;
    std::vector<ParseError> errors;
    std::vector<LexerError> lexErrors;
};

static ParseResult lexAndParse(const std::string& source) {
    ParseResult result;
    Grammar& g = getGrammar();

    // Parser lexes internally via Parser(grammar, source)
    Parser parser(g, source);

    result.lexErrors = parser.getLexerErrors();
    if (parser.hasLexerErrors()) {
        result.success = false;
        return result;
    }

    result.success = parser.parse();
    result.ast = parser.getASTRoot();
    result.errors = parser.getErrors();
    return result;
}

// ---------------------------------------------------------------------------
// Test: simplest valid program
// ---------------------------------------------------------------------------
static void testSimpleProgram() {
    TEST("Simple program: program test(input, output); begin end.");
    auto r = lexAndParse(
        "program test(input, output);\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->name, std::string("test"));
    EXPECT_EQ(r.ast->parameters.size(), size_t(2));
    EXPECT_EQ(r.ast->parameters[0], std::string("input"));
    EXPECT_EQ(r.ast->parameters[1], std::string("output"));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: program with no parameters
// ---------------------------------------------------------------------------
static void testProgramNoParams() {
    TEST("Program without parameters");
    auto r = lexAndParse(
        "program minimal;\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->name, std::string("minimal"));
    EXPECT_TRUE(r.ast->parameters.empty());
    PASS();
}

// ---------------------------------------------------------------------------
// Test: const and var declarations
// ---------------------------------------------------------------------------
static void testConstAndVarDecl() {
    TEST("Const and var declarations");
    auto r = lexAndParse(
        "program test(input, output);\n"
        "const PI = 3;\n"
        "var x, y : integer;\n"
        "    z : real;\n"
        "begin\n"
        "  x := 10\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_TRUE(r.ast->block != nullptr);
    EXPECT_EQ(r.ast->block->constDecls.size(), size_t(1));
    EXPECT_EQ(r.ast->block->varDecls.size(), size_t(2));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: control flow (if/else, for)
// ---------------------------------------------------------------------------
static void testControlFlow() {
    TEST("Control flow: if/else and for");
    auto r = lexAndParse(
        "program test(input, output);\n"
        "var x, y : integer;\n"
        "begin\n"
        "  read(x, y);\n"
        "  if x > y then\n"
        "    write(x)\n"
        "  else\n"
        "    write(y);\n"
        "  for x := 1 to 10 do\n"
        "    y := y + x\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);

    // Check compound statement has multiple statements
    auto block = r.ast->block;
    EXPECT_TRUE(block != nullptr);
    auto compStmt = std::dynamic_pointer_cast<CompoundStmtNode>(block->compoundStmt);
    EXPECT_TRUE(compStmt != nullptr);
    // read, if, for = 3 statements
    EXPECT_EQ(compStmt->statements.size(), size_t(3));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: expressions and assignment
// ---------------------------------------------------------------------------
static void testExpressions() {
    TEST("Expressions: arithmetic with precedence");
    auto r = lexAndParse(
        "program test;\n"
        "var x : integer;\n"
        "begin\n"
        "  x := 1 + 2 * 3\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Test: unary minus is accepted at factor start and after binary minus
// ---------------------------------------------------------------------------
static void testUnaryMinusExpressions() {
    TEST("Expressions: unary minus in nested arithmetic");
    auto r = lexAndParse(
        "program main;\n"
        "var a, b, c, d, e: integer;\n"
        "begin\n"
        "  a := 5;\n"
        "  b := 5;\n"
        "  c := 1;\n"
        "  d := -2;\n"
        "  e := (d * 1 div 2) + (a - b) - -(c + 3) mod 2;\n"
        "  e := ((d mod 2 + 67) + -(a - b) - -((c + 2) mod 2))\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Test: array access
// ---------------------------------------------------------------------------
static void testArrayAccess() {
    TEST("Array variable declarations and access");
    auto r = lexAndParse(
        "program test;\n"
        "var arr : array[1..10] of integer;\n"
        "begin\n"
        "  arr[5] := 42\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Test: subprogram (function)
// ---------------------------------------------------------------------------
static void testSubprogram() {
    TEST("Subprogram: function declaration");
    auto r = lexAndParse(
        "program test(input, output);\n"
        "var a, b : integer;\n"
        "function max(x, y : integer) : integer;\n"
        "begin\n"
        "  if x > y then max := x\n"
        "  else max := y\n"
        "end;\n"
        "begin\n"
        "  read(a, b);\n"
        "  write(max(a, b))\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Test: complex program (matches test_complex.pas)
// ---------------------------------------------------------------------------
static void testComplexProgram() {
    TEST("Complex program with procedure, arrays, loops");
    auto r = lexAndParse(
        "program example(input, output);\n"
        "const MAX = 100;\n"
        "var arr : array[1..10] of integer;\n"
        "    i, sum : integer;\n"
        "procedure init(n : integer);\n"
        "var j : integer;\n"
        "begin\n"
        "  for j := 1 to n do\n"
        "    arr[j] := j * 2\n"
        "end;\n"
        "begin\n"
        "  init(10);\n"
        "  sum := 0;\n"
        "  for i := 1 to 10 do\n"
        "    sum := sum + arr[i];\n"
        "  write(sum)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->name, std::string("example"));

    auto block = r.ast->block;
    EXPECT_TRUE(block != nullptr);
    EXPECT_EQ(block->constDecls.size(), size_t(1));
    EXPECT_EQ(block->varDecls.size(), size_t(2));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: comments are properly skipped
// ---------------------------------------------------------------------------
static void testComments() {
    TEST("Comments: { } and (* *) are skipped");
    auto r = lexAndParse(
        "{ Pascal-S program with comments }\n"
        "program test(input, output);\n"
        "var x : integer; (* inline comment *)\n"
        "begin\n"
        "  x := 42 { another comment }\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->name, std::string("test"));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: character constant
// ---------------------------------------------------------------------------
static void testCharConstant() {
    TEST("Character constant in const declaration");
    auto r = lexAndParse(
        "program test;\n"
        "const CH = 'A';\n"
        "var x : integer;\n"
        "begin\n"
        "  x := 10\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->block->constDecls.size(), size_t(1));
    PASS();
}

// ---------------------------------------------------------------------------
// Test: AST output is not empty
// ---------------------------------------------------------------------------
static void testASTOutput() {
    TEST("AST print produces output");
    auto r = lexAndParse(
        "program test;\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    std::ostringstream oss;
    printAST(r.ast, oss);
    EXPECT_TRUE(oss.str().size() > 0);
    // Should contain "Program"
    EXPECT_TRUE(oss.str().find("Program") != std::string::npos);
    PASS();
}

// ---------------------------------------------------------------------------
// Test: case insensitivity in full pipeline
// ---------------------------------------------------------------------------
static void testCaseInsensitive() {
    TEST("Case-insensitive keywords in full pipeline");
    auto r = lexAndParse(
        "PROGRAM Test(Input, Output);\n"
        "VAR X : INTEGER;\n"
        "BEGIN\n"
        "  X := 10\n"
        "END.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "===== Lexer + Parser Integration Tests =====\n\n";

    testSimpleProgram();
    testProgramNoParams();
    testConstAndVarDecl();
    testControlFlow();
    testExpressions();
    testUnaryMinusExpressions();
    testArrayAccess();
    testSubprogram();
    testComplexProgram();
    testComments();
    testCharConstant();
    testASTOutput();
    testCaseInsensitive();

    std::cout << "\n===== Results: " << passedTests << "/" << totalTests << " passed =====\n";
    return (passedTests == totalTests) ? 0 : 1;
}
