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
#include <functional>
#include <unordered_map>
#include <memory>

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

struct LocatedStringList {
    std::vector<std::string> values;
    int line = 0;
    int column = 0;
};

struct FactorTail {
    enum class Kind { None, Index, Call };
    Kind kind = Kind::None;
    ExprNodeList expressions;
    std::vector<std::string> fields;
};

struct StatementTail {
    enum class Kind { Assign, IndexedAssign, Call, BareCall };
    Kind kind = Kind::BareCall;
    ExprNodeList expressions;
    std::vector<std::string> fields;
    ExprNodePtr value;
};

struct CaseBranchValue {
    ExprNodeList labels;
    StmtNodePtr statement;
};

using CaseBranchList = std::vector<CaseBranchValue>;

using SemanticValue = std::variant<
    std::monostate,
    Token,
    std::string,
    LocatedStringList,
    std::vector<std::string>,
    DeclNodePtr,
    StmtNodePtr,
    ExprNodePtr,
    BlockNodePtr,
    ProgramNodePtr,
    DeclNodeList,
    StmtNodeList,
    ExprNodeList,
    CaseBranchValue,
    CaseBranchList,
    RelOpTail,
    ExprChain,
    FactorTail,
    StatementTail
>;

// Semantic action signature: (production, rhsValues) -> reduced SemanticValue
using SemanticAction = std::function<SemanticValue(
    const Production& production,
    const std::vector<SemanticValue>& rhsValues)>;

class Parser {
public:
    // Construct from a Lexer (on-demand tokenization; Parser does NOT own the Lexer)
    Parser(Grammar& grammar, Lexer& lexer);

    bool parse();
    void setTraceEnabled(bool enabled) { traceEnabled_ = enabled; }

    const std::vector<ParseError>& getErrors() const { return errors; }
    const std::vector<LexerError>& getLexerErrors() const;
    bool hasLexerErrors() const;
    const std::vector<Token>& getConsumedTokens() const { return consumedTokens; }
    void printParseProcess(std::ostream& os) const;
    ProgramNodePtr getASTRoot() const { return astRoot; }

    // ---------------------------------------------------------------------------
    // Extensible semantic action registry
    // Key: production LHS name (e.g. "statement", "expression")
    // Each registered handler is tried in order; it may return std::monostate
    // to signal "not handled", and the next handler will be tried.
    // ---------------------------------------------------------------------------
    void registerAction(const std::string& lhs, SemanticAction action);

private:
    Grammar& grammar;
    Lexer* lexer;                        // non-owning

    Token currentTok;                    // current lookahead token
    std::vector<Token> consumedTokens;   // history of all tokens consumed
    std::vector<ParseError> errors;
    ProgramNodePtr astRoot;
    std::vector<SemanticValue> semanticStack;

    // Extensible action table: lhs -> list of handlers (tried in order)
    std::unordered_map<std::string, std::vector<SemanticAction>> actionTable;
    void registerDefaultActions();

    // Parse process log
    struct ParseStep {
        std::string stack;
        std::string input;
        std::string action;
    };
    std::vector<ParseStep> steps;
    bool traceEnabled_ = false;

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
