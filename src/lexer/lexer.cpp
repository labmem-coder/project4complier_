#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

// ---------------------------------------------------------------------------
// Keyword table (all lowercase; Pascal-S is case-insensitive)
// ---------------------------------------------------------------------------
const std::unordered_map<std::string, TokenType>& Lexer::keywordMap() {
    static const std::unordered_map<std::string, TokenType> map = {
        {"program",   TokenType::PROGRAM},
        {"var",       TokenType::VAR},
        {"const",     TokenType::CONST},
        {"procedure", TokenType::PROCEDURE},
        {"function",  TokenType::FUNCTION},
        {"begin",     TokenType::BEGIN},
        {"end",       TokenType::END},
        {"if",        TokenType::IF},
        {"then",      TokenType::THEN},
        {"else",      TokenType::ELSE},
        {"for",       TokenType::FOR},
        {"to",        TokenType::TO},
        {"do",        TokenType::DO},
        {"read",      TokenType::READ},
        {"write",     TokenType::WRITE},
        {"while",     TokenType::WHILE},
        {"repeat",    TokenType::REPEAT},
        {"until",     TokenType::UNTIL},
        {"integer",   TokenType::INTEGER_KW},
        {"real",      TokenType::REAL_KW},
        {"boolean",   TokenType::BOOLEAN_KW},
        {"char",      TokenType::CHAR_KW},
        {"array",     TokenType::ARRAY},
        {"of",        TokenType::OF},
        {"not",       TokenType::NOT},
        {"div",       TokenType::DIV_KW},
        {"mod",       TokenType::MOD},
        {"and",       TokenType::AND_KW},
        {"or",        TokenType::OR_KW},
    };
    return map;
}

// ---------------------------------------------------------------------------
// Helper predicates
// ---------------------------------------------------------------------------
bool Lexer::isDigit(char c)        { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c)        { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool Lexer::isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
Lexer::Lexer(const std::string& source)
    : source(source), pos(0), line(1), column(1) {}

// ---------------------------------------------------------------------------
// Character access
// ---------------------------------------------------------------------------
char Lexer::peek() const {
    if (pos >= source.size()) return '\0';
    return source[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source.size()) return '\0';
    return source[pos + 1];
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= source.size();
}

// ---------------------------------------------------------------------------
// Error / token helpers
// ---------------------------------------------------------------------------
void Lexer::addToken(TokenType type, const std::string& lexeme, int startLine, int startCol) {
    result.push_back({type, lexeme, startLine, startCol});
}

void Lexer::addError(int errLine, int errCol, const std::string& msg) {
    errors.push_back({errLine, errCol, msg});
}

void Lexer::pushBracket(char bracket, int bracketLine, int bracketCol) {
    bracketStack.push_back(bracket);
    bracketLines.push_back(bracketLine);
    bracketCols.push_back(bracketCol);
}

void Lexer::popBracket(char closingBracket, int closingLine, int closingCol) {
    if (bracketStack.empty()) {
        addError(closingLine, closingCol,
                 std::string("Unmatched closing bracket '") + closingBracket + "'");
        return;
    }

    const char open = bracketStack.back();
    const bool matched = (open == '(' && closingBracket == ')') ||
                         (open == '[' && closingBracket == ']');
    if (!matched) {
        addError(closingLine, closingCol,
                 std::string("Bracket mismatch: '") + open + "' does not match '" +
                 closingBracket + "'");
        bracketStack.pop_back();
        bracketLines.pop_back();
        bracketCols.pop_back();
        return;
    }

    bracketStack.pop_back();
    bracketLines.pop_back();
    bracketCols.pop_back();
}

void Lexer::reportUnclosedBrackets() {
    while (!bracketStack.empty()) {
        const char open = bracketStack.back();
        const int openLine = bracketLines.back();
        const int openCol = bracketCols.back();
        addError(openLine, openCol,
                 std::string("Unclosed opening bracket '") + open + "'");
        bracketStack.pop_back();
        bracketLines.pop_back();
        bracketCols.pop_back();
    }
}

// ---------------------------------------------------------------------------
// Whitespace and comment skipping
// ---------------------------------------------------------------------------
bool Lexer::skipBraceComment() {
    // Current char is '{', already consumed by caller decision — but we consume here.
    int startLine = line;
    int startCol = column;
    advance(); // consume '{'
    while (!isAtEnd()) {
        if (peek() == '}') {
            advance(); // consume '}'
            return true;
        }
        advance();
    }
    addError(startLine, startCol, "Unterminated comment starting with '{'");
    return false;
}

bool Lexer::skipParenStarComment() {
    // Current two chars are '(' '*'
    int startLine = line;
    int startCol = column;
    advance(); // consume '('
    advance(); // consume '*'
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == ')') {
            advance(); // consume '*'
            advance(); // consume ')'
            return true;
        }
        advance();
    }
    addError(startLine, startCol, "Unterminated comment starting with '(*'");
    return false;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        // Whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }
        // Brace comment { ... }
        if (c == '{') {
            skipBraceComment();
            continue;
        }
        // Paren-star comment (* ... *)
        if (c == '(' && peekNext() == '*') {
            skipParenStarComment();
            continue;
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Number scanning: integer or real (e.g. 123, 3.14)
// ---------------------------------------------------------------------------
void Lexer::scanNumber(int startLine, int startCol) {
    size_t start = pos;
    while (!isAtEnd() && isDigit(peek())) advance();

    // 123abc -> invalid identifier, since identifiers may not start with a digit.
    if (!isAtEnd() && isAlpha(peek())) {
        while (!isAtEnd() && isAlphaNumeric(peek())) advance();
        addError(startLine, startCol,
                 "Invalid identifier '" + source.substr(start, pos - start) +
                 "': identifiers cannot start with a digit");
        return;
    }

    // Check for real number (decimal point not followed by another dot => not '..')
    bool sawFraction = false;
    if (!isAtEnd() && peek() == '.' && peekNext() != '.') {
        advance(); // consume '.'
        sawFraction = true;
        if (!isAtEnd() && isDigit(peek())) {
            while (!isAtEnd() && isDigit(peek())) advance();
        }
    }

    // 1.2.3 -> invalid number with multiple decimal points.
    if (sawFraction && !isAtEnd() && peek() == '.' && peekNext() != '.') {
        advance(); // consume the extra '.'
        while (!isAtEnd() && (isDigit(peek()) || peek() == '.')) advance();
        addError(startLine, startCol,
                 "Invalid number format '" + source.substr(start, pos - start) +
                 "': multiple decimal points");
        return;
    }

    std::string lexeme = source.substr(start, pos - start);
    addToken(TokenType::NUM, lexeme, startLine, startCol);
}

// ---------------------------------------------------------------------------
// Identifier / keyword scanning
// ---------------------------------------------------------------------------
void Lexer::scanIdentifierOrKeyword(int startLine, int startCol) {
    size_t start = pos;
    while (!isAtEnd() && isAlphaNumeric(peek())) advance();

    std::string lexeme = source.substr(start, pos - start);

    // Lowercase copy for keyword lookup (Pascal is case-insensitive)
    std::string lower = lexeme;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });

    auto& kw = keywordMap();
    auto it = kw.find(lower);
    if (it != kw.end()) {
        // Store keyword in its canonical lowercase form
        addToken(it->second, lower, startLine, startCol);
    } else {
        addToken(TokenType::ID, lexeme, startLine, startCol);
    }
}

// ---------------------------------------------------------------------------
// Character literal: 'x'
// ---------------------------------------------------------------------------
void Lexer::scanCharLiteral(int startLine, int startCol) {
    advance(); // consume opening '\''
    if (isAtEnd()) {
        addError(startLine, startCol, "Unterminated character literal");
        return;
    }
    char ch = advance(); // the character
    if (isAtEnd() || peek() != '\'') {
        addError(startLine, startCol, "Unterminated character literal, expected closing '\\''");
        return;
    }
    advance(); // consume closing '\''
    // Store as 'x' (with quotes) — convenient for C code generation
    std::string lexeme = std::string("'") + ch + "'";
    addToken(TokenType::LETTER, lexeme, startLine, startCol);
}

// ---------------------------------------------------------------------------
// Main scan loop
// ---------------------------------------------------------------------------
void Lexer::scanToken() {
    int startLine = line;
    int startCol = column;
    char c = peek();

    // Numbers
    if (isDigit(c)) {
        scanNumber(startLine, startCol);
        return;
    }

    // Identifiers and keywords
    if (isAlpha(c)) {
        scanIdentifierOrKeyword(startLine, startCol);
        return;
    }

    // Character literal
    if (c == '\'') {
        scanCharLiteral(startLine, startCol);
        return;
    }

    // Operators and delimiters
    advance(); // consume the character

    switch (c) {
        case '+': addToken(TokenType::PLUS,      "+",  startLine, startCol); break;
        case '-': addToken(TokenType::MINUS,     "-",  startLine, startCol); break;
        case '*': addToken(TokenType::MULTIPLY,  "*",  startLine, startCol); break;
        case '/': addToken(TokenType::DIVIDE,    "/",  startLine, startCol); break;
        case '=': addToken(TokenType::EQ,        "=",  startLine, startCol); break;
        case '(':
            pushBracket('(', startLine, startCol);
            addToken(TokenType::LPAREN, "(", startLine, startCol);
            break;
        case ')':
            popBracket(')', startLine, startCol);
            addToken(TokenType::RPAREN, ")", startLine, startCol);
            break;
        case '[':
            pushBracket('[', startLine, startCol);
            addToken(TokenType::LBRACKET, "[", startLine, startCol);
            break;
        case ']':
            popBracket(']', startLine, startCol);
            addToken(TokenType::RBRACKET, "]", startLine, startCol);
            break;
        case ',': addToken(TokenType::COMMA,     ",",  startLine, startCol); break;
        case ';': addToken(TokenType::SEMICOLON, ";",  startLine, startCol); break;

        case ':':
            if (!isAtEnd() && peek() == '=') {
                advance(); // consume '='
                addToken(TokenType::ASSIGN, ":=", startLine, startCol);
            } else {
                addToken(TokenType::COLON, ":", startLine, startCol);
            }
            break;

        case '.':
            if (!isAtEnd() && peek() == '.') {
                advance(); // consume second '.'
                addToken(TokenType::DOTDOT, "..", startLine, startCol);
            } else {
                addToken(TokenType::DOT, ".", startLine, startCol);
            }
            break;

        case '<':
            if (!isAtEnd() && peek() == '=') {
                advance();
                addToken(TokenType::LE, "<=", startLine, startCol);
            } else if (!isAtEnd() && peek() == '>') {
                advance();
                addToken(TokenType::NE, "<>", startLine, startCol);
            } else {
                addToken(TokenType::LT, "<", startLine, startCol);
            }
            break;

        case '>':
            if (!isAtEnd() && peek() == '=') {
                advance();
                addToken(TokenType::GE, ">=", startLine, startCol);
            } else {
                addToken(TokenType::GT, ">", startLine, startCol);
            }
            break;

        default:
            if (static_cast<unsigned char>(c) >= 128) {
                addError(startLine, startCol,
                         "Invalid non-ASCII character encountered");
            } else {
                addError(startLine, startCol,
                         std::string("Unexpected character '") + c + "'");
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Public: tokenize the whole source
// ---------------------------------------------------------------------------
std::vector<Token> Lexer::tokenize() {
    result.clear();
    errors.clear();
    pos = 0;
    line = 1;
    column = 1;
    bracketStack.clear();
    bracketLines.clear();
    bracketCols.clear();

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (!isAtEnd()) {
            scanToken();
        }
    }

    reportUnclosedBrackets();

    // Append EOF token
    addToken(TokenType::END_OF_FILE, "EOF", line, column);
    return result;
}

// ---------------------------------------------------------------------------
// Convenience functions
// ---------------------------------------------------------------------------
std::vector<Token> tokenizeFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open source file: " << filename << "\n";
        return {};
    }
    std::ostringstream oss;
    oss << fin.rdbuf();
    return tokenizeSource(oss.str());
}

std::vector<Token> tokenizeSource(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}
