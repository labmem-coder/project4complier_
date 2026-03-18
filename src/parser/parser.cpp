#include "parser.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

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

std::vector<Token> readTokensFromFile(const std::string& filename) {
    std::vector<Token> tokens;
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open token file: " << filename << "\n";
        return tokens;
    }

    auto nameMap = buildTokenNameMap();
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string typeName;
        std::string lexeme;
        int ln = 0;
        int col = 0;
        if (!(iss >> typeName >> lexeme >> ln >> col)) continue;

        auto it = nameMap.find(typeName);
        if (it == nameMap.end()) {
            std::cerr << "Warning: unknown token type '" << typeName << "', skipping.\n";
            continue;
        }

        tokens.push_back({it->second, lexeme, ln, col});
    }

    return tokens;
}

Parser::Parser(Grammar& grammar, const std::vector<Token>& tokens)
    : grammar(grammar), tokens(tokens), pos(0), astRoot(nullptr) {}

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
        case TokenType::MINUS:
<<<<<<< HEAD
            // Context-sensitive: in factor context '-' is uminus, elsewhere addop
            // The parse table will determine via stack top which applies.
            return "addop";
=======
>>>>>>> dbfc27feaef9e504a69605d7b00e00f710e62744
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
    if (pos >= tokens.size()) return "$";
    return mapTokenToGrammarTerminal(tokens[pos]);
}

Token Parser::currentToken() const {
    if (pos >= tokens.size()) return {TokenType::END_OF_FILE, "EOF", 0, 0};
    return tokens[pos];
}

void Parser::advance() {
    if (pos < tokens.size()) pos++;
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
    std::string result;
    for (size_t i = pos; i < tokens.size() && i < pos + 10; ++i) {
        if (!result.empty()) result += " ";
        result += tokens[i].lexeme;
    }
    if (pos + 10 < tokens.size()) result += " ...";
    if (pos >= tokens.size()) result = "$";
    return result;
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
    astRoot.reset();
    pos = 0;

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

        std::string lookupTerminal = input;
        if (top == "factor" && tok.type == TokenType::MINUS) lookupTerminal = "-";
        if (top == "const_value" && tok.type == TokenType::PLUS) lookupTerminal = "+";
        if (top == "const_value" && tok.type == TokenType::MINUS) lookupTerminal = "-";
        if (tok.type == TokenType::EQ) {
            auto rowCheck = grammar.parseTable.find(top);
            if (rowCheck != grammar.parseTable.end() &&
                rowCheck->second.find("=") != rowCheck->second.end()) {
                lookupTerminal = "=";
            }
        }

        auto rowIt = grammar.parseTable.find(top);
        if (rowIt == grammar.parseTable.end()) {
            step.action = "ERROR: no parse table row for '" + top + "'";
            steps.push_back(step);
            addError("Internal error: no parse table row for non-terminal '" + top + "'");
            stk.pop();
            continue;
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
                while (pos < tokens.size()) {
                    std::string nextT = currentTerminal();
                    if (top == "factor" && tokens[pos].type == TokenType::MINUS) nextT = "-";
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

SemanticValue Parser::buildSemanticValue(const Production& production,
                                         const std::vector<SemanticValue>& rhsValues) {
    const std::string& lhs = production.lhs;

    if (lhs == "programstruct" && rhsEquals(production, {"program_head", ";", "program_body", "."})) {
        auto program = programValue(rhsValues[0]);
        if (!program) program = std::make_shared<ProgramNode>();
        program->block = blockValue(rhsValues[2]);
        return program;
    }

    if (lhs == "program_head" && rhsEquals(production, {"program", "id", "(", "idlist", ")"})) {
        auto program = std::make_shared<ProgramNode>();
        program->name = tokenLexeme(rhsValues[1]);
        program->parameters = stringListValue(rhsValues[3]);
        return program;
    }

    if (lhs == "program_head" && rhsEquals(production, {"program", "id", "program_head'"})) {
        auto program = std::make_shared<ProgramNode>();
        program->name = tokenLexeme(rhsValues[1]);
        program->parameters = stringListValue(rhsValues[2]);
        return program;
    }

    if (lhs == "program_head" && rhsEquals(production, {"program", "id"})) {
        auto program = std::make_shared<ProgramNode>();
        program->name = tokenLexeme(rhsValues[1]);
        return program;
    }

    if (lhs == "program_head'" && rhsEquals(production, {"(", "idlist", ")"})) {
        return stringListValue(rhsValues[1]);
    }

    if (lhs == "program_head'" && production.rhs.empty()) {
        return std::vector<std::string>{};
    }

    if (lhs == "program_body" &&
        rhsEquals(production, {"const_declarations", "var_declarations",
                               "subprogram_declarations", "compound_statement"})) {
        auto block = std::make_shared<BlockNode>();
        block->constDecls = declListValue(rhsValues[0]);
        block->varDecls = declListValue(rhsValues[1]);
        block->subprogramDecls = declListValue(rhsValues[2]);
        block->compoundStmt = stmtValue(rhsValues[3]);
        return block;
    }

    if (lhs == "idlist" && rhsEquals(production, {"id", "idlist'"})) {
        std::vector<std::string> names{tokenLexeme(rhsValues[0])};
        const auto tail = stringListValue(rhsValues[1]);
        names.insert(names.end(), tail.begin(), tail.end());
        return names;
    }

    if (lhs == "idlist'" && rhsEquals(production, {",", "id", "idlist'"})) {
        std::vector<std::string> names{tokenLexeme(rhsValues[1])};
        const auto tail = stringListValue(rhsValues[2]);
        names.insert(names.end(), tail.begin(), tail.end());
        return names;
    }

    if (lhs == "idlist'" && production.rhs.empty()) return std::vector<std::string>{};

    if (lhs == "const_declarations" && production.rhs.empty()) return DeclNodeList{};
    if (lhs == "const_declarations" && rhsEquals(production, {"const", "const_decl_list"})) {
        return declListValue(rhsValues[1]);
    }

    if (lhs == "const_decl_list" &&
        rhsEquals(production, {"id", "=", "const_value", ";", "const_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<ConstDeclNode>();
        node->name = tokenLexeme(rhsValues[0]);
        node->value = exprValue(rhsValues[2]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[4]));
        return decls;
    }

    if (lhs == "const_decl_list_tail" &&
        rhsEquals(production, {"id", "=", "const_value", ";", "const_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<ConstDeclNode>();
        node->name = tokenLexeme(rhsValues[0]);
        node->value = exprValue(rhsValues[2]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[4]));
        return decls;
    }

    if (lhs == "const_decl_list_tail" && production.rhs.empty()) return DeclNodeList{};

    if (lhs == "const_value" && rhsEquals(production, {"+", "num"})) {
        auto literal = std::make_shared<LiteralExprNode>();
        literal->literalType = "num";
        literal->value = "+" + tokenLexeme(rhsValues[1]);
        return std::static_pointer_cast<ExprNode>(literal);
    }

    if (lhs == "const_value" && rhsEquals(production, {"-", "num"})) {
        auto literal = std::make_shared<LiteralExprNode>();
        literal->literalType = "num";
        literal->value = "-" + tokenLexeme(rhsValues[1]);
        return std::static_pointer_cast<ExprNode>(literal);
    }

    if (lhs == "const_value" && rhsEquals(production, {"num"})) {
        auto literal = std::make_shared<LiteralExprNode>();
        literal->literalType = "num";
        literal->value = tokenLexeme(rhsValues[0]);
        return std::static_pointer_cast<ExprNode>(literal);
    }

    if (lhs == "const_value" && rhsEquals(production, {"letter"})) {
        auto literal = std::make_shared<LiteralExprNode>();
        literal->literalType = "char";
        literal->value = tokenLexeme(rhsValues[0]);
        return std::static_pointer_cast<ExprNode>(literal);
    }

    if (lhs == "var_declarations" && production.rhs.empty()) return DeclNodeList{};
    if (lhs == "var_declarations" && rhsEquals(production, {"var", "var_decl_list"})) {
        return declListValue(rhsValues[1]);
    }

    if (lhs == "var_decl_list" &&
        rhsEquals(production, {"idlist", ":", "type", ";", "var_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<VarDeclNode>();
        node->names = stringListValue(rhsValues[0]);
        node->typeName = stringValue(rhsValues[2]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[4]));
        return decls;
    }

    if (lhs == "var_decl_list" &&
        rhsEquals(production, {"id", "idlist'", ":", "type", ";", "var_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<VarDeclNode>();
        node->names.push_back(tokenLexeme(rhsValues[0]));
        const auto tailNames = stringListValue(rhsValues[1]);
        node->names.insert(node->names.end(), tailNames.begin(), tailNames.end());
        node->typeName = stringValue(rhsValues[3]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[5]));
        return decls;
    }

    if (lhs == "var_decl_list_tail" &&
        rhsEquals(production, {"idlist", ":", "type", ";", "var_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<VarDeclNode>();
        node->names = stringListValue(rhsValues[0]);
        node->typeName = stringValue(rhsValues[2]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[4]));
        return decls;
    }

    if (lhs == "var_decl_list_tail" &&
        rhsEquals(production, {"id", "idlist'", ":", "type", ";", "var_decl_list_tail"})) {
        DeclNodeList decls;
        auto node = std::make_shared<VarDeclNode>();
        node->names.push_back(tokenLexeme(rhsValues[0]));
        const auto tailNames = stringListValue(rhsValues[1]);
        node->names.insert(node->names.end(), tailNames.begin(), tailNames.end());
        node->typeName = stringValue(rhsValues[3]);
        decls.push_back(node);
        appendDeclList(decls, declListValue(rhsValues[5]));
        return decls;
    }

    if (lhs == "var_decl_list_tail" && production.rhs.empty()) return DeclNodeList{};

    if (lhs == "type" && rhsEquals(production, {"basic_type"})) return typeNameFromBasic(rhsValues[0]);
    if (lhs == "type" && rhsEquals(production, {"array", "[", "period", "]", "of", "basic_type"})) {
        return "array[" + stringValue(rhsValues[2]) + "] of " + typeNameFromBasic(rhsValues[5]);
    }

    if (lhs == "basic_type" && production.rhs.size() == 1) return tokenLexeme(rhsValues[0]);

    if (lhs == "period" && rhsEquals(production, {"num", "..", "num", "period'"})) {
        std::string range = tokenLexeme(rhsValues[0]) + ".." + tokenLexeme(rhsValues[2]);
        const std::string tail = stringValue(rhsValues[3]);
        if (!tail.empty()) range += tail;
        return range;
    }

    if (lhs == "period'" && rhsEquals(production, {",", "num", "..", "num", "period'"})) {
        std::string range = "," + tokenLexeme(rhsValues[1]) + ".." + tokenLexeme(rhsValues[3]);
        const std::string tail = stringValue(rhsValues[4]);
        if (!tail.empty()) range += tail;
        return range;
    }

    if (lhs == "period'" && production.rhs.empty()) return std::string{};

    if (lhs == "subprogram_declarations" && production.rhs.empty()) return DeclNodeList{};
    if (lhs == "subprogram_declarations" &&
        rhsEquals(production, {"subprogram", ";", "subprogram_declarations"})) {
        DeclNodeList decls;
        const auto sub = declValue(rhsValues[0]);
        if (sub) decls.push_back(sub);
        appendDeclList(decls, declListValue(rhsValues[2]));
        return decls;
    }

    if (lhs == "subprogram") return DeclNodePtr{};

    if (lhs == "subprogram_head" || lhs == "formal_parameter" || lhs == "parameter_list" ||
        lhs == "parameter_list'" || lhs == "parameter" || lhs == "var_parameter" ||
        lhs == "value_parameter") {
        return std::monostate{};
    }

    if (lhs == "subprogram_body" &&
        rhsEquals(production, {"const_declarations", "var_declarations", "compound_statement"})) {
        auto block = std::make_shared<BlockNode>();
        block->constDecls = declListValue(rhsValues[0]);
        block->varDecls = declListValue(rhsValues[1]);
        block->compoundStmt = stmtValue(rhsValues[2]);
        return block;
    }

    if (lhs == "compound_statement" && rhsEquals(production, {"begin", "statement_list", "end"})) {
        auto compound = std::make_shared<CompoundStmtNode>();
        compound->statements = stmtListValue(rhsValues[1]);
        return std::static_pointer_cast<StmtNode>(compound);
    }

    if (lhs == "statement_list" && rhsEquals(production, {"statement", "statement_list'"})) {
        StmtNodeList list;
        const auto first = stmtValue(rhsValues[0]);
        if (first) list.push_back(first);
        appendStmtList(list, stmtListValue(rhsValues[1]));
        return list;
    }

    if (lhs == "statement_list'" && rhsEquals(production, {";", "statement", "statement_list'"})) {
        StmtNodeList list;
        const auto first = stmtValue(rhsValues[1]);
        if (first) list.push_back(first);
        appendStmtList(list, stmtListValue(rhsValues[2]));
        return list;
    }

    if (lhs == "statement_list'" && production.rhs.empty()) return StmtNodeList{};
    if (lhs == "statement" && production.rhs.empty()) {
        return std::static_pointer_cast<StmtNode>(std::make_shared<EmptyStmtNode>());
    }

    if (lhs == "statement" && rhsEquals(production, {"id", "statement_id_tail"})) {
        const std::string name = tokenLexeme(rhsValues[0]);
        const StatementTail tail = statementTailValue(rhsValues[1]);

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

    if (lhs == "statement" && rhsEquals(production, {"begin", "statement_list", "end"})) {
        auto compound = std::make_shared<CompoundStmtNode>();
        compound->statements = stmtListValue(rhsValues[1]);
        return std::static_pointer_cast<StmtNode>(compound);
    }

    if (lhs == "statement" && rhsEquals(production, {"if", "expression", "then", "statement", "else_part"})) {
        auto node = std::make_shared<IfStmtNode>();
        node->condition = exprValue(rhsValues[1]);
        node->thenStmt = stmtValue(rhsValues[3]);
        node->elseStmt = stmtValue(rhsValues[4]);
        return std::static_pointer_cast<StmtNode>(node);
    }

    if (lhs == "statement" &&
        rhsEquals(production, {"for", "id", "assignop", "expression", "to", "expression", "do", "statement"})) {
        auto node = std::make_shared<ForStmtNode>();
        node->iterator = tokenLexeme(rhsValues[1]);
        node->startExpr = exprValue(rhsValues[3]);
        node->endExpr = exprValue(rhsValues[5]);
        node->body = stmtValue(rhsValues[7]);
        return std::static_pointer_cast<StmtNode>(node);
    }

    if (lhs == "statement" && rhsEquals(production, {"read", "(", "variable_list", ")"})) {
        auto node = std::make_shared<ReadStmtNode>();
        node->variables = exprListValue(rhsValues[2]);
        return std::static_pointer_cast<StmtNode>(node);
    }

    if (lhs == "statement" && rhsEquals(production, {"write", "(", "expression_list", ")"})) {
        auto node = std::make_shared<WriteStmtNode>();
        node->expressions = exprListValue(rhsValues[2]);
        return std::static_pointer_cast<StmtNode>(node);
    }

    if (lhs == "statement_id_tail" && rhsEquals(production, {"assignop", "expression"})) {
        StatementTail tail;
        tail.kind = StatementTail::Kind::Assign;
        tail.value = exprValue(rhsValues[1]);
        return tail;
    }

    if (lhs == "statement_id_tail" &&
        rhsEquals(production, {"[", "expression_list", "]", "assignop", "expression"})) {
        StatementTail tail;
        tail.kind = StatementTail::Kind::IndexedAssign;
        tail.expressions = exprListValue(rhsValues[1]);
        tail.value = exprValue(rhsValues[4]);
        return tail;
    }

    if (lhs == "statement_id_tail" && rhsEquals(production, {"(", "expression_list", ")"})) {
        StatementTail tail;
        tail.kind = StatementTail::Kind::Call;
        tail.expressions = exprListValue(rhsValues[1]);
        return tail;
    }

    if (lhs == "statement_id_tail" && production.rhs.empty()) return StatementTail{};

    if (lhs == "variable_list" && rhsEquals(production, {"variable", "variable_list'"})) {
        ExprNodeList list;
        const auto first = exprValue(rhsValues[0]);
        if (first) list.push_back(first);
        appendExprList(list, exprListValue(rhsValues[1]));
        return list;
    }

    if (lhs == "variable_list'" && rhsEquals(production, {",", "variable", "variable_list'"})) {
        ExprNodeList list;
        const auto first = exprValue(rhsValues[1]);
        if (first) list.push_back(first);
        appendExprList(list, exprListValue(rhsValues[2]));
        return list;
    }

    if (lhs == "variable_list'" && production.rhs.empty()) return ExprNodeList{};

    if (lhs == "variable" && rhsEquals(production, {"id", "id_varpart"})) {
        auto variable = std::make_shared<VariableExprNode>();
        variable->name = tokenLexeme(rhsValues[0]);
        variable->indices = exprListValue(rhsValues[1]);
        return std::static_pointer_cast<ExprNode>(variable);
    }

    if (lhs == "id_varpart" && production.rhs.empty()) return ExprNodeList{};
    if (lhs == "id_varpart" && rhsEquals(production, {"[", "expression_list", "]"})) {
        return exprListValue(rhsValues[1]);
    }

    if (lhs == "else_part" && production.rhs.empty()) return StmtNodePtr{};
    if (lhs == "else_part" && rhsEquals(production, {"else", "statement"})) {
        return stmtValue(rhsValues[1]);
    }

    if (lhs == "expression_list" && rhsEquals(production, {"expression", "expression_list'"})) {
        ExprNodeList list;
        const auto first = exprValue(rhsValues[0]);
        if (first) list.push_back(first);
        appendExprList(list, exprListValue(rhsValues[1]));
        return list;
    }

    if (lhs == "expression_list'" && rhsEquals(production, {",", "expression", "expression_list'"})) {
        ExprNodeList list;
        const auto first = exprValue(rhsValues[1]);
        if (first) list.push_back(first);
        appendExprList(list, exprListValue(rhsValues[2]));
        return list;
    }

    if (lhs == "expression_list'" && production.rhs.empty()) return ExprNodeList{};

    if (lhs == "expression" && rhsEquals(production, {"simple_expression", "expression_tail"})) {
        auto left = exprValue(rhsValues[0]);
        const auto tail = relTailValue(rhsValues[1]);
        if (tail.op.empty()) return left;

        auto binary = std::make_shared<BinaryExprNode>();
        binary->op = tail.op;
        binary->left = left;
        binary->right = tail.rhs;
        return std::static_pointer_cast<ExprNode>(binary);
    }

    if (lhs == "expression_tail" && rhsEquals(production, {"relop", "simple_expression"})) {
        RelOpTail tail;
        tail.op = tokenLexeme(rhsValues[0]);
        tail.rhs = exprValue(rhsValues[1]);
        return tail;
    }

    if (lhs == "expression_tail" && production.rhs.empty()) return RelOpTail{};

    if (lhs == "simple_expression" && rhsEquals(production, {"term", "simple_expression'"})) {
        return foldLeftAssociative(exprValue(rhsValues[0]), exprChainValue(rhsValues[1]));
    }

    if (lhs == "simple_expression'" &&
        rhsEquals(production, {"addop", "term", "simple_expression'"})) {
        ExprChain chain;
        chain.items.push_back({tokenLexeme(rhsValues[0]), exprValue(rhsValues[1])});
        const auto tail = exprChainValue(rhsValues[2]);
        chain.items.insert(chain.items.end(), tail.items.begin(), tail.items.end());
        return chain;
    }

    if (lhs == "simple_expression'" && production.rhs.empty()) return ExprChain{};

    if (lhs == "term" && rhsEquals(production, {"factor", "term'"})) {
        return foldLeftAssociative(exprValue(rhsValues[0]), exprChainValue(rhsValues[1]));
    }

    if (lhs == "term'" && rhsEquals(production, {"mulop", "factor", "term'"})) {
        ExprChain chain;
        chain.items.push_back({tokenLexeme(rhsValues[0]), exprValue(rhsValues[1])});
        const auto tail = exprChainValue(rhsValues[2]);
        chain.items.insert(chain.items.end(), tail.items.begin(), tail.items.end());
        return chain;
    }

    if (lhs == "term'" && production.rhs.empty()) return ExprChain{};

    if (lhs == "factor" && rhsEquals(production, {"num"})) {
        auto literal = std::make_shared<LiteralExprNode>();
        literal->literalType = "num";
        literal->value = tokenLexeme(rhsValues[0]);
        return std::static_pointer_cast<ExprNode>(literal);
    }

    if (lhs == "factor" && rhsEquals(production, {"id", "factor_id_tail"})) {
        const std::string name = tokenLexeme(rhsValues[0]);
        const FactorTail tail = factorTailValue(rhsValues[1]);

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

    if (lhs == "factor" && rhsEquals(production, {"(", "expression", ")"})) {
        return exprValue(rhsValues[1]);
    }

    if (lhs == "factor" && rhsEquals(production, {"not", "factor"})) {
        auto node = std::make_shared<UnaryExprNode>();
        node->op = "not";
        node->operand = exprValue(rhsValues[1]);
        return std::static_pointer_cast<ExprNode>(node);
    }

    if (lhs == "factor" && rhsEquals(production, {"-", "factor"})) {
        auto node = std::make_shared<UnaryExprNode>();
        node->op = "-";
        node->operand = exprValue(rhsValues[1]);
        return std::static_pointer_cast<ExprNode>(node);
    }

    if (lhs == "factor_id_tail" && rhsEquals(production, {"[", "expression_list", "]"})) {
        FactorTail tail;
        tail.kind = FactorTail::Kind::Index;
        tail.expressions = exprListValue(rhsValues[1]);
        return tail;
    }

    if (lhs == "factor_id_tail" && rhsEquals(production, {"(", "expression_list", ")"})) {
        FactorTail tail;
        tail.kind = FactorTail::Kind::Call;
        tail.expressions = exprListValue(rhsValues[1]);
        return tail;
    }

    if (lhs == "factor_id_tail" && production.rhs.empty()) return FactorTail{};

    return std::monostate{};
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
