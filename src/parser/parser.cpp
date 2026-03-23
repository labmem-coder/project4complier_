#include "parser.h"
#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {

std::string joinRhs(const std::vector<std::string>& rhs) {
    std::string result;
    for (size_t i = 0; i < rhs.size(); ++i) {
        if (i) result += " ";
        result += rhs[i];
    }
    return result;
}

bool rhsEquals(const Production& production, std::initializer_list<const char*> expected) {
    if (production.rhs.size() != expected.size()) return false;
    size_t index = 0;
    for (const char* symbol : expected) {
        if (production.rhs[index++] != symbol) return false;
    }
    return true;
}

std::string tokenLexeme(const SemanticValue& value) {
    if (const auto* token = std::get_if<Token>(&value)) return token->lexeme;
    return "";
}

std::string stringValue(const SemanticValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) return *text;
    return "";
}

std::vector<std::string> stringListValue(const SemanticValue& value) {
    if (const auto* texts = std::get_if<std::vector<std::string>>(&value)) return *texts;
    return {};
}

DeclNodePtr declValue(const SemanticValue& value) {
    if (const auto* node = std::get_if<DeclNodePtr>(&value)) return *node;
    return nullptr;
}

StmtNodePtr stmtValue(const SemanticValue& value) {
    if (const auto* node = std::get_if<StmtNodePtr>(&value)) return *node;
    return nullptr;
}

ExprNodePtr exprValue(const SemanticValue& value) {
    if (const auto* node = std::get_if<ExprNodePtr>(&value)) return *node;
    return nullptr;
}

BlockNodePtr blockValue(const SemanticValue& value) {
    if (const auto* node = std::get_if<BlockNodePtr>(&value)) return *node;
    return nullptr;
}

ProgramNodePtr programValue(const SemanticValue& value) {
    if (const auto* node = std::get_if<ProgramNodePtr>(&value)) return *node;
    return nullptr;
}

DeclNodeList declListValue(const SemanticValue& value) {
    if (const auto* nodes = std::get_if<DeclNodeList>(&value)) return *nodes;
    return {};
}

StmtNodeList stmtListValue(const SemanticValue& value) {
    if (const auto* nodes = std::get_if<StmtNodeList>(&value)) return *nodes;
    return {};
}

ExprNodeList exprListValue(const SemanticValue& value) {
    if (const auto* nodes = std::get_if<ExprNodeList>(&value)) return *nodes;
    return {};
}

RelOpTail relTailValue(const SemanticValue& value) {
    if (const auto* tail = std::get_if<RelOpTail>(&value)) return *tail;
    return {};
}

ExprChain exprChainValue(const SemanticValue& value) {
    if (const auto* chain = std::get_if<ExprChain>(&value)) return *chain;
    return {};
}

FactorTail factorTailValue(const SemanticValue& value) {
    if (const auto* tail = std::get_if<FactorTail>(&value)) return *tail;
    return {};
}

StatementTail statementTailValue(const SemanticValue& value) {
    if (const auto* tail = std::get_if<StatementTail>(&value)) return *tail;
    return {};
}

void appendDeclList(DeclNodeList& target, const DeclNodeList& source) {
    for (const auto& node : source) {
        if (node) target.push_back(node);
    }
}

void appendStmtList(StmtNodeList& target, const StmtNodeList& source) {
    for (const auto& node : source) {
        if (node) target.push_back(node);
    }
}

void appendExprList(ExprNodeList& target, const ExprNodeList& source) {
    for (const auto& node : source) {
        if (node) target.push_back(node);
    }
}

ExprNodePtr foldLeftAssociative(const ExprNodePtr& left, const ExprChain& chain) {
    ExprNodePtr current = left;
    for (const auto& item : chain.items) {
        auto binary = std::make_shared<BinaryExprNode>();
        binary->op = item.first;
        binary->left = current;
        binary->right = item.second;
        current = binary;
    }
    return current;
}

std::string typeNameFromBasic(const SemanticValue& value) {
    if (const auto* token = std::get_if<Token>(&value)) return token->lexeme;
    return stringValue(value);
}

} // namespace

Parser::Parser(Grammar& grammar, Lexer& lexer)
    : grammar(grammar), lexer(&lexer), astRoot(nullptr) {
    currentTok = this->lexer->nextToken();
    registerDefaultActions();
}

const std::vector<LexerError>& Parser::getLexerErrors() const {
    return lexer->getErrors();
}

bool Parser::hasLexerErrors() const {
    return lexer->hasErrors();
}

std::string Parser::mapTokenToGrammarTerminal(const Token& tok) const {
    switch (tok.type) {
        case TokenType::EQ:
        case TokenType::NE:
        case TokenType::LT:
        case TokenType::LE:
        case TokenType::GT:
        case TokenType::GE:
            return "relop";
        case TokenType::PLUS:
        case TokenType::OR_KW:
            return "addop";
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::DIV_KW:
        case TokenType::MOD:
        case TokenType::AND_KW:
            return "mulop";
        default:
            return tokenTypeToTerminal(tok.type);
    }
}

std::string Parser::currentTerminal() const {
    return mapTokenToGrammarTerminal(currentTok);
}

Token Parser::currentToken() const {
    return currentTok;
}

void Parser::advance() {
    consumedTokens.push_back(currentTok);
    currentTok = lexer->nextToken();
}

void Parser::addError(const std::string& msg) {
    Token tok = currentToken();
    errors.push_back({tok.line, tok.column, msg});
    astRoot.reset();
}

std::string Parser::stackToString(const std::stack<std::string>& st) const {
    std::stack<std::string> tmp = st;
    std::vector<std::string> values;
    while (!tmp.empty()) {
        values.push_back(tmp.top());
        tmp.pop();
    }

    std::string result;
    for (auto it = values.rbegin(); it != values.rend(); ++it) {
        if (it->rfind("#ACT:", 0) == 0) continue;
        if (!result.empty()) result += " ";
        result += *it;
    }
    return result;
}

std::string Parser::remainingInput() const {
    // In on-demand mode we only have the current lookahead
    if (currentTok.type == TokenType::END_OF_FILE) return "$";
    return currentTok.lexeme + " ...";
}

std::string Parser::makeActionSymbol(int productionIndex) const {
    return "#ACT:" + std::to_string(productionIndex);
}

bool Parser::isActionSymbol(const std::string& symbol) const {
    return symbol.rfind("#ACT:", 0) == 0;
}

int Parser::actionProductionIndex(const std::string& symbol) const {
    return std::stoi(symbol.substr(5));
}

bool Parser::parse() {
    errors.clear();
    steps.clear();
    semanticStack.clear();
    consumedTokens.clear();
    astRoot.reset();

    std::stack<std::string> stk;
    stk.push("$");
    stk.push(grammar.startSymbol);

    int maxSteps = 100000;
    int stepCount = 0;

    while (!stk.empty() && stepCount < maxSteps) {
        ++stepCount;
        const std::string top = stk.top();
        const std::string input = currentTerminal();
        const Token tok = currentToken();

        ParseStep step;
        step.stack = stackToString(stk);
        step.input = remainingInput();

        if (isActionSymbol(top)) {
            const int productionIndex = actionProductionIndex(top);
            executeSemanticAction(productionIndex);
            step.action = "REDUCE " + grammar.productions[productionIndex].lhs + " -> " +
                          (grammar.productions[productionIndex].rhs.empty()
                               ? "epsilon"
                               : joinRhs(grammar.productions[productionIndex].rhs));
            steps.push_back(step);
            stk.pop();
            continue;
        }

        if (top == "$") {
            if (input == "$") {
                step.action = "ACCEPT";
                steps.push_back(step);
                if (!semanticStack.empty()) {
                    astRoot = programValue(semanticStack.back());
                }
                return errors.empty();
            }

            step.action = "ERROR: unexpected tokens after program end";
            steps.push_back(step);
            addError("Unexpected tokens after program end near '" + tok.lexeme + "'");
            return false;
        }

        if (!grammar.nonTerminals.count(top)) {
            bool match = false;
            if (top == input) {
                match = true;
            } else if (top == "-" && tok.type == TokenType::MINUS) {
                match = true;
            } else if (top == "+" && tok.type == TokenType::PLUS) {
                match = true;
            } else if (top == "=" && tok.type == TokenType::EQ) {
                match = true;
            } else if (top == "addop" &&
                       (tok.type == TokenType::PLUS || tok.type == TokenType::MINUS ||
                        tok.type == TokenType::OR_KW)) {
                match = true;
            } else if (top == "relop" &&
                       (tok.type == TokenType::EQ || tok.type == TokenType::NE ||
                        tok.type == TokenType::LT || tok.type == TokenType::LE ||
                        tok.type == TokenType::GT || tok.type == TokenType::GE)) {
                match = true;
            } else if (top == "mulop" &&
                       (tok.type == TokenType::MULTIPLY || tok.type == TokenType::DIVIDE ||
                        tok.type == TokenType::DIV_KW || tok.type == TokenType::MOD ||
                        tok.type == TokenType::AND_KW)) {
                match = true;
            } else if (top == "assignop" && tok.type == TokenType::ASSIGN) {
                match = true;
            }

            if (match) {
                step.action = "MATCH '" + top + "'";
                steps.push_back(step);
                stk.pop();
                semanticStack.push_back(tok);
                advance();
            } else {
                step.action = "ERROR: expected '" + top + "', got '" + tok.lexeme + "'";
                steps.push_back(step);
                addError("Expected '" + top + "', but got '" + tok.lexeme + "' (line " +
                         std::to_string(tok.line) + ", col " + std::to_string(tok.column) + ")");
                stk.pop();
            }
            continue;
        }

        auto rowIt = grammar.parseTable.find(top);
        if (rowIt == grammar.parseTable.end()) {
            step.action = "ERROR: no parse table row for '" + top + "'";
            steps.push_back(step);
            addError("Internal error: no parse table row for non-terminal '" + top + "'");
            stk.pop();
            continue;
        }

        std::string lookupTerminal = input;
        if (tok.type == TokenType::PLUS) {
            if (rowIt->second.find("+") != rowIt->second.end()) {
                lookupTerminal = "+";
            } else if (rowIt->second.find("addop") != rowIt->second.end()) {
                lookupTerminal = "addop";
            }
        } else if (tok.type == TokenType::MINUS) {
            if (rowIt->second.find("-") != rowIt->second.end()) {
                lookupTerminal = "-";
            } else if (rowIt->second.find("addop") != rowIt->second.end()) {
                lookupTerminal = "addop";
            }
        } else if (tok.type == TokenType::EQ && rowIt->second.find("=") != rowIt->second.end()) {
            lookupTerminal = "=";
        }

        auto cellIt = rowIt->second.find(lookupTerminal);
        if (cellIt == rowIt->second.end()) {
            step.action = "ERROR: no entry for [" + top + ", " + lookupTerminal + "]";
            steps.push_back(step);

            std::string expected;
            for (auto eit = rowIt->second.begin(); eit != rowIt->second.end(); ++eit) {
                if (!expected.empty()) expected += ", ";
                expected += "'" + eit->first + "'";
            }

            addError("Syntax error at '" + tok.lexeme + "' (line " + std::to_string(tok.line) +
                     ", col " + std::to_string(tok.column) + "): expected one of {" + expected +
                     "} for non-terminal '" + top + "'");

            bool recovered = false;
            auto& followSet = grammar.followSets[top];
            auto& firstSet = grammar.firstSets[top];

            if (followSet.count(lookupTerminal)) {
                stk.pop();
                recovered = true;
            } else {
                while (currentTok.type != TokenType::END_OF_FILE) {
                    std::string nextT = currentTerminal();
                    if (top == "factor" && currentTok.type == TokenType::MINUS) nextT = "-";
                    if (firstSet.count(nextT) || followSet.count(nextT)) {
                        recovered = true;
                        break;
                    }
                    advance();
                }
                if (!recovered) {
                    stk.pop();
                } else if (followSet.count(currentTerminal())) {
                    stk.pop();
                }
            }
            continue;
        }

        const int prodIdx = cellIt->second;
        const auto& prod = grammar.productions[prodIdx];

        step.action = "PREDICT " + prod.lhs + " -> ";
        step.action += prod.rhs.empty() ? "epsilon" : joinRhs(prod.rhs);
        steps.push_back(step);

        stk.pop();
        stk.push(makeActionSymbol(prodIdx));
        for (int k = static_cast<int>(prod.rhs.size()) - 1; k >= 0; --k) {
            stk.push(prod.rhs[k]);
        }
    }

    if (stepCount >= maxSteps) {
        addError("Parse exceeded maximum steps (possible infinite loop)");
    }

    return errors.empty();
}

void Parser::executeSemanticAction(int productionIndex) {
    const Production& production = grammar.productions[productionIndex];
    const size_t popCount = production.rhs.size();

    std::vector<SemanticValue> rhsValues(popCount);
    for (size_t i = 0; i < popCount; ++i) {
        if (semanticStack.empty()) {
            rhsValues[popCount - i - 1] = std::monostate{};
            continue;
        }
        rhsValues[popCount - i - 1] = semanticStack.back();
        semanticStack.pop_back();
    }

    SemanticValue reduced = buildSemanticValue(production, rhsValues);
    semanticStack.push_back(reduced);

    if (production.lhs == "programstruct") {
        astRoot = programValue(reduced);
    }
}

// ---------------------------------------------------------------------------
// Extensible action registry
// ---------------------------------------------------------------------------
void Parser::registerAction(const std::string& lhs, SemanticAction action) {
    // New actions are prepended so that user-registered actions take priority
    actionTable[lhs].insert(actionTable[lhs].begin(), std::move(action));
}

SemanticValue Parser::buildSemanticValue(const Production& production,
                                         const std::vector<SemanticValue>& rhsValues) {
    auto it = actionTable.find(production.lhs);
    if (it != actionTable.end()) {
        for (auto& handler : it->second) {
            SemanticValue result = handler(production, rhsValues);
            if (!std::holds_alternative<std::monostate>(result)) {
                return result;
            }
        }
    }
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// Register all default semantic actions for the Pascal-S grammar.
// Each non-terminal's rules are grouped into one handler function.
// To extend: call parser.registerAction("new_nt", handler) before parse().
// ---------------------------------------------------------------------------
void Parser::registerDefaultActions() {

    // --- programstruct ---
    registerAction("programstruct", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"program_head", ";", "program_body", "."})) {
            auto prog = programValue(v[0]);
            if (!prog) prog = std::make_shared<ProgramNode>();
            prog->block = blockValue(v[2]);
            return prog;
        }
        return std::monostate{};
    });

    // --- program_head ---
    registerAction("program_head", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"program", "id", "(", "idlist", ")"})) {
            auto prog = std::make_shared<ProgramNode>();
            prog->name = tokenLexeme(v[1]);
            prog->parameters = stringListValue(v[3]);
            return prog;
        }
        if (rhsEquals(p, {"program", "id", "program_head'"})) {
            auto prog = std::make_shared<ProgramNode>();
            prog->name = tokenLexeme(v[1]);
            prog->parameters = stringListValue(v[2]);
            return prog;
        }
        if (rhsEquals(p, {"program", "id"})) {
            auto prog = std::make_shared<ProgramNode>();
            prog->name = tokenLexeme(v[1]);
            return prog;
        }
        return std::monostate{};
    });

    // --- program_head' ---
    registerAction("program_head'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"(", "idlist", ")"})) return stringListValue(v[1]);
        if (p.rhs.empty()) return std::vector<std::string>{};
        return std::monostate{};
    });

    // --- program_body ---
    registerAction("program_body", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"const_declarations", "var_declarations",
                          "subprogram_declarations", "compound_statement"})) {
            auto block = std::make_shared<BlockNode>();
            block->constDecls = declListValue(v[0]);
            block->varDecls = declListValue(v[1]);
            block->subprogramDecls = declListValue(v[2]);
            block->compoundStmt = stmtValue(v[3]);
            return block;
        }
        return std::monostate{};
    });

    // --- idlist / idlist' ---
    registerAction("idlist", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"id", "idlist'"})) {
            std::vector<std::string> names{tokenLexeme(v[0])};
            auto tail = stringListValue(v[1]);
            names.insert(names.end(), tail.begin(), tail.end());
            return names;
        }
        return std::monostate{};
    });

    registerAction("idlist'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {",", "id", "idlist'"})) {
            std::vector<std::string> names{tokenLexeme(v[1])};
            auto tail = stringListValue(v[2]);
            names.insert(names.end(), tail.begin(), tail.end());
            return names;
        }
        if (p.rhs.empty()) return std::vector<std::string>{};
        return std::monostate{};
    });

    // --- const_declarations ---
    registerAction("const_declarations", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) return DeclNodeList{};
        if (rhsEquals(p, {"const", "const_decl_list"})) return declListValue(v[1]);
        return std::monostate{};
    });

    // --- const_decl_list ---
    registerAction("const_decl_list", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"id", "=", "const_value", ";", "const_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<ConstDeclNode>();
            node->name = tokenLexeme(v[0]);
            node->value = exprValue(v[2]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[4]));
            return decls;
        }
        return std::monostate{};
    });

    // --- const_decl_list_tail ---
    registerAction("const_decl_list_tail", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"id", "=", "const_value", ";", "const_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<ConstDeclNode>();
            node->name = tokenLexeme(v[0]);
            node->value = exprValue(v[2]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[4]));
            return decls;
        }
        if (p.rhs.empty()) return DeclNodeList{};
        return std::monostate{};
    });

    // --- const_value ---
    registerAction("const_value", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"+", "num"})) {
            auto lit = std::make_shared<LiteralExprNode>();
            lit->literalType = "num";
            lit->value = "+" + tokenLexeme(v[1]);
            return std::static_pointer_cast<ExprNode>(lit);
        }
        if (rhsEquals(p, {"-", "num"})) {
            auto lit = std::make_shared<LiteralExprNode>();
            lit->literalType = "num";
            lit->value = "-" + tokenLexeme(v[1]);
            return std::static_pointer_cast<ExprNode>(lit);
        }
        if (rhsEquals(p, {"num"})) {
            auto lit = std::make_shared<LiteralExprNode>();
            lit->literalType = "num";
            lit->value = tokenLexeme(v[0]);
            return std::static_pointer_cast<ExprNode>(lit);
        }
        if (rhsEquals(p, {"letter"})) {
            auto lit = std::make_shared<LiteralExprNode>();
            lit->literalType = "char";
            lit->value = tokenLexeme(v[0]);
            return std::static_pointer_cast<ExprNode>(lit);
        }
        return std::monostate{};
    });

    // --- var_declarations ---
    registerAction("var_declarations", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) return DeclNodeList{};
        if (rhsEquals(p, {"var", "var_decl_list"})) return declListValue(v[1]);
        return std::monostate{};
    });

    // --- var_decl_list ---
    registerAction("var_decl_list", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"idlist", ":", "type", ";", "var_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<VarDeclNode>();
            node->names = stringListValue(v[0]);
            node->typeName = stringValue(v[2]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[4]));
            return decls;
        }
        if (rhsEquals(p, {"id", "idlist'", ":", "type", ";", "var_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<VarDeclNode>();
            node->names.push_back(tokenLexeme(v[0]));
            auto tailNames = stringListValue(v[1]);
            node->names.insert(node->names.end(), tailNames.begin(), tailNames.end());
            node->typeName = stringValue(v[3]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[5]));
            return decls;
        }
        return std::monostate{};
    });

    // --- var_decl_list_tail ---
    registerAction("var_decl_list_tail", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"idlist", ":", "type", ";", "var_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<VarDeclNode>();
            node->names = stringListValue(v[0]);
            node->typeName = stringValue(v[2]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[4]));
            return decls;
        }
        if (rhsEquals(p, {"id", "idlist'", ":", "type", ";", "var_decl_list_tail"})) {
            DeclNodeList decls;
            auto node = std::make_shared<VarDeclNode>();
            node->names.push_back(tokenLexeme(v[0]));
            auto tailNames = stringListValue(v[1]);
            node->names.insert(node->names.end(), tailNames.begin(), tailNames.end());
            node->typeName = stringValue(v[3]);
            decls.push_back(node);
            appendDeclList(decls, declListValue(v[5]));
            return decls;
        }
        if (p.rhs.empty()) return DeclNodeList{};
        return std::monostate{};
    });

    // --- type ---
    registerAction("type", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"basic_type"})) return typeNameFromBasic(v[0]);
        if (rhsEquals(p, {"array", "[", "period", "]", "of", "basic_type"})) {
            return "array[" + stringValue(v[2]) + "] of " + typeNameFromBasic(v[5]);
        }
        return std::monostate{};
    });

    // --- basic_type ---
    registerAction("basic_type", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.size() == 1) return tokenLexeme(v[0]);
        return std::monostate{};
    });

    // --- period / period' ---
    registerAction("period", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"num", "..", "num", "period'"})) {
            std::string range = tokenLexeme(v[0]) + ".." + tokenLexeme(v[2]);
            auto tail = stringValue(v[3]);
            if (!tail.empty()) range += tail;
            return range;
        }
        return std::monostate{};
    });

    registerAction("period'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {",", "num", "..", "num", "period'"})) {
            std::string range = "," + tokenLexeme(v[1]) + ".." + tokenLexeme(v[3]);
            auto tail = stringValue(v[4]);
            if (!tail.empty()) range += tail;
            return range;
        }
        if (p.rhs.empty()) return std::string{};
        return std::monostate{};
    });

    // --- subprogram_declarations ---
    registerAction("subprogram_declarations", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) return DeclNodeList{};
        if (rhsEquals(p, {"subprogram", ";", "subprogram_declarations"})) {
            DeclNodeList decls;
            auto sub = declValue(v[0]);
            if (sub) decls.push_back(sub);
            appendDeclList(decls, declListValue(v[2]));
            return decls;
        }
        return std::monostate{};
    });

    // --- subprogram (placeholder — not yet fully implemented) ---
    registerAction("subprogram", [](const Production&, const std::vector<SemanticValue>&) -> SemanticValue {
        return DeclNodePtr{};
    });

    // --- subprogram_head, formal_parameter, parameter_list, etc. ---
    auto passthrough = [](const Production&, const std::vector<SemanticValue>&) -> SemanticValue {
        return std::monostate{};
    };
    registerAction("subprogram_head", passthrough);
    registerAction("formal_parameter", passthrough);
    registerAction("parameter_list", passthrough);
    registerAction("parameter_list'", passthrough);
    registerAction("parameter", passthrough);
    registerAction("var_parameter", passthrough);
    registerAction("value_parameter", passthrough);

    // --- subprogram_body ---
    registerAction("subprogram_body", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"const_declarations", "var_declarations", "compound_statement"})) {
            auto block = std::make_shared<BlockNode>();
            block->constDecls = declListValue(v[0]);
            block->varDecls = declListValue(v[1]);
            block->compoundStmt = stmtValue(v[2]);
            return block;
        }
        return std::monostate{};
    });

    // --- compound_statement ---
    registerAction("compound_statement", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"begin", "statement_list", "end"})) {
            auto compound = std::make_shared<CompoundStmtNode>();
            compound->statements = stmtListValue(v[1]);
            return std::static_pointer_cast<StmtNode>(compound);
        }
        return std::monostate{};
    });

    // --- statement_list ---
    registerAction("statement_list", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"statement", "statement_list'"})) {
            StmtNodeList list;
            auto first = stmtValue(v[0]);
            if (first) list.push_back(first);
            appendStmtList(list, stmtListValue(v[1]));
            return list;
        }
        return std::monostate{};
    });

    // --- statement_list' ---
    registerAction("statement_list'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {";", "statement", "statement_list'"})) {
            StmtNodeList list;
            auto first = stmtValue(v[1]);
            if (first) list.push_back(first);
            appendStmtList(list, stmtListValue(v[2]));
            return list;
        }
        if (p.rhs.empty()) return StmtNodeList{};
        return std::monostate{};
    });

    // --- statement ---
    registerAction("statement", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) {
            return std::static_pointer_cast<StmtNode>(std::make_shared<EmptyStmtNode>());
        }
        if (rhsEquals(p, {"id", "statement_id_tail"})) {
            const std::string name = tokenLexeme(v[0]);
            const StatementTail tail = statementTailValue(v[1]);
            if (tail.kind == StatementTail::Kind::Call || tail.kind == StatementTail::Kind::BareCall) {
                auto call = std::make_shared<CallStmtNode>();
                call->callee = name;
                call->arguments = tail.expressions;
                return std::static_pointer_cast<StmtNode>(call);
            }
            auto assign = std::make_shared<AssignStmtNode>();
            auto target = std::make_shared<VariableExprNode>();
            target->name = name;
            if (tail.kind == StatementTail::Kind::IndexedAssign) {
                target->indices = tail.expressions;
            }
            assign->target = std::static_pointer_cast<ExprNode>(target);
            assign->value = tail.value;
            return std::static_pointer_cast<StmtNode>(assign);
        }
        if (rhsEquals(p, {"begin", "statement_list", "end"})) {
            auto compound = std::make_shared<CompoundStmtNode>();
            compound->statements = stmtListValue(v[1]);
            return std::static_pointer_cast<StmtNode>(compound);
        }
        if (rhsEquals(p, {"if", "expression", "then", "statement", "else_part"})) {
            auto node = std::make_shared<IfStmtNode>();
            node->condition = exprValue(v[1]);
            node->thenStmt = stmtValue(v[3]);
            node->elseStmt = stmtValue(v[4]);
            return std::static_pointer_cast<StmtNode>(node);
        }
        if (rhsEquals(p, {"for", "id", "assignop", "expression", "to", "expression", "do", "statement"})) {
            auto node = std::make_shared<ForStmtNode>();
            node->iterator = tokenLexeme(v[1]);
            node->startExpr = exprValue(v[3]);
            node->endExpr = exprValue(v[5]);
            node->body = stmtValue(v[7]);
            return std::static_pointer_cast<StmtNode>(node);
        }
        if (rhsEquals(p, {"read", "(", "variable_list", ")"})) {
            auto node = std::make_shared<ReadStmtNode>();
            node->variables = exprListValue(v[2]);
            return std::static_pointer_cast<StmtNode>(node);
        }
        if (rhsEquals(p, {"write", "(", "expression_list", ")"})) {
            auto node = std::make_shared<WriteStmtNode>();
            node->expressions = exprListValue(v[2]);
            return std::static_pointer_cast<StmtNode>(node);
        }
        return std::monostate{};
    });

    // --- statement_id_tail ---
    registerAction("statement_id_tail", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"assignop", "expression"})) {
            StatementTail tail;
            tail.kind = StatementTail::Kind::Assign;
            tail.value = exprValue(v[1]);
            return tail;
        }
        if (rhsEquals(p, {"[", "expression_list", "]", "assignop", "expression"})) {
            StatementTail tail;
            tail.kind = StatementTail::Kind::IndexedAssign;
            tail.expressions = exprListValue(v[1]);
            tail.value = exprValue(v[4]);
            return tail;
        }
        if (rhsEquals(p, {"(", "expression_list", ")"})) {
            StatementTail tail;
            tail.kind = StatementTail::Kind::Call;
            tail.expressions = exprListValue(v[1]);
            return tail;
        }
        if (p.rhs.empty()) return StatementTail{};
        return std::monostate{};
    });

    // --- variable_list ---
    registerAction("variable_list", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"variable", "variable_list'"})) {
            ExprNodeList list;
            auto first = exprValue(v[0]);
            if (first) list.push_back(first);
            appendExprList(list, exprListValue(v[1]));
            return list;
        }
        return std::monostate{};
    });

    registerAction("variable_list'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {",", "variable", "variable_list'"})) {
            ExprNodeList list;
            auto first = exprValue(v[1]);
            if (first) list.push_back(first);
            appendExprList(list, exprListValue(v[2]));
            return list;
        }
        if (p.rhs.empty()) return ExprNodeList{};
        return std::monostate{};
    });

    // --- variable ---
    registerAction("variable", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"id", "id_varpart"})) {
            auto variable = std::make_shared<VariableExprNode>();
            variable->name = tokenLexeme(v[0]);
            variable->indices = exprListValue(v[1]);
            return std::static_pointer_cast<ExprNode>(variable);
        }
        return std::monostate{};
    });

    // --- id_varpart ---
    registerAction("id_varpart", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) return ExprNodeList{};
        if (rhsEquals(p, {"[", "expression_list", "]"})) return exprListValue(v[1]);
        return std::monostate{};
    });

    // --- else_part ---
    registerAction("else_part", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (p.rhs.empty()) return StmtNodePtr{};
        if (rhsEquals(p, {"else", "statement"})) return stmtValue(v[1]);
        return std::monostate{};
    });

    // --- expression_list ---
    registerAction("expression_list", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"expression", "expression_list'"})) {
            ExprNodeList list;
            auto first = exprValue(v[0]);
            if (first) list.push_back(first);
            appendExprList(list, exprListValue(v[1]));
            return list;
        }
        return std::monostate{};
    });

    registerAction("expression_list'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {",", "expression", "expression_list'"})) {
            ExprNodeList list;
            auto first = exprValue(v[1]);
            if (first) list.push_back(first);
            appendExprList(list, exprListValue(v[2]));
            return list;
        }
        if (p.rhs.empty()) return ExprNodeList{};
        return std::monostate{};
    });

    // --- expression ---
    registerAction("expression", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"simple_expression", "expression_tail"})) {
            auto left = exprValue(v[0]);
            auto tail = relTailValue(v[1]);
            if (tail.op.empty()) return left;
            auto binary = std::make_shared<BinaryExprNode>();
            binary->op = tail.op;
            binary->left = left;
            binary->right = tail.rhs;
            return std::static_pointer_cast<ExprNode>(binary);
        }
        return std::monostate{};
    });

    // --- expression_tail ---
    registerAction("expression_tail", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"relop", "simple_expression"})) {
            RelOpTail tail;
            tail.op = tokenLexeme(v[0]);
            tail.rhs = exprValue(v[1]);
            return tail;
        }
        if (p.rhs.empty()) return RelOpTail{};
        return std::monostate{};
    });

    // --- simple_expression ---
    registerAction("simple_expression", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"term", "simple_expression'"})) {
            return foldLeftAssociative(exprValue(v[0]), exprChainValue(v[1]));
        }
        return std::monostate{};
    });

    registerAction("simple_expression'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"addop", "term", "simple_expression'"})) {
            ExprChain chain;
            chain.items.push_back({tokenLexeme(v[0]), exprValue(v[1])});
            auto tail = exprChainValue(v[2]);
            chain.items.insert(chain.items.end(), tail.items.begin(), tail.items.end());
            return chain;
        }
        if (p.rhs.empty()) return ExprChain{};
        return std::monostate{};
    });

    // --- term ---
    registerAction("term", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"factor", "term'"})) {
            return foldLeftAssociative(exprValue(v[0]), exprChainValue(v[1]));
        }
        return std::monostate{};
    });

    registerAction("term'", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"mulop", "factor", "term'"})) {
            ExprChain chain;
            chain.items.push_back({tokenLexeme(v[0]), exprValue(v[1])});
            auto tail = exprChainValue(v[2]);
            chain.items.insert(chain.items.end(), tail.items.begin(), tail.items.end());
            return chain;
        }
        if (p.rhs.empty()) return ExprChain{};
        return std::monostate{};
    });

    // --- factor ---
    registerAction("factor", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"num"})) {
            auto lit = std::make_shared<LiteralExprNode>();
            lit->literalType = "num";
            lit->value = tokenLexeme(v[0]);
            return std::static_pointer_cast<ExprNode>(lit);
        }
        if (rhsEquals(p, {"id", "factor_id_tail"})) {
            const std::string name = tokenLexeme(v[0]);
            const FactorTail tail = factorTailValue(v[1]);
            if (tail.kind == FactorTail::Kind::Call) {
                auto call = std::make_shared<CallExprNode>();
                call->callee = name;
                call->arguments = tail.expressions;
                return std::static_pointer_cast<ExprNode>(call);
            }
            auto variable = std::make_shared<VariableExprNode>();
            variable->name = name;
            if (tail.kind == FactorTail::Kind::Index) {
                variable->indices = tail.expressions;
            }
            return std::static_pointer_cast<ExprNode>(variable);
        }
        if (rhsEquals(p, {"(", "expression", ")"})) return exprValue(v[1]);
        if (rhsEquals(p, {"not", "factor"})) {
            auto node = std::make_shared<UnaryExprNode>();
            node->op = "not";
            node->operand = exprValue(v[1]);
            return std::static_pointer_cast<ExprNode>(node);
        }
        if (rhsEquals(p, {"-", "factor"})) {
            auto node = std::make_shared<UnaryExprNode>();
            node->op = "-";
            node->operand = exprValue(v[1]);
            return std::static_pointer_cast<ExprNode>(node);
        }
        return std::monostate{};
    });

    // --- factor_id_tail ---
    registerAction("factor_id_tail", [](const Production& p, const std::vector<SemanticValue>& v) -> SemanticValue {
        if (rhsEquals(p, {"[", "expression_list", "]"})) {
            FactorTail tail;
            tail.kind = FactorTail::Kind::Index;
            tail.expressions = exprListValue(v[1]);
            return tail;
        }
        if (rhsEquals(p, {"(", "expression_list", ")"})) {
            FactorTail tail;
            tail.kind = FactorTail::Kind::Call;
            tail.expressions = exprListValue(v[1]);
            return tail;
        }
        if (p.rhs.empty()) return FactorTail{};
        return std::monostate{};
    });
}

void Parser::printParseProcess(std::ostream& os) const {
    os << "Parse Process Log\n";
    os << "=================\n\n";
    os << std::string(60, '-') << "\n";

    char buf[256];
    std::snprintf(buf, sizeof(buf), "%-6s %-40s %-30s %s\n", "Step", "Stack", "Input", "Action");
    os << buf;
    os << std::string(60, '-') << "\n";

    for (size_t i = 0; i < steps.size(); ++i) {
        std::string stackStr = steps[i].stack;
        if (stackStr.size() > 38) stackStr = stackStr.substr(0, 35) + "...";

        std::string inputStr = steps[i].input;
        if (inputStr.size() > 28) inputStr = inputStr.substr(0, 25) + "...";

        std::snprintf(buf, sizeof(buf), "%-6d %-40s %-30s %s\n",
                      static_cast<int>(i + 1),
                      stackStr.c_str(),
                      inputStr.c_str(),
                      steps[i].action.c_str());
        os << buf;
    }

    os << std::string(60, '-') << "\n";
}
