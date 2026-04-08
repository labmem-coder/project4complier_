#include "ast.h"
#include "codegen.h"
#include "grammar.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "symbol_table.h"
#include "token.h"

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
            std::ostringstream _oss;                                 \
            _oss << "expected '" << (b) << "', got '" << (a) << "'"; \
            FAIL(_oss.str());                                        \
            return;                                                  \
        }                                                            \
    } while (0)

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

struct ParseResult {
    bool success = false;
    ProgramNodePtr ast;
    std::vector<ParseError> errors;
    std::vector<LexerError> lexErrors;
};

struct PipelineResult {
    ParseResult parse;
    bool semanticSuccess = false;
    std::vector<SemanticError> semanticErrors;
    std::string generatedC;
};

static ParseResult lexAndParse(const std::string& source) {
    ParseResult result;
    Grammar& g = getGrammar();

    Lexer lexer(source);
    Parser parser(g, lexer);

    result.success = parser.parse();
    result.lexErrors.assign(lexer.getErrors().begin(), lexer.getErrors().end());
    if (lexer.hasErrors()) result.success = false;

    result.ast = parser.getASTRoot();
    result.errors = parser.getErrors();
    return result;
}

static PipelineResult runFullPipeline(const std::string& source) {
    PipelineResult result;
    result.parse = lexAndParse(source);
    if (!result.parse.success || !result.parse.ast) return result;

    SemanticAnalyzer analyzer;
    result.semanticSuccess = analyzer.analyze(result.parse.ast);
    result.semanticErrors = analyzer.getErrors();
    if (!result.semanticSuccess) return result;

    CCodeGenerator codegen;
    result.generatedC = codegen.generate(result.parse.ast);
    return result;
}

static bool containsErrorMessage(const std::vector<SemanticError>& errors,
                                 const std::string& fragment) {
    for (const auto& err : errors) {
        if (err.message.find(fragment) != std::string::npos) return true;
    }
    return false;
}

static std::string normalizeCode(const std::string& code) {
    std::istringstream iss(code);
    std::string line;
    std::string normalized;
    while (std::getline(iss, line)) {
        size_t start = line.find_first_not_of(" \t\r");
        size_t end = line.find_last_not_of(" \t\r");
        std::string trimmed = (start == std::string::npos)
            ? ""
            : line.substr(start, end - start + 1);
        if (trimmed.empty()) continue;
        if (!normalized.empty()) normalized += "\n";
        normalized += trimmed;
    }
    return normalized;
}

// ---------------------------------------------------------------------------
// Lexer tests
// ---------------------------------------------------------------------------
static void testLexerCommentsAndStrings() {
    TEST("Lexer: // comments and string literals");
    auto tokens = tokenizeSource(
        "x // comment\n"
        "'Hello, world!'\n");
    EXPECT_EQ(tokens[0].type, TokenType::ID);
    EXPECT_EQ(tokens[0].lexeme, std::string("x"));
    EXPECT_EQ(tokens[1].type, TokenType::STRING);
    EXPECT_EQ(tokens[1].lexeme, std::string("'Hello, world!'"));
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
    PASS();
}

static void testLexerCharLiteral() {
    TEST("Lexer: character literal remains LETTER");
    auto tokens = tokenizeSource("'A'");
    EXPECT_EQ(tokens[0].type, TokenType::LETTER);
    EXPECT_EQ(tokens[0].lexeme, std::string("'A'"));
    PASS();
}

static void testLexerRejectsTrailingDotReal() {
    TEST("Lexer: reject real literal without fractional digits");
    Lexer lexer("123.");
    auto tokens = lexer.tokenize();
    EXPECT_TRUE(lexer.hasErrors());
    EXPECT_EQ(tokens.back().type, TokenType::END_OF_FILE);
    PASS();
}

// ---------------------------------------------------------------------------
// Symbol table tests
// ---------------------------------------------------------------------------
static void testSymbolTableScopes() {
    TEST("Symbol table: declare, shadow, and scope lookup");
    SymbolTable table;
    table.enterScope();

    Symbol global;
    global.name = "x";
    global.kind = SymbolKind::Variable;
    global.type = TypeInfo::makeSimple("integer");
    EXPECT_TRUE(table.declare(global));
    EXPECT_TRUE(!table.declare(global));
    EXPECT_TRUE(table.lookup("x") != nullptr);
    EXPECT_TRUE(table.lookupCurrent("x") != nullptr);

    table.enterScope();
    Symbol local;
    local.name = "x";
    local.kind = SymbolKind::Variable;
    local.type = TypeInfo::makeSimple("real");
    EXPECT_TRUE(table.declare(local));
    EXPECT_TRUE(table.lookup("x") != nullptr);
    EXPECT_EQ(table.lookup("x")->type.baseType, std::string("real"));
    table.exitScope();

    EXPECT_TRUE(table.lookup("x") != nullptr);
    EXPECT_EQ(table.lookup("x")->type.baseType, std::string("integer"));
    table.exitScope();
    PASS();
}

// ---------------------------------------------------------------------------
// Parse / AST tests
// ---------------------------------------------------------------------------
static void testSimpleProgram() {
    TEST("Parse: simple program");
    auto r = lexAndParse(
        "program test(input, output);\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_EQ(r.ast->name, std::string("test"));
    EXPECT_EQ(r.ast->parameters.size(), size_t(2));
    PASS();
}

static void testZeroArgumentFunctionCall() {
    TEST("Parse: zero-argument function call");
    auto r = lexAndParse(
        "program test;\n"
        "function getValue: integer;\n"
        "begin\n"
        "  getValue := 42\n"
        "end;\n"
        "begin\n"
        "  write(getValue())\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    PASS();
}

static void testWhileStatement() {
    TEST("Parse/Semantic/Codegen: while statement");
    auto r = runFullPipeline(
        "program main;\n"
        "var i: integer;\n"
        "begin\n"
        "  i := 0;\n"
        "  while i < 3 do\n"
        "    i := i + 1;\n"
        "  write(i)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    EXPECT_TRUE(normalizeCode(r.generatedC).find("while ((i < 3))") != std::string::npos);
    PASS();
}

static void testBareZeroArgumentFunctionValue() {
    TEST("Parse/Semantic: zero-argument function used without parentheses");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: integer;\n"
        "function defn: integer;\n"
        "begin\n"
        "  defn := 4\n"
        "end;\n"
        "begin\n"
        "  a := defn;\n"
        "  write(a)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    EXPECT_TRUE(normalizeCode(r.generatedC).find("a = defn();") != std::string::npos);
    PASS();
}

static void testAstForSubprogramBodyAndParams() {
    TEST("AST: subprogram parameters and body are constructed");
    auto r = lexAndParse(
        "program test;\n"
        "function func(p, x: integer): integer;\n"
        "begin\n"
        "  p := p - 1;\n"
        "  func := p\n"
        "end;\n"
        "begin\n"
        "  write(func(1, 2))\n"
        "end.\n"
    );
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.ast != nullptr);
    EXPECT_TRUE(r.ast->block != nullptr);
    EXPECT_EQ(r.ast->block->subprogramDecls.size(), size_t(1));
    auto sub = std::dynamic_pointer_cast<SubprogramDeclNode>(r.ast->block->subprogramDecls[0]);
    EXPECT_TRUE(sub != nullptr);
    EXPECT_EQ(sub->parameters.size(), size_t(1));
    auto param = std::dynamic_pointer_cast<ParamDeclNode>(sub->parameters[0]);
    EXPECT_TRUE(param != nullptr);
    EXPECT_EQ(param->names.size(), size_t(2));
    EXPECT_TRUE(sub->body != nullptr);
    EXPECT_TRUE(sub->body->compoundStmt != nullptr);
    PASS();
}

// ---------------------------------------------------------------------------
// Semantic tests
// ---------------------------------------------------------------------------
static void testSemanticPassForFunctionArguments() {
    TEST("Semantic: function arguments are counted correctly");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: integer;\n"
        "function func(p: integer): integer;\n"
        "begin\n"
        "  p := p - 1;\n"
        "  func := p\n"
        "end;\n"
        "begin\n"
        "  a := func(10)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    PASS();
}

static void testSemanticDuplicateDeclarations() {
    TEST("Semantic: duplicate declarations are reported");
    auto r = runFullPipeline(
        "program main;\n"
        "const A = 1;\n"
        "      A = 2;\n"
        "var x: integer;\n"
        "    x: integer;\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Duplicate constant declaration"));
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Duplicate variable declaration"));
    PASS();
}

static void testSemanticTypeMismatch() {
    TEST("Semantic: assignment type mismatch is reported");
    auto r = runFullPipeline(
        "program main;\n"
        "var x: integer;\n"
        "begin\n"
        "  x := 'ABC'\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Type mismatch in assignment"));
    PASS();
}

static void testSemanticUndeclaredIdentifier() {
    TEST("Semantic: undeclared identifier is reported");
    auto r = runFullPipeline(
        "program main;\n"
        "begin\n"
        "  write(x)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Undeclared identifier"));
    PASS();
}

static void testSemanticConstAssignment() {
    TEST("Semantic: constant assignment is rejected");
    auto r = runFullPipeline(
        "program main;\n"
        "const x = 1;\n"
        "begin\n"
        "  x := 2\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Cannot assign to constant 'x'"));
    PASS();
}

static void testSemanticArgumentTypeMismatch() {
    TEST("Semantic: argument type mismatch is reported");
    auto r = runFullPipeline(
        "program main;\n"
        "function f(x: integer): integer;\n"
        "begin\n"
        "  f := x\n"
        "end;\n"
        "begin\n"
        "  write(f('A'))\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Argument 1 of 'f' expects integer, got char"));
    PASS();
}

static void testSemanticByReferenceRequiresVariable() {
    TEST("Semantic: by-reference argument must be a variable");
    auto r = runFullPipeline(
        "program main;\n"
        "procedure p(var x: integer);\n"
        "begin\n"
        "end;\n"
        "begin\n"
        "  p(1)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Argument 1 of 'p' must be a variable"));
    PASS();
}

static void testSemanticInvalidArrayBounds() {
    TEST("Semantic: invalid array bounds are reported");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: array[5..1] of integer;\n"
        "begin\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Invalid array bounds in declaration"));
    PASS();
}

static void testSemanticArrayIndexOutOfBounds() {
    TEST("Semantic: static array index out of bounds is reported");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: array[1..3] of integer;\n"
        "begin\n"
        "  write(a[4])\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Array index 4 out of bounds"));
    PASS();
}

static void testSemanticMissingFunctionReturn() {
    TEST("Semantic: missing function return assignment is reported");
    auto r = runFullPipeline(
        "program main;\n"
        "function f: integer;\n"
        "begin\n"
        "end;\n"
        "begin\n"
        "  write(1)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Function 'f' has no return assignment"));
    PASS();
}

static void testSemanticDuplicateCaseLabel() {
    TEST("Semantic: duplicate case labels are reported");
    auto r = runFullPipeline(
        "program main;\n"
        "var i: integer;\n"
        "begin\n"
        "  case i of\n"
        "    1: write('A');\n"
        "    1: write('B')\n"
        "  end\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Duplicate case label '1'"));
    PASS();
}

static void testSemanticInvalidOperatorOperands() {
    TEST("Semantic: invalid operator operands are reported");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: integer;\n"
        "begin\n"
        "  a := 'A' + 1\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "Operator '+' requires numeric operands"));
    PASS();
}

static void testSemanticWhileConditionMustBeBoolean() {
    TEST("Semantic: while condition must be boolean");
    auto r = runFullPipeline(
        "program main;\n"
        "var i: integer;\n"
        "begin\n"
        "  i := 0;\n"
        "  while i do\n"
        "    i := i + 1\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(!r.semanticSuccess);
    EXPECT_TRUE(containsErrorMessage(r.semanticErrors, "While condition must be boolean"));
    PASS();
}

// ---------------------------------------------------------------------------
// Code generation comparison tests
// ---------------------------------------------------------------------------
static void testCodegenSimpleProgram() {
    TEST("Codegen: simple assignment and write");
    auto r = runFullPipeline(
        "program main;\n"
        "var a: integer;\n"
        "begin\n"
        "  a := 3;\n"
        "  write(a)\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);

    const std::string expected =
        "#include <stdio.h>\n"
        "int a;\n"
        "int main() {\n"
        "    a = 3;\n"
        "    printf(\"%d\", a);\n"
        "    return 0;\n"
        "}";
    EXPECT_EQ(normalizeCode(r.generatedC), normalizeCode(expected));
    PASS();
}

static void testCodegenFunctionCall() {
    TEST("Codegen: function body and call are emitted");
    auto r = runFullPipeline(
        "program main;\n"
        "function dec1(p: integer): integer;\n"
        "begin\n"
        "  p := p - 1;\n"
        "  dec1 := p\n"
        "end;\n"
        "begin\n"
        "  write(dec1(3))\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);

    const std::string expected =
        "#include <stdio.h>\n"
        "int dec1(int p) {\n"
        "    int dec1_result;\n"
        "    p = (p - 1);\n"
        "    dec1_result = p;\n"
        "    return dec1_result;\n"
        "}\n"
        "int main() {\n"
        "    printf(\"%d\", dec1(3));\n"
        "    return 0;\n"
        "}";
    EXPECT_EQ(normalizeCode(r.generatedC), normalizeCode(expected));
    PASS();
}

static void testCodegenStringLiteral() {
    TEST("Codegen: writeln with string literal");
    auto r = runFullPipeline(
        "program main;\n"
        "begin\n"
        "  writeln('Hello, world!')\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);

    const std::string expected =
        "#include <stdio.h>\n"
        "int main() {\n"
        "    printf(\"%s\\n\", \"Hello, world!\");\n"
        "    return 0;\n"
        "}";
    EXPECT_EQ(normalizeCode(r.generatedC), normalizeCode(expected));
    PASS();
}

// ---------------------------------------------------------------------------
// Misc integration tests
// ---------------------------------------------------------------------------
static void testComments() {
    TEST("Integration: comments are skipped across the pipeline");
    auto r = runFullPipeline(
        "{ Pascal-S program with comments }\n"
        "program test;\n"
        "// line comment before declarations\n"
        "var x : integer; (* inline comment *)\n"
        "begin\n"
        "  x := 42; // line comment after statement\n"
        "  { another comment }\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    PASS();
}

static void testCaseInsensitive() {
    TEST("Integration: case-insensitive keywords in full pipeline");
    auto r = runFullPipeline(
        "PROGRAM Test(Input, Output);\n"
        "VAR X : INTEGER;\n"
        "BEGIN\n"
        "  X := 10\n"
        "END.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    PASS();
}

static void testExtendedLanguageFeatures() {
    TEST("Integration: string type, case, break, continue");
    auto r = runFullPipeline(
        "program ExtendedPascalTest;\n"
        "var\n"
        "  str1, str2: string;\n"
        "  ch: char;\n"
        "  i: integer;\n"
        "function GetNumber: integer;\n"
        "begin\n"
        "  GetNumber := 42\n"
        "end;\n"
        "begin\n"
        "  str1 := 'Hello';\n"
        "  ch := '!';\n"
        "  write(str1);\n"
        "  i := 2;\n"
        "  case i of\n"
        "    1: write('One');\n"
        "    2: begin\n"
        "         write('Two');\n"
        "         break\n"
        "       end;\n"
        "    3: write('Three')\n"
        "  end;\n"
        "  for i := 1 to 5 do\n"
        "  begin\n"
        "    if i = 3 then continue;\n"
        "    write(i)\n"
        "  end\n"
        "end.\n"
    );
    EXPECT_TRUE(r.parse.success);
    EXPECT_TRUE(r.semanticSuccess);
    EXPECT_TRUE(normalizeCode(r.generatedC).find("const char* str1;") != std::string::npos);
    EXPECT_TRUE(normalizeCode(r.generatedC).find("switch (i)") != std::string::npos);
    EXPECT_TRUE(normalizeCode(r.generatedC).find("continue;") != std::string::npos);
    PASS();
}

int main() {
    std::cout << "===== Consolidated Pascal-S Integration Tests =====\n\n";

    testLexerCommentsAndStrings();
    testLexerCharLiteral();
    testLexerRejectsTrailingDotReal();
    testSymbolTableScopes();

    testSimpleProgram();
    testZeroArgumentFunctionCall();
    testWhileStatement();
    testBareZeroArgumentFunctionValue();
    testAstForSubprogramBodyAndParams();

    testSemanticPassForFunctionArguments();
    testSemanticDuplicateDeclarations();
    testSemanticTypeMismatch();
    testSemanticUndeclaredIdentifier();
    testSemanticConstAssignment();
    testSemanticArgumentTypeMismatch();
    testSemanticByReferenceRequiresVariable();
    testSemanticInvalidArrayBounds();
    testSemanticArrayIndexOutOfBounds();
    testSemanticMissingFunctionReturn();
    testSemanticDuplicateCaseLabel();
    testSemanticInvalidOperatorOperands();
    testSemanticWhileConditionMustBeBoolean();

    testCodegenSimpleProgram();
    testCodegenFunctionCall();
    testCodegenStringLiteral();

    testComments();
    testCaseInsensitive();
    testExtendedLanguageFeatures();

    std::cout << "\n===== Results: " << passedTests << "/" << totalTests << " passed =====\n";
    return (passedTests == totalTests) ? 0 : 1;
}
