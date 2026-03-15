#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <unordered_map>
#include <vector>

// Token types for Pascal-S lexer output
enum class TokenType {
    // Keywords
    PROGRAM, VAR, CONST, PROCEDURE, FUNCTION,
    BEGIN, END, IF, THEN, ELSE, FOR, TO, DO,
    READ, WRITE, WHILE, REPEAT, UNTIL,
    INTEGER_KW, REAL_KW, BOOLEAN_KW, CHAR_KW,
    ARRAY, OF, NOT,

    // Operators
    ASSIGN,     // :=
    PLUS,       // +
    MINUS,      // -
    MULTIPLY,   // *
    DIVIDE,     // /    (real division)
    DIV_KW,     // div  (integer division)
    MOD,        // mod
    AND_KW,     // and
    OR_KW,      // or

    // Relational operators
    EQ,         // =
    NE,         // <>
    LT,         // <
    LE,         // <=
    GT,         // >
    GE,         // >=

    // Delimiters
    LPAREN,     // (
    RPAREN,     // )
    LBRACKET,   // [
    RBRACKET,   // ]
    SEMICOLON,  // ;
    COLON,      // :
    COMMA,      // ,
    DOT,        // .
    DOTDOT,     // ..

    // Literals
    NUM,        // integer or real number literal
    LETTER,     // character literal like 'a'
    ID,         // identifier

    // Special
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

// Map from token type name string to TokenType enum
inline std::unordered_map<std::string, TokenType> buildTokenNameMap() {
    return {
        {"PROGRAM", TokenType::PROGRAM}, {"VAR", TokenType::VAR},
        {"CONST", TokenType::CONST}, {"PROCEDURE", TokenType::PROCEDURE},
        {"FUNCTION", TokenType::FUNCTION}, {"BEGIN", TokenType::BEGIN},
        {"END", TokenType::END}, {"IF", TokenType::IF},
        {"THEN", TokenType::THEN}, {"ELSE", TokenType::ELSE},
        {"FOR", TokenType::FOR}, {"TO", TokenType::TO},
        {"DO", TokenType::DO}, {"READ", TokenType::READ},
        {"WRITE", TokenType::WRITE}, {"WHILE", TokenType::WHILE},
        {"REPEAT", TokenType::REPEAT}, {"UNTIL", TokenType::UNTIL},
        {"INTEGER", TokenType::INTEGER_KW}, {"REAL", TokenType::REAL_KW},
        {"BOOLEAN", TokenType::BOOLEAN_KW}, {"CHAR", TokenType::CHAR_KW},
        {"ARRAY", TokenType::ARRAY}, {"OF", TokenType::OF},
        {"NOT", TokenType::NOT},
        {"ASSIGN", TokenType::ASSIGN}, {"PLUS", TokenType::PLUS},
        {"MINUS", TokenType::MINUS}, {"MULTIPLY", TokenType::MULTIPLY},
        {"DIVIDE", TokenType::DIVIDE}, {"DIV", TokenType::DIV_KW},
        {"MOD", TokenType::MOD}, {"AND", TokenType::AND_KW},
        {"OR", TokenType::OR_KW},
        {"EQ", TokenType::EQ}, {"NE", TokenType::NE},
        {"LT", TokenType::LT}, {"LE", TokenType::LE},
        {"GT", TokenType::GT}, {"GE", TokenType::GE},
        {"LPAREN", TokenType::LPAREN}, {"RPAREN", TokenType::RPAREN},
        {"LBRACKET", TokenType::LBRACKET}, {"RBRACKET", TokenType::RBRACKET},
        {"SEMICOLON", TokenType::SEMICOLON}, {"COLON", TokenType::COLON},
        {"COMMA", TokenType::COMMA}, {"DOT", TokenType::DOT},
        {"DOTDOT", TokenType::DOTDOT},
        {"NUM", TokenType::NUM}, {"LETTER", TokenType::LETTER},
        {"ID", TokenType::ID},
        {"END_OF_FILE", TokenType::END_OF_FILE}, {"EOF", TokenType::END_OF_FILE}
    };
}

// Map TokenType enum to the terminal name used in grammar
inline std::string tokenTypeToTerminal(TokenType t) {
    static const std::unordered_map<TokenType, std::string> m = {
        {TokenType::PROGRAM, "program"}, {TokenType::VAR, "var"},
        {TokenType::CONST, "const"}, {TokenType::PROCEDURE, "procedure"},
        {TokenType::FUNCTION, "function"}, {TokenType::BEGIN, "begin"},
        {TokenType::END, "end"}, {TokenType::IF, "if"},
        {TokenType::THEN, "then"}, {TokenType::ELSE, "else"},
        {TokenType::FOR, "for"}, {TokenType::TO, "to"},
        {TokenType::DO, "do"}, {TokenType::READ, "read"},
        {TokenType::WRITE, "write"}, {TokenType::WHILE, "while"},
        {TokenType::REPEAT, "repeat"}, {TokenType::UNTIL, "until"},
        {TokenType::INTEGER_KW, "integer"}, {TokenType::REAL_KW, "real"},
        {TokenType::BOOLEAN_KW, "boolean"}, {TokenType::CHAR_KW, "char"},
        {TokenType::ARRAY, "array"}, {TokenType::OF, "of"},
        {TokenType::NOT, "not"},
        {TokenType::ASSIGN, "assignop"}, {TokenType::PLUS, "+"},
        {TokenType::MINUS, "-"}, {TokenType::MULTIPLY, "*"},
        {TokenType::DIVIDE, "/"}, {TokenType::DIV_KW, "div"},
        {TokenType::MOD, "mod"}, {TokenType::AND_KW, "and"},
        {TokenType::OR_KW, "or"},
        {TokenType::EQ, "="}, {TokenType::NE, "<>"},
        {TokenType::LT, "<"}, {TokenType::LE, "<="},
        {TokenType::GT, ">"}, {TokenType::GE, ">="},
        {TokenType::LPAREN, "("}, {TokenType::RPAREN, ")"},
        {TokenType::LBRACKET, "["}, {TokenType::RBRACKET, "]"},
        {TokenType::SEMICOLON, ";"}, {TokenType::COLON, ":"},
        {TokenType::COMMA, ","}, {TokenType::DOT, "."},
        {TokenType::DOTDOT, ".."},
        {TokenType::NUM, "num"}, {TokenType::LETTER, "letter"},
        {TokenType::ID, "id"},
        {TokenType::END_OF_FILE, "$"}
    };
    auto it = m.find(t);
    if (it != m.end()) return it->second;
    return "UNKNOWN";
}

// Read tokens from lexer output file
inline std::vector<Token> readTokens(const std::string& filename) {
    std::vector<Token> tokens;
    // implemented in token_reader.cpp
    return tokens;
}

#endif // TOKEN_H
