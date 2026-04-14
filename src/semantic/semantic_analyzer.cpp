#include "semantic_analyzer.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <set>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void SemanticAnalyzer::addError(const std::string& msg) {
    errors_.push_back({0, 0, msg});
}

void SemanticAnalyzer::addError(const ASTNode& node, const std::string& msg) {
    errors_.push_back({node.line, node.column, msg});
}

void SemanticAnalyzer::markCurrentFunctionAssigned() {
    if (!functionAssignedStack_.empty()) {
        functionAssignedStack_.back() = true;
    }
}

std::optional<std::string> SemanticAnalyzer::constantValueKey(ExprNode* expr) const {
    if (!expr) return std::nullopt;
    auto* literal = dynamic_cast<LiteralExprNode*>(expr);
    if (!literal) return std::nullopt;
    return literal->literalType + ":" + literal->value;
}

bool SemanticAnalyzer::typesCompatible(const TypeInfo& lhs, const TypeInfo& rhs) const {
    if (lhs.isArray() || rhs.isArray()) return false;  // no array-level assignment
    if (lhs.baseType == rhs.baseType) return true;
    // integer -> real promotion
    if (lhs.isReal() && rhs.isInteger()) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Type inference
// ---------------------------------------------------------------------------
TypeInfo SemanticAnalyzer::inferType(ExprNode* expr) {
    if (!expr) return TypeInfo::makeSimple("void");

    switch (expr->kind) {
        case ASTNodeKind::VariableExpr: {
            auto* ve = static_cast<VariableExprNode*>(expr);
            Symbol* sym = symTable_.lookup(ve->name);
            if (!sym) return TypeInfo::makeSimple("void");
            if (sym->kind == SymbolKind::Function && ve->indices.empty() && ve->fields.empty()) {
                return TypeInfo::makeSimple(sym->returnType);
            }
            TypeInfo current = sym->type;
            if (current.isArray()) {
                if (!ve->indices.empty()) {
                    current = TypeInfo::fromString(current.elementType);
                } else {
                    return current;
                }
            }
            for (const auto& field : ve->fields) {
                if (!current.isRecord()) return TypeInfo::makeSimple("void");
                const TypeInfo* fieldType = current.findField(field);
                if (!fieldType) return TypeInfo::makeSimple("void");
                current = *fieldType;
            }
            return current;
        }

        case ASTNodeKind::LiteralExpr: {
            auto* le = static_cast<LiteralExprNode*>(expr);
            if (le->literalType == "integer" || le->literalType == "num") {
                // Check for decimal point
                if (le->value.find('.') != std::string::npos)
                    return TypeInfo::makeSimple("real");
                return TypeInfo::makeSimple("integer");
            }
            if (le->literalType == "real")   return TypeInfo::makeSimple("real");
            if (le->literalType == "char" || le->literalType == "letter")
                return TypeInfo::makeSimple("char");
            if (le->literalType == "string")
                return TypeInfo::makeSimple("string");
            return TypeInfo::makeSimple("integer");
        }

        case ASTNodeKind::UnaryExpr: {
            auto* ue = static_cast<UnaryExprNode*>(expr);
            if (ue->op == "not") return TypeInfo::makeSimple("boolean");
            return inferType(ue->operand.get());
        }

        case ASTNodeKind::BinaryExpr: {
            auto* be = static_cast<BinaryExprNode*>(expr);
            TypeInfo lt = inferType(be->left.get());
            TypeInfo rt = inferType(be->right.get());

            // Relational operators always produce boolean
            if (be->op == "=" || be->op == "<>" || be->op == "<" ||
                be->op == "<=" || be->op == ">" || be->op == ">=")
                return TypeInfo::makeSimple("boolean");

            // Logical operators
            if (be->op == "and" || be->op == "or")
                return TypeInfo::makeSimple("boolean");

            // Division always produces real in standard Pascal
            if (be->op == "/") return TypeInfo::makeSimple("real");

            // Integer-only operators
            if (be->op == "div" || be->op == "mod")
                return TypeInfo::makeSimple("integer");

            // Arithmetic: promote to real if either operand is real
            if (lt.isReal() || rt.isReal())
                return TypeInfo::makeSimple("real");
            return TypeInfo::makeSimple("integer");
        }

        case ASTNodeKind::CallExpr: {
            auto* ce = static_cast<CallExprNode*>(expr);
            Symbol* sym = symTable_.lookup(ce->callee);
            if (sym && sym->kind == SymbolKind::Function)
                return TypeInfo::makeSimple(sym->returnType);
            return TypeInfo::makeSimple("integer");
        }

        default:
            return TypeInfo::makeSimple("void");
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
bool SemanticAnalyzer::analyze(ProgramNodePtr ast) {
    errors_.clear();
    if (!ast) {
        addError("No AST to analyze (null root)");
        return false;
    }
    ast->accept(*this);
    return errors_.empty();
}

// ---------------------------------------------------------------------------
// Visitor implementations
// ---------------------------------------------------------------------------
void SemanticAnalyzer::visitProgram(ProgramNode& node) {
    symTable_.enterScope();

    // Declare program name
    Symbol progSym;
    progSym.name = node.name;
    progSym.kind = SymbolKind::Program;
    symTable_.declare(progSym);

    // Declare program parameters (input, output, etc.) as variables
    for (auto& param : node.parameters) {
        Symbol pSym;
        pSym.name = param;
        pSym.kind = SymbolKind::Variable;
        pSym.type = TypeInfo::makeSimple("integer");
        symTable_.declare(pSym);
    }

    if (node.block) node.block->accept(*this);
    symTable_.exitScope();
}

void SemanticAnalyzer::visitBlock(BlockNode& node) {
    for (auto& d : node.constDecls)      if (d) d->accept(*this);
    for (auto& d : node.varDecls)        if (d) d->accept(*this);
    for (auto& d : node.subprogramDecls) if (d) d->accept(*this);
    if (node.compoundStmt) node.compoundStmt->accept(*this);
}

void SemanticAnalyzer::visitConstDecl(ConstDeclNode& node) {
    Symbol sym;
    sym.name = node.name;
    sym.kind = SymbolKind::Constant;
    // Infer type from value
    if (node.value) {
        sym.type = inferType(node.value.get());
        auto* lit = dynamic_cast<LiteralExprNode*>(node.value.get());
        if (lit) sym.constValue = lit->value;
    } else {
        sym.type = TypeInfo::makeSimple("integer");
    }
    if (!symTable_.declare(sym))
        addError(node, "Duplicate constant declaration: '" + node.name + "'");
}

void SemanticAnalyzer::visitVarDecl(VarDeclNode& node) {
    TypeInfo ti = TypeInfo::fromString(node.typeName);
    std::function<void(const TypeInfo&, const std::string&)> validateRecordType =
        [&](const TypeInfo& type, const std::string& owner) {
            if (!type.isRecord()) return;
            std::set<std::string> seen;
            for (const auto& field : type.recordFields) {
                if (!seen.insert(field.first).second) {
                    addError(node, "Duplicate field declaration '" + field.first + "' in " + owner);
                }
                validateRecordType(field.second, "record field '" + field.first + "'");
            }
        };
    if (ti.isArray() && ti.arrayLow > ti.arrayHigh) {
        addError(node, "Invalid array bounds in declaration: lower bound exceeds upper bound");
    }
    validateRecordType(ti, "record type");
    for (auto& name : node.names) {
        Symbol sym;
        sym.name = name;
        sym.kind = SymbolKind::Variable;
        sym.type = ti;
        if (!symTable_.declare(sym))
            addError(node, "Duplicate variable declaration: '" + name + "'");
    }
}

void SemanticAnalyzer::visitSubprogramDecl(SubprogramDeclNode& node) {
    // Declare in enclosing scope
    Symbol sym;
    sym.name = node.name;
    sym.kind = (node.headerKind == "function") ? SymbolKind::Function : SymbolKind::Procedure;
    sym.returnType = node.returnType;

    // Collect parameter info
    for (auto& p : node.parameters) {
        auto* pd = dynamic_cast<ParamDeclNode*>(p.get());
        if (pd) {
            TypeInfo pType = TypeInfo::fromString(pd->typeName);
            for (auto& pname : pd->names) {
                sym.params.push_back({pname, pType, pd->byReference});
            }
        }
    }
    if (!symTable_.declare(sym))
        addError(node, "Duplicate subprogram declaration: '" + node.name + "'");

    // Enter subprogram scope
    symTable_.enterScope();
    currentFunction_ = (sym.kind == SymbolKind::Function) ? node.name : "";
    if (sym.kind == SymbolKind::Function) {
        functionAssignedStack_.push_back(false);
    }

    // Declare parameters in the new scope
    for (auto& p : node.parameters) {
        auto* pd = dynamic_cast<ParamDeclNode*>(p.get());
        if (pd) {
            TypeInfo pType = TypeInfo::fromString(pd->typeName);
            for (auto& pname : pd->names) {
                Symbol paramSym;
                paramSym.name = pname;
                paramSym.kind = SymbolKind::Parameter;
                paramSym.type = pType;
                paramSym.byReference = pd->byReference;
                if (!symTable_.declare(paramSym))
                    addError(*pd, "Duplicate parameter name: '" + pname + "'");
            }
        }
    }

    // For functions, declare the function name as a local variable (return value)
    if (sym.kind == SymbolKind::Function) {
        Symbol retSym;
        retSym.name = node.name;
        retSym.kind = SymbolKind::Variable;
        retSym.type = TypeInfo::makeSimple(node.returnType);
        symTable_.declare(retSym);  // shadows the function in current scope
    }

    if (node.body) node.body->accept(*this);

    if (sym.kind == SymbolKind::Function) {
        if (!functionAssignedStack_.back()) {
            addError(node, "Function '" + node.name + "' has no return assignment");
        }
        functionAssignedStack_.pop_back();
    }

    currentFunction_ = "";
    symTable_.exitScope();
}

void SemanticAnalyzer::visitParamDecl(ParamDeclNode& /*node*/) {
    // Handled inside visitSubprogramDecl
}

void SemanticAnalyzer::visitCompoundStmt(CompoundStmtNode& node) {
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void SemanticAnalyzer::visitAssignStmt(AssignStmtNode& node) {
    if (node.target) node.target->accept(*this);
    if (node.value)  node.value->accept(*this);

    if (node.target && node.value) {
        if (auto* ve = dynamic_cast<VariableExprNode*>(node.target.get())) {
            Symbol* targetSym = symTable_.lookup(ve->name);
            if (targetSym && targetSym->kind == SymbolKind::Constant) {
                addError(*ve, "Cannot assign to constant '" + ve->name + "'");
            }
            if (!currentFunction_.empty() && ve->name == currentFunction_ && ve->indices.empty()) {
                markCurrentFunctionAssigned();
            }
        }
        TypeInfo lhsType = inferType(node.target.get());
        TypeInfo rhsType = inferType(node.value.get());
        if (!typesCompatible(lhsType, rhsType)) {
            auto* ve = dynamic_cast<VariableExprNode*>(node.target.get());
            std::string name = ve ? ve->name : "<expr>";
            addError(node, "Type mismatch in assignment to '" + name +
                     "': cannot assign " + rhsType.toString() +
                     " to " + lhsType.toString());
        }
    }
}

void SemanticAnalyzer::visitCallStmt(CallStmtNode& node) {
    Symbol* sym = symTable_.lookup(node.callee);
    if (!sym) {
        // Built-in procedures: read, write, readln, writeln
        std::string lower = node.callee;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        if (lower != "read" && lower != "write" &&
            lower != "readln" && lower != "writeln") {
            addError(node, "Undeclared procedure: '" + node.callee + "'");
        }
    } else if (sym->kind != SymbolKind::Procedure && sym->kind != SymbolKind::Function) {
        addError(node, "'" + node.callee + "' is not callable");
    } else {
        if (node.arguments.size() != sym->params.size()) {
            addError(node, "Call to '" + node.callee + "' expects " +
                     std::to_string(sym->params.size()) + " argument(s), got " +
                     std::to_string(node.arguments.size()));
        } else {
            for (size_t i = 0; i < node.arguments.size(); ++i) {
                if (node.arguments[i]) node.arguments[i]->accept(*this);
                TypeInfo actual = inferType(node.arguments[i].get());
                const ParamInfo& expected = sym->params[i];
                if (!typesCompatible(expected.type, actual)) {
                    addError(*node.arguments[i], "Argument " + std::to_string(i + 1) + " of '" + node.callee +
                             "' expects " + expected.type.toString() +
                             ", got " + actual.toString());
                }
                if (expected.byReference &&
                    !dynamic_cast<VariableExprNode*>(node.arguments[i].get())) {
                    addError(*node.arguments[i], "Argument " + std::to_string(i + 1) + " of '" + node.callee +
                             "' must be a variable");
                }
            }
            return;
        }
    }
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void SemanticAnalyzer::visitIfStmt(IfStmtNode& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenStmt)  node.thenStmt->accept(*this);
    if (node.elseStmt)  node.elseStmt->accept(*this);
}

void SemanticAnalyzer::visitForStmt(ForStmtNode& node) {
    // Iterator must be declared and integer
    Symbol* sym = symTable_.lookup(node.iterator);
    if (!sym) {
        addError(node, "Undeclared for-loop iterator: '" + node.iterator + "'");
    } else if (!sym->type.isInteger()) {
        addError(node, "For-loop iterator '" + node.iterator + "' must be integer, got " +
                 sym->type.toString());
    }
    if (node.startExpr) node.startExpr->accept(*this);
    if (node.endExpr)   node.endExpr->accept(*this);
    if (node.startExpr && !inferType(node.startExpr.get()).isInteger()) {
        addError(*node.startExpr, "For-loop start expression must be integer");
    }
    if (node.endExpr && !inferType(node.endExpr.get()).isInteger()) {
        addError(*node.endExpr, "For-loop end expression must be integer");
    }
    ++loopDepth_;
    if (node.body)      node.body->accept(*this);
    --loopDepth_;
}

void SemanticAnalyzer::visitWhileStmt(WhileStmtNode& node) {
    if (node.condition) node.condition->accept(*this);
    TypeInfo condType = inferType(node.condition.get());
    if (!condType.isBoolean()) {
        addError(*node.condition, "While condition must be boolean");
    }
    ++loopDepth_;
    if (node.body) node.body->accept(*this);
    --loopDepth_;
}

void SemanticAnalyzer::visitCaseStmt(CaseStmtNode& node) {
    if (node.expression) node.expression->accept(*this);
    TypeInfo caseType = inferType(node.expression.get());
    std::set<std::string> seenLabels;

    ++caseDepth_;
    for (auto& branch : node.branches) {
        for (auto& label : branch.labels) {
            if (label) {
                label->accept(*this);
                if (auto key = constantValueKey(label.get())) {
                    if (!seenLabels.insert(*key).second) {
                        addError(*label, "Duplicate case label '" +
                                 key->substr(key->find(':') + 1) + "'");
                    }
                }
                TypeInfo labelType = inferType(label.get());
                if (!typesCompatible(caseType, labelType) &&
                    !typesCompatible(labelType, caseType)) {
                    addError(*label, "Case label type mismatch: expected " + caseType.toString() +
                             ", got " + labelType.toString());
                }
            }
        }
        if (branch.statement) branch.statement->accept(*this);
    }
    --caseDepth_;
}

void SemanticAnalyzer::visitBreakStmt(BreakStmtNode& node) {
    if (loopDepth_ <= 0 && caseDepth_ <= 0) {
        addError(node, "'break' can only appear inside for-loop or case statement");
    }
}

void SemanticAnalyzer::visitContinueStmt(ContinueStmtNode& node) {
    if (loopDepth_ <= 0) {
        addError(node, "'continue' can only appear inside for-loop");
    }
}

void SemanticAnalyzer::visitReadStmt(ReadStmtNode& node) {
    for (auto& v : node.variables) {
        if (v) {
            v->accept(*this);
            // read arguments should be variables
            auto* ve = dynamic_cast<VariableExprNode*>(v.get());
            if (!ve) addError(*v, "read() argument must be a variable");
        }
    }
}

void SemanticAnalyzer::visitWriteStmt(WriteStmtNode& node) {
    for (auto& e : node.expressions) {
        if (e) e->accept(*this);
    }
}

void SemanticAnalyzer::visitEmptyStmt(EmptyStmtNode& /*node*/) {}

void SemanticAnalyzer::visitVariableExpr(VariableExprNode& node) {
    Symbol* sym = symTable_.lookup(node.name);
    if (!sym) {
        addError(node, "Undeclared identifier: '" + node.name + "'");
        return;
    }

    TypeInfo current = sym->type;
    if (!node.indices.empty()) {
        if (!current.isArray()) {
            addError(node, "'" + node.name + "' is not an array");
            return;
        }
        for (auto& idx : node.indices) {
            if (idx) {
                idx->accept(*this);
                TypeInfo idxType = inferType(idx.get());
                if (!idxType.isInteger())
                    addError(*idx, "Array index for '" + node.name + "' must be integer");
                if (auto* literal = dynamic_cast<LiteralExprNode*>(idx.get());
                    literal && idxType.isInteger()) {
                    try {
                        int value = std::stoi(literal->value);
                        if (value < current.arrayLow || value > current.arrayHigh) {
                            addError(*idx, "Array index " + std::to_string(value) + " out of bounds for '" +
                                     node.name + "' [" + std::to_string(current.arrayLow) + ".." +
                                     std::to_string(current.arrayHigh) + "]");
                        }
                    } catch (...) {
                    }
                }
            }
        }
        current = TypeInfo::fromString(current.elementType);
    }

    for (const auto& field : node.fields) {
        if (!current.isRecord()) {
            addError(node, "Cannot access field '" + field + "' on non-record '" + node.name + "'");
            return;
        }
        const TypeInfo* fieldType = current.findField(field);
        if (!fieldType) {
            addError(node, "Record '" + node.name + "' has no field '" + field + "'");
            return;
        }
        current = *fieldType;
    }
}

void SemanticAnalyzer::visitLiteralExpr(LiteralExprNode& /*node*/) {}

void SemanticAnalyzer::visitUnaryExpr(UnaryExprNode& node) {
    if (node.operand) node.operand->accept(*this);
    TypeInfo operandType = inferType(node.operand.get());
    if (node.op == "not" && !operandType.isBoolean()) {
        addError(node, "Operator 'not' requires boolean operand");
    }
    if (node.op == "-" && !operandType.isNumeric()) {
        addError(node, "Unary '-' requires numeric operand");
    }
}

void SemanticAnalyzer::visitBinaryExpr(BinaryExprNode& node) {
    if (node.left)  node.left->accept(*this);
    if (node.right) node.right->accept(*this);

    TypeInfo lt = inferType(node.left.get());
    TypeInfo rt = inferType(node.right.get());

    if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/") {
        if (!lt.isNumeric() || !rt.isNumeric()) {
            addError(node, "Operator '" + node.op + "' requires numeric operands");
        }
        return;
    }

    if (node.op == "div" || node.op == "mod") {
        if (!lt.isInteger())
            addError(node, "Left operand of '" + node.op + "' must be integer");
        if (!rt.isInteger())
            addError(node, "Right operand of '" + node.op + "' must be integer");
        return;
    }

    if (node.op == "and" || node.op == "or") {
        if (!lt.isBoolean() || !rt.isBoolean()) {
            addError(node, "Operator '" + node.op + "' requires boolean operands");
        }
        return;
    }

    if (node.op == "=" || node.op == "<>" || node.op == "<" ||
        node.op == "<=" || node.op == ">" || node.op == ">=") {
        if (!typesCompatible(lt, rt) && !typesCompatible(rt, lt)) {
            addError(node, "Operands of '" + node.op + "' are incompatible: " +
                     lt.toString() + " and " + rt.toString());
        }
    }
}

void SemanticAnalyzer::visitCallExpr(CallExprNode& node) {
    Symbol* sym = symTable_.lookup(node.callee);
    if (!sym) {
        addError(node, "Undeclared function: '" + node.callee + "'");
    } else if (sym->kind != SymbolKind::Function) {
        addError(node, "'" + node.callee + "' is not a function");
    } else {
        if (node.arguments.size() != sym->params.size()) {
            addError(node, "Function '" + node.callee + "' expects " +
                     std::to_string(sym->params.size()) + " argument(s), got " +
                     std::to_string(node.arguments.size()));
        } else {
            for (size_t i = 0; i < node.arguments.size(); ++i) {
                if (node.arguments[i]) node.arguments[i]->accept(*this);
                TypeInfo actual = inferType(node.arguments[i].get());
                const ParamInfo& expected = sym->params[i];
                if (!typesCompatible(expected.type, actual)) {
                    addError(*node.arguments[i], "Argument " + std::to_string(i + 1) + " of '" + node.callee +
                             "' expects " + expected.type.toString() +
                             ", got " + actual.toString());
                }
                if (expected.byReference &&
                    !dynamic_cast<VariableExprNode*>(node.arguments[i].get())) {
                    addError(*node.arguments[i], "Argument " + std::to_string(i + 1) + " of '" + node.callee +
                             "' must be a variable");
                }
            }
            return;
        }
    }
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}
