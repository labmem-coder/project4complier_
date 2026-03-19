#include "lexer.h"
#include "token.h"
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

#define EXPECT_EQ(a, b)                                              \
    do {                                                             \
        if ((a) != (b)) {                                            \
            std::ostringstream _oss;                                  \
            _oss << "expected '" << (b) << "', got '" << (a) << "'"; \
            FAIL(_oss.str());                                        \
            return;                                                  \
        }                                                            \
    } while (0)

#define EXPECT_TRUE(cond)                                  \
    do {                                                   \
        if (!(cond)) {                                     \
            FAIL(#cond " is false");                       \
            return;                                        \
        }                                                  \
    } while (0)

// ---------------------------------------------------------------------------
// Individual token tests
// ---------------------------------------------------------------------------
static void testKeywords() {
    TEST("Keywords");
    auto tokens = tokenizeSource("program var const procedure function begin end "
                                 "if then else for to do read write while repeat until "
                                 "integer real boolean char array of not div mod and or");
    // Last token is EOF
    EXPECT_EQ(tokens.back().type, TokenType::END_OF_FILE);

    // Check all keywords in order (27 keywords + EOF = 28 tokens)
    TokenType expected[] = {
        TokenType::PROGRAM, TokenType::VAR, TokenType::CONST,
        TokenType::PROCEDURE, TokenType::FUNCTION,
        TokenType::BEGIN, TokenType::END,
        TokenType::IF, TokenType::THEN, TokenType::ELSE,
        TokenType::FOR, TokenType::TO, TokenType::DO,
        TokenType::READ, TokenType::WRITE,
        TokenType::WHILE, TokenType::REPEAT, TokenType::UNTIL,
        TokenType::INTEGER_KW, TokenType::REAL_KW, TokenType::BOOLEAN_KW, TokenType::CHAR_KW,
        TokenType::ARRAY, TokenType::OF, TokenType::NOT,
        TokenType::DIV_KW, TokenType::MOD, TokenType::AND_KW, TokenType::OR_KW,
        TokenType::END_OF_FILE
    };
    EXPECT_EQ(tokens.size(), size_t(30));
    for (size_t i = 0; i < 30; ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
    PASS();
}

static void testCaseInsensitiveKeywords() {
    TEST("Case-insensitive keywords");
    auto tokens = tokenizeSource("PROGRAM Program pRoGrAm BEGIN End");
    EXPECT_EQ(tokens[0].type, TokenType::PROGRAM);
    EXPECT_EQ(tokens[1].type, TokenType::PROGRAM);
    EXPECT_EQ(tokens[2].type, TokenType::PROGRAM);
    EXPECT_EQ(tokens[3].type, TokenType::BEGIN);
    EXPECT_EQ(tokens[4].type, TokenType::END);
    // Keywords should be stored in lowercase
    EXPECT_EQ(tokens[0].lexeme, std::string("program"));
    EXPECT_EQ(tokens[1].lexeme, std::string("program"));
    PASS();
}

static void testIdentifiers() {
    TEST("Identifiers");
    auto tokens = tokenizeSource("myVar x_1 _temp counter123");
    EXPECT_EQ(tokens[0].type, TokenType::ID);
    EXPECT_EQ(tokens[0].lexeme, std::string("myVar"));
    EXPECT_EQ(tokens[1].type, TokenType::ID);
    EXPECT_EQ(tokens[1].lexeme, std::string("x_1"));
    EXPECT_EQ(tokens[2].type, TokenType::ID);
    EXPECT_EQ(tokens[3].type, TokenType::ID);
    PASS();
}

static void testIntegerNumbers() {
    TEST("Integer numbers");
    auto tokens = tokenizeSource("0 42 100 999");
    EXPECT_EQ(tokens[0].type, TokenType::NUM);
    EXPECT_EQ(tokens[0].lexeme, std::string("0"));
    EXPECT_EQ(tokens[1].type, TokenType::NUM);
    EXPECT_EQ(tokens[1].lexeme, std::string("42"));
    EXPECT_EQ(tokens[2].type, TokenType::NUM);
    EXPECT_EQ(tokens[2].lexeme, std::string("100"));
    EXPECT_EQ(tokens[3].type, TokenType::NUM);
    EXPECT_EQ(tokens[3].lexeme, std::string("999"));
    PASS();
}

static void testRealNumbers() {
    TEST("Real numbers");
    auto tokens = tokenizeSource("3.14 0.5 100.0");
    EXPECT_EQ(tokens[0].type, TokenType::NUM);
    EXPECT_EQ(tokens[0].lexeme, std::string("3.14"));
    EXPECT_EQ(tokens[1].type, TokenType::NUM);
    EXPECT_EQ(tokens[1].lexeme, std::string("0.5"));
    EXPECT_EQ(tokens[2].type, TokenType::NUM);
    EXPECT_EQ(tokens[2].lexeme, std::string("100.0"));
    PASS();
}

static void testDotDotNotReal() {
    TEST("1..10 should be NUM DOTDOT NUM, not a real number");
    auto tokens = tokenizeSource("1..10");
    EXPECT_EQ(tokens[0].type, TokenType::NUM);
    EXPECT_EQ(tokens[0].lexeme, std::string("1"));
    EXPECT_EQ(tokens[1].type, TokenType::DOTDOT);
    EXPECT_EQ(tokens[2].type, TokenType::NUM);
    EXPECT_EQ(tokens[2].lexeme, std::string("10"));
    PASS();
}

static void testCharLiteral() {
    TEST("Character literals");
    auto tokens = tokenizeSource("'A' 'z' '0'");
    EXPECT_EQ(tokens[0].type, TokenType::LETTER);
    EXPECT_EQ(tokens[0].lexeme, std::string("'A'"));
    EXPECT_EQ(tokens[1].type, TokenType::LETTER);
    EXPECT_EQ(tokens[1].lexeme, std::string("'z'"));
    EXPECT_EQ(tokens[2].type, TokenType::LETTER);
    EXPECT_EQ(tokens[2].lexeme, std::string("'0'"));
    PASS();
}

static void testOperators() {
    TEST("Operators");
    auto tokens = tokenizeSource(":= + - * / = <> < <= > >=");
    TokenType expected[] = {
        TokenType::ASSIGN, TokenType::PLUS, TokenType::MINUS,
        TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::EQ,
        TokenType::NE, TokenType::LT, TokenType::LE,
        TokenType::GT, TokenType::GE, TokenType::END_OF_FILE
    };
    for (size_t i = 0; i < 12; ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
    PASS();
}

static void testDelimiters() {
    TEST("Delimiters");
    auto tokens = tokenizeSource("( ) [ ] ; : , . ..");
    TokenType expected[] = {
        TokenType::LPAREN, TokenType::RPAREN,
        TokenType::LBRACKET, TokenType::RBRACKET,
        TokenType::SEMICOLON, TokenType::COLON,
        TokenType::COMMA, TokenType::DOT, TokenType::DOTDOT,
        TokenType::END_OF_FILE
    };
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
    PASS();
}

static void testBraceComment() {
    TEST("Brace comments { ... }");
    auto tokens = tokenizeSource("x { this is a comment } y");
    EXPECT_EQ(tokens.size(), size_t(3)); // x, y, EOF
    EXPECT_EQ(tokens[0].type, TokenType::ID);
    EXPECT_EQ(tokens[0].lexeme, std::string("x"));
    EXPECT_EQ(tokens[1].type, TokenType::ID);
    EXPECT_EQ(tokens[1].lexeme, std::string("y"));
    PASS();
}

static void testParenStarComment() {
    TEST("Paren-star comments (* ... *)");
    auto tokens = tokenizeSource("a (* comment *) b");
    EXPECT_EQ(tokens.size(), size_t(3)); // a, b, EOF
    EXPECT_EQ(tokens[0].type, TokenType::ID);
    EXPECT_EQ(tokens[1].type, TokenType::ID);
    PASS();
}

static void testLineAndColumn() {
    TEST("Line and column tracking");
    auto tokens = tokenizeSource("program\n  test");
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);
    EXPECT_EQ(tokens[1].line, 2);
    EXPECT_EQ(tokens[1].column, 3);
    PASS();
}

static void testUnterminatedComment() {
    TEST("Unterminated comment error");
    Lexer lexer("x { unterminated");
    auto tokens = lexer.tokenize();
    EXPECT_TRUE(lexer.hasErrors());
    EXPECT_EQ(lexer.getErrors()[0].message, std::string("Unterminated comment starting with '{'"));
    PASS();
}

static void testUnexpectedCharacter() {
    TEST("Unexpected character error");
    Lexer lexer("x @ y");
    auto tokens = lexer.tokenize();
    EXPECT_TRUE(lexer.hasErrors());
    PASS();
}

static void testEmptySource() {
    TEST("Empty source");
    auto tokens = tokenizeSource("");
    EXPECT_EQ(tokens.size(), size_t(1));
    EXPECT_EQ(tokens[0].type, TokenType::END_OF_FILE);
    PASS();
}

// ---------------------------------------------------------------------------
// Full program tokenization test
// ---------------------------------------------------------------------------
static void testFullProgram() {
    TEST("Full simple program tokenization");
    auto tokens = tokenizeSource(
        "program test(input, output);\n"
        "begin\n"
        "end.\n"
    );

    TokenType expected[] = {
        TokenType::PROGRAM, TokenType::ID, TokenType::LPAREN,
        TokenType::ID, TokenType::COMMA, TokenType::ID,
        TokenType::RPAREN, TokenType::SEMICOLON,
        TokenType::BEGIN, TokenType::END, TokenType::DOT,
        TokenType::END_OF_FILE
    };
    EXPECT_EQ(tokens.size(), size_t(12));
    for (size_t i = 0; i < 12; ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }

    EXPECT_EQ(tokens[0].lexeme, std::string("program"));
    EXPECT_EQ(tokens[1].lexeme, std::string("test"));
    PASS();
}

static void testComplexProgram() {
    TEST("Complex program tokenization");
    auto tokens = tokenizeSource(
        "program example(input, output);\n"
        "const MAX = 100;\n"
        "var arr : array[1..10] of integer;\n"
        "    i, sum : integer;\n"
        "begin\n"
        "  sum := 0;\n"
        "  for i := 1 to 10 do\n"
        "    sum := sum + arr[i];\n"
        "  write(sum)\n"
        "end.\n"
    );

    // Verify no errors
    Lexer lexer(
        "program example(input, output);\n"
        "const MAX = 100;\n"
        "var arr : array[1..10] of integer;\n"
        "    i, sum : integer;\n"
        "begin\n"
        "  sum := 0;\n"
        "  for i := 1 to 10 do\n"
        "    sum := sum + arr[i];\n"
        "  write(sum)\n"
        "end.\n"
    );
    lexer.tokenize();
    EXPECT_TRUE(!lexer.hasErrors());

    // Check a few key tokens
    EXPECT_EQ(tokens[0].type, TokenType::PROGRAM);
    EXPECT_EQ(tokens[1].type, TokenType::ID);
    EXPECT_EQ(tokens[1].lexeme, std::string("example"));

    // Find 'array' keyword
    bool foundArray = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::ARRAY) { foundArray = true; break; }
    }
    EXPECT_TRUE(foundArray);

    // Find '..' token
    bool foundDotDot = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::DOTDOT) { foundDotDot = true; break; }
    }
    EXPECT_TRUE(foundDotDot);

    // Last meaningful token before EOF should be '.'
    EXPECT_EQ(tokens[tokens.size() - 2].type, TokenType::DOT);
    EXPECT_EQ(tokens.back().type, TokenType::END_OF_FILE);
    PASS();
}

static void testTokenTypeToString() {
    TEST("tokenTypeToString coverage");
    EXPECT_EQ(tokenTypeToString(TokenType::PROGRAM), std::string("PROGRAM"));
    EXPECT_EQ(tokenTypeToString(TokenType::ID), std::string("ID"));
    EXPECT_EQ(tokenTypeToString(TokenType::NUM), std::string("NUM"));
    EXPECT_EQ(tokenTypeToString(TokenType::ASSIGN), std::string("ASSIGN"));
    EXPECT_EQ(tokenTypeToString(TokenType::END_OF_FILE), std::string("EOF"));
    PASS();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "===== Pascal-S Lexer Tests =====\n\n";

    testKeywords();
    testCaseInsensitiveKeywords();
    testIdentifiers();
    testIntegerNumbers();
    testRealNumbers();
    testDotDotNotReal();
    testCharLiteral();
    testOperators();
    testDelimiters();
    testBraceComment();
    testParenStarComment();
    testLineAndColumn();
    testUnterminatedComment();
    testUnexpectedCharacter();
    testEmptySource();
    testFullProgram();
    testComplexProgram();
    testTokenTypeToString();

    std::cout << "\n===== Results: " << passedTests << "/" << totalTests << " passed =====\n";
    return (passedTests == totalTests) ? 0 : 1;
}
