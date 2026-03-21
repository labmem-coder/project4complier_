#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

struct LexerError {
    int line;
    int column;
    std::string message;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();
    const std::vector<LexerError>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

private:
    std::string source;
    size_t pos;
    int line;
    int column;
    std::vector<Token> result;
    std::vector<LexerError> errors;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    bool skipBraceComment();
    bool skipParenStarComment();

    void addToken(TokenType type, const std::string& lexeme, int startLine, int startCol);
    void addError(int errLine, int errCol, const std::string& msg);

    void scanToken();
    void scanNumber(int startLine, int startCol);
    void scanIdentifierOrKeyword(int startLine, int startCol);
    void scanCharLiteral(int startLine, int startCol);
    void pushBracket(char bracket, int bracketLine, int bracketCol);
    void popBracket(char closingBracket, int closingLine, int closingCol);
    void reportUnclosedBrackets();

    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);

    static const std::unordered_map<std::string, TokenType>& keywordMap();

    std::vector<char> bracketStack;
    std::vector<int> bracketLines;
    std::vector<int> bracketCols;
};

// Convenience: tokenize a Pascal-S source file, returns tokens (with EOF appended)
std::vector<Token> tokenizeFile(const std::string& filename);

// Convenience: tokenize a Pascal-S source string
std::vector<Token> tokenizeSource(const std::string& source);

#endif // LEXER_H
