#include "parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>

// ==================== Token Reader ====================

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
        if (line.empty()) continue;
        // Skip comment lines starting with #
        if (line[0] == '#') continue;

        std::istringstream iss(line);
        std::string typeName, lexeme;
        int ln = 0, col = 0;
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

// ==================== Parser ====================

Parser::Parser(Grammar& grammar, const std::vector<Token>& tokens)
    : grammar(grammar), tokens(tokens), pos(0) {}

std::string Parser::mapTokenToGrammarTerminal(const Token& tok) const {
    // Map relational operators to "relop"
    switch (tok.type) {
        case TokenType::EQ: case TokenType::NE:
        case TokenType::LT: case TokenType::LE:
        case TokenType::GT: case TokenType::GE:
            return "relop";
        // Map additive operators to "addop"
        case TokenType::PLUS:
            return "addop";
        case TokenType::MINUS:
            // Context-sensitive: in factor context '-' is uminus, elsewhere addop
            // We handle '-' as both addop and uminus in the grammar.
            // The parse table will determine via stack top which applies.
            return "addop";
        case TokenType::OR_KW:
            return "addop";
        // Map multiplicative operators to "mulop"
        case TokenType::MULTIPLY: case TokenType::DIVIDE:
        case TokenType::DIV_KW: case TokenType::MOD:
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
    if (pos >= tokens.size()) {
        return {TokenType::END_OF_FILE, "EOF", 0, 0};
    }
    return tokens[pos];
}

void Parser::advance() {
    if (pos < tokens.size()) pos++;
}

void Parser::addError(const std::string& msg) {
    Token tok = currentToken();
    errors.push_back({tok.line, tok.column, msg});
}

std::string Parser::stackToString(const std::stack<std::string>& st) const {
    // Copy stack to vector for display
    std::stack<std::string> tmp = st;
    std::vector<std::string> v;
    while (!tmp.empty()) {
        v.push_back(tmp.top());
        tmp.pop();
    }
    std::string result;
    for (auto it = v.rbegin(); it != v.rend(); ++it) {
        if (!result.empty()) result += " ";
        result += *it;
    }
    return result;
}

std::string Parser::remainingInput() const {
    std::string result;
    for (size_t i = pos; i < tokens.size() && i < pos + 10; i++) {
        if (!result.empty()) result += " ";
        result += tokens[i].lexeme;
    }
    if (pos + 10 < tokens.size()) result += " ...";
    if (pos >= tokens.size()) result = "$";
    return result;
}

bool Parser::parse() {
    errors.clear();
    steps.clear();
    pos = 0;

    std::stack<std::string> stk;
    stk.push("$");
    stk.push(grammar.startSymbol);

    int maxSteps = 100000; // safety limit
    int stepCount = 0;

    while (!stk.empty() && stepCount < maxSteps) {
        stepCount++;
        std::string top = stk.top();
        std::string input = currentTerminal();
        Token tok = currentToken();

        // Record step
        ParseStep step;
        step.stack = stackToString(stk);
        step.input = remainingInput();

        if (top == "$") {
            if (input == "$") {
                step.action = "ACCEPT";
                steps.push_back(step);
                return errors.empty();
            } else {
                step.action = "ERROR: unexpected tokens after program end";
                steps.push_back(step);
                addError("Unexpected tokens after program end near '" + tok.lexeme + "'");
                return false;
            }
        }

        // Check if top is terminal
        if (!grammar.nonTerminals.count(top)) {
            // Terminal on stack
            // Special handling: factor uses "-" directly, but input maps to "addop"
            // We need to match "-" on stack with "-" token
            bool match = false;
            if (top == input) {
                match = true;
            } else if (top == "-" && tok.type == TokenType::MINUS) {
                match = true;
            } else if (top == "+" && tok.type == TokenType::PLUS) {
                match = true;
            } else if (top == "=" && tok.type == TokenType::EQ) {
                match = true;
            } else if (top == "addop" && (tok.type == TokenType::PLUS || tok.type == TokenType::MINUS || tok.type == TokenType::OR_KW)) {
                match = true;
            } else if (top == "relop" && (tok.type == TokenType::EQ || tok.type == TokenType::NE ||
                        tok.type == TokenType::LT || tok.type == TokenType::LE ||
                        tok.type == TokenType::GT || tok.type == TokenType::GE)) {
                match = true;
            } else if (top == "mulop" && (tok.type == TokenType::MULTIPLY || tok.type == TokenType::DIVIDE ||
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
                advance();
            } else {
                step.action = "ERROR: expected '" + top + "', got '" + tok.lexeme + "'";
                steps.push_back(step);
                addError("Expected '" + top + "', but got '" + tok.lexeme + "' (line " +
                         std::to_string(tok.line) + ", col " + std::to_string(tok.column) + ")");
                // Error recovery: pop the expected terminal and continue (panic mode)
                stk.pop();
            }
        } else {
            // Non-terminal on stack: consult parse table
            // Determine the lookup terminal
            std::string lookupTerminal = input;

            // Special: if top is "factor" and input is "-" (mapped to addop), look up "-"
            if (top == "factor" && tok.type == TokenType::MINUS) {
                lookupTerminal = "-";
            }
            // Special: if top is "const_value" and (input is "+" or "-")
            if (top == "const_value" && tok.type == TokenType::PLUS) {
                lookupTerminal = "+";
            }
            if (top == "const_value" && tok.type == TokenType::MINUS) {
                lookupTerminal = "-";
            }
            // Special: '=' is used literally in const declarations (id = const_value)
            // but EQ token maps to "relop". When a non-terminal expects literal '=',
            // use '=' for lookup.
            if (tok.type == TokenType::EQ) {
                auto rowCheck = grammar.parseTable.find(top);
                if (rowCheck != grammar.parseTable.end()) {
                    if (rowCheck->second.find("=") != rowCheck->second.end() &&
                        rowCheck->second.find("relop") == rowCheck->second.end()) {
                        lookupTerminal = "=";
                    } else if (rowCheck->second.find("=") != rowCheck->second.end() &&
                               rowCheck->second.find("relop") != rowCheck->second.end()) {
                        // Both exist — context issue; prefer '=' if "relop" wouldn't work
                        lookupTerminal = "=";
                    }
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
                // Error recovery: try to find a synchronizing token
                step.action = "ERROR: no entry for [" + top + ", " + lookupTerminal + "]";
                steps.push_back(step);

                std::string expected;
                for (auto eit = rowIt->second.begin(); eit != rowIt->second.end(); ++eit) {
                    if (!expected.empty()) expected += ", ";
                    expected += "'" + eit->first + "'";
                }
                addError("Syntax error at '" + tok.lexeme + "' (line " +
                         std::to_string(tok.line) + ", col " + std::to_string(tok.column) +
                         "): expected one of {" + expected + "} for non-terminal '" + top + "'");

                // Panic mode error recovery:
                // Skip input tokens until we find one in FOLLOW(top) or in FIRST(top),
                // or pop the non-terminal
                bool recovered = false;
                auto& followSet = grammar.followSets[top];
                auto& firstSet = grammar.firstSets[top];

                // First try: if current input is in FOLLOW, pop the non-terminal (assume ε)
                if (followSet.count(lookupTerminal)) {
                    stk.pop();
                    recovered = true;
                } else {
                    // Skip tokens until we find something in FIRST or FOLLOW
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
                        // Reached EOF without recovery
                        stk.pop();
                    } else if (followSet.count(currentTerminal())) {
                        stk.pop();
                    }
                    // else: found in FIRST, retry the non-terminal
                }
                continue;
            }

            int prodIdx = cellIt->second;
            auto& prod = grammar.productions[prodIdx];

            step.action = "PREDICT " + prod.lhs + " -> ";
            if (prod.rhs.empty()) {
                step.action += "ε";
            } else {
                for (size_t k = 0; k < prod.rhs.size(); k++) {
                    if (k) step.action += " ";
                    step.action += prod.rhs[k];
                }
            }
            steps.push_back(step);

            stk.pop();
            // Push rhs in reverse order
            for (int k = (int)prod.rhs.size() - 1; k >= 0; k--) {
                stk.push(prod.rhs[k]);
            }
        }
    }

    if (stepCount >= maxSteps) {
        addError("Parse exceeded maximum steps (possible infinite loop)");
    }

    return errors.empty();
}

void Parser::printParseProcess(std::ostream& os) const {
    os << "Parse Process Log\n";
    os << "=================\n\n";

    // Column headers
    os << std::string(60, '-') << "\n";
    char buf[256];
    snprintf(buf, sizeof(buf), "%-6s %-40s %-30s %s\n", "Step", "Stack", "Input", "Action");
    os << buf;
    os << std::string(60, '-') << "\n";

    for (size_t i = 0; i < steps.size(); i++) {
        std::string stackStr = steps[i].stack;
        if (stackStr.size() > 38) stackStr = stackStr.substr(0, 35) + "...";
        std::string inputStr = steps[i].input;
        if (inputStr.size() > 28) inputStr = inputStr.substr(0, 25) + "...";

        snprintf(buf, sizeof(buf), "%-6d %-40s %-30s %s\n",
                 (int)(i + 1), stackStr.c_str(), inputStr.c_str(), steps[i].action.c_str());
        os << buf;
    }
    os << std::string(60, '-') << "\n";
}
