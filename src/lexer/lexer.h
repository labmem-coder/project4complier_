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

    // On-demand tokenization: returns the next token each time it is called.
    // Returns END_OF_FILE token when source is exhausted.
    Token nextToken();

    const std::vector<LexerError>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

private:
    std::string source;
    size_t pos;
    int line;
    int column;
    std::vector<LexerError> errors;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    bool skipBraceComment();
    bool skipParenStarComment();

    void addError(int errLine, int errCol, const std::string& msg);

    // Each scan method returns a Token directly.
    // Returns END_OF_FILE with empty lexeme as sentinel when only an error was produced.
    Token scanToken();
    Token scanNumber(int startLine, int startCol);
    Token scanIdentifierOrKeyword(int startLine, int startCol);
    Token scanCharLiteral(int startLine, int startCol);

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

    bool eofEmitted = false;
};

// Convenience: tokenize a Pascal-S source file, returns tokens (with EOF appended)
std::vector<Token> tokenizeFile(const std::string& filename);

// Convenience: tokenize a Pascal-S source string
std::vector<Token> tokenizeSource(const std::string& source);

#endif // LEXER_H
