#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "grammar.h"
#include <vector>
#include <string>
#include <stack>
#include <fstream>

struct ParseError {
    int line;
    int column;
    std::string message;
};

class Parser {
public:
    Parser(Grammar& grammar, const std::vector<Token>& tokens);

    bool parse();
    const std::vector<ParseError>& getErrors() const { return errors; }
    void printParseProcess(std::ostream& os) const;

private:
    Grammar& grammar;
    std::vector<Token> tokens;
    size_t pos;
    std::vector<ParseError> errors;

    // Parse process log
    struct ParseStep {
        std::string stack;
        std::string input;
        std::string action;
    };
    std::vector<ParseStep> steps;

    std::string currentTerminal() const;
    Token currentToken() const;
    void advance();
    void addError(const std::string& msg);
    std::string stackToString(const std::stack<std::string>& st) const;
    std::string remainingInput() const;

    // Map relop/addop/mulop token terminals to grammar symbols
    std::string mapTokenToGrammarTerminal(const Token& tok) const;
};

// Read tokens from a file in the format: TOKEN_TYPE lexeme line column
std::vector<Token> readTokensFromFile(const std::string& filename);

#endif
