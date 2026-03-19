#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "lexer.h"
#include "grammar.h"
#include "ast.h"
#include <vector>
#include <string>
#include <stack>
#include <variant>

struct ParseError {
    int line;
    int column;
    std::string message;
};

struct RelOpTail {
    std::string op;
    ExprNodePtr rhs;
};

struct ExprChain {
    std::vector<std::pair<std::string, ExprNodePtr>> items;
};

struct FactorTail {
    enum class Kind { None, Index, Call };
    Kind kind = Kind::None;
    ExprNodeList expressions;
};

struct StatementTail {
    enum class Kind { Assign, IndexedAssign, Call, BareCall };
    Kind kind = Kind::BareCall;
    ExprNodeList expressions;
    ExprNodePtr value;
};

using SemanticValue = std::variant<
    std::monostate,
    Token,
    std::string,
    std::vector<std::string>,
    DeclNodePtr,
    StmtNodePtr,
    ExprNodePtr,
    BlockNodePtr,
    ProgramNodePtr,
    DeclNodeList,
    StmtNodeList,
    ExprNodeList,
    RelOpTail,
    ExprChain,
    FactorTail,
    StatementTail
>;

class Parser {
public:
    // Construct from Pascal-S source code (lexes internally)
    Parser(Grammar& grammar, const std::string& source);

    // Construct from pre-lexed token stream
    Parser(Grammar& grammar, const std::vector<Token>& tokens);

    bool parse();

    const std::vector<ParseError>& getErrors() const { return errors; }
    const std::vector<LexerError>& getLexerErrors() const { return lexerErrors; }
    bool hasLexerErrors() const { return !lexerErrors.empty(); }
    const std::vector<Token>& getTokens() const { return tokens; }
    void printParseProcess(std::ostream& os) const;
    ProgramNodePtr getASTRoot() const { return astRoot; }

private:
    Grammar& grammar;
    std::vector<Token> tokens;
    std::vector<LexerError> lexerErrors;
    size_t pos;
    std::vector<ParseError> errors;
    ProgramNodePtr astRoot;
    std::vector<SemanticValue> semanticStack;

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

    std::string makeActionSymbol(int productionIndex) const;
    bool isActionSymbol(const std::string& symbol) const;
    int actionProductionIndex(const std::string& symbol) const;
    void executeSemanticAction(int productionIndex);
    SemanticValue buildSemanticValue(const Production& production,
                                     const std::vector<SemanticValue>& rhsValues);
};

#endif
