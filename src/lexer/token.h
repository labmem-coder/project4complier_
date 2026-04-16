#ifndef TOKEN_H
#define TOKEN_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// Token types for Pascal-S lexer output
enum class TokenType {
    // Keywords
    PROGRAM, VAR, CONST, PROCEDURE, FUNCTION,
    BEGIN, END, IF, THEN, ELSE, FOR, TO, DOWNTO, DO,
    READ, WRITE, WHILE, REPEAT, UNTIL,
    INTEGER_KW, REAL_KW, BOOLEAN_KW, CHAR_KW, STRING_KW,
    ARRAY, OF, NOT, CASE, BREAK, CONTINUE, RECORD,
    TRUE_KW, FALSE_KW,

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
    STRING,     // string literal like 'hello'
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
        {"DOWNTO", TokenType::DOWNTO},
        {"DO", TokenType::DO}, {"READ", TokenType::READ},
        {"WRITE", TokenType::WRITE}, {"WHILE", TokenType::WHILE},
        {"REPEAT", TokenType::REPEAT}, {"UNTIL", TokenType::UNTIL},
        {"INTEGER", TokenType::INTEGER_KW}, {"REAL", TokenType::REAL_KW},
        {"BOOLEAN", TokenType::BOOLEAN_KW}, {"CHAR", TokenType::CHAR_KW},
        {"STRING_KW", TokenType::STRING_KW},
        {"ARRAY", TokenType::ARRAY}, {"OF", TokenType::OF},
        {"NOT", TokenType::NOT}, {"CASE", TokenType::CASE},
        {"BREAK", TokenType::BREAK}, {"CONTINUE", TokenType::CONTINUE},
        {"RECORD", TokenType::RECORD},
        {"TRUE", TokenType::TRUE_KW}, {"FALSE", TokenType::FALSE_KW},
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
        {"STRING", TokenType::STRING},
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
        {TokenType::DOWNTO, "downto"},
        {TokenType::DO, "do"}, {TokenType::READ, "read"},
        {TokenType::WRITE, "write"}, {TokenType::WHILE, "while"},
        {TokenType::REPEAT, "repeat"}, {TokenType::UNTIL, "until"},
        {TokenType::INTEGER_KW, "integer"}, {TokenType::REAL_KW, "real"},
        {TokenType::BOOLEAN_KW, "boolean"}, {TokenType::CHAR_KW, "char"},
        {TokenType::STRING_KW, "string_kw"},
        {TokenType::ARRAY, "array"}, {TokenType::OF, "of"},
        {TokenType::NOT, "not"}, {TokenType::CASE, "case"},
        {TokenType::BREAK, "break"}, {TokenType::CONTINUE, "continue"},
        {TokenType::RECORD, "record"},
        {TokenType::TRUE_KW, "true"}, {TokenType::FALSE_KW, "false"},
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
        {TokenType::STRING, "string"},
        {TokenType::ID, "id"},
        {TokenType::END_OF_FILE, "$"}
    };
    auto it = m.find(t);
    if (it != m.end()) return it->second;
    return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& os, TokenType t) {
    return os << static_cast<int>(t);
}

// Map TokenType to its display name string (e.g. PROGRAM, ID, NUM, ...)
inline std::string tokenTypeToString(TokenType t) {
    static const std::unordered_map<TokenType, std::string> m = {
        {TokenType::PROGRAM, "PROGRAM"}, {TokenType::VAR, "VAR"},
        {TokenType::CONST, "CONST"}, {TokenType::PROCEDURE, "PROCEDURE"},
        {TokenType::FUNCTION, "FUNCTION"}, {TokenType::BEGIN, "BEGIN"},
        {TokenType::END, "END"}, {TokenType::IF, "IF"},
        {TokenType::THEN, "THEN"}, {TokenType::ELSE, "ELSE"},
        {TokenType::FOR, "FOR"}, {TokenType::TO, "TO"},
        {TokenType::DOWNTO, "DOWNTO"},
        {TokenType::DO, "DO"}, {TokenType::READ, "READ"},
        {TokenType::WRITE, "WRITE"}, {TokenType::WHILE, "WHILE"},
        {TokenType::REPEAT, "REPEAT"}, {TokenType::UNTIL, "UNTIL"},
        {TokenType::INTEGER_KW, "INTEGER"}, {TokenType::REAL_KW, "REAL"},
        {TokenType::BOOLEAN_KW, "BOOLEAN"}, {TokenType::CHAR_KW, "CHAR"},
        {TokenType::STRING_KW, "STRING_KW"},
        {TokenType::ARRAY, "ARRAY"}, {TokenType::OF, "OF"},
        {TokenType::NOT, "NOT"}, {TokenType::CASE, "CASE"},
        {TokenType::BREAK, "BREAK"}, {TokenType::CONTINUE, "CONTINUE"},
        {TokenType::RECORD, "RECORD"},
        {TokenType::TRUE_KW, "TRUE"}, {TokenType::FALSE_KW, "FALSE"},
        {TokenType::ASSIGN, "ASSIGN"}, {TokenType::PLUS, "PLUS"},
        {TokenType::MINUS, "MINUS"}, {TokenType::MULTIPLY, "MULTIPLY"},
        {TokenType::DIVIDE, "DIVIDE"}, {TokenType::DIV_KW, "DIV"},
        {TokenType::MOD, "MOD"}, {TokenType::AND_KW, "AND"},
        {TokenType::OR_KW, "OR"},
        {TokenType::EQ, "EQ"}, {TokenType::NE, "NE"},
        {TokenType::LT, "LT"}, {TokenType::LE, "LE"},
        {TokenType::GT, "GT"}, {TokenType::GE, "GE"},
        {TokenType::LPAREN, "LPAREN"}, {TokenType::RPAREN, "RPAREN"},
        {TokenType::LBRACKET, "LBRACKET"}, {TokenType::RBRACKET, "RBRACKET"},
        {TokenType::SEMICOLON, "SEMICOLON"}, {TokenType::COLON, "COLON"},
        {TokenType::COMMA, "COMMA"}, {TokenType::DOT, "DOT"},
        {TokenType::DOTDOT, "DOTDOT"},
        {TokenType::NUM, "NUM"}, {TokenType::LETTER, "LETTER"},
        {TokenType::STRING, "STRING"},
        {TokenType::ID, "ID"},
        {TokenType::END_OF_FILE, "EOF"}
    };
    auto it = m.find(t);
    if (it != m.end()) return it->second;
    return "UNKNOWN";
}

#endif // TOKEN_H
