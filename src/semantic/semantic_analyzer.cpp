#include "semantic_analyzer.h"
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void SemanticAnalyzer::addError(const std::string& msg) {
    errors_.push_back({msg});
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
            if (sym->type.isArray()) {
                if (!ve->indices.empty())
                    return TypeInfo::makeSimple(sym->type.elementType); // indexed access
                return sym->type;  // whole array
            }
            return sym->type;
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
        addError("Duplicate constant declaration: '" + node.name + "'");
}

void SemanticAnalyzer::visitVarDecl(VarDeclNode& node) {
    TypeInfo ti = TypeInfo::fromString(node.typeName);
    for (auto& name : node.names) {
        Symbol sym;
        sym.name = name;
        sym.kind = SymbolKind::Variable;
        sym.type = ti;
        if (!symTable_.declare(sym))
            addError("Duplicate variable declaration: '" + name + "'");
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
        addError("Duplicate subprogram declaration: '" + node.name + "'");

    // Enter subprogram scope
    symTable_.enterScope();
    currentFunction_ = (sym.kind == SymbolKind::Function) ? node.name : "";

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
                    addError("Duplicate parameter name: '" + pname + "'");
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
        TypeInfo lhsType = inferType(node.target.get());
        TypeInfo rhsType = inferType(node.value.get());
        if (!typesCompatible(lhsType, rhsType)) {
            auto* ve = dynamic_cast<VariableExprNode*>(node.target.get());
            std::string name = ve ? ve->name : "<expr>";
            addError("Type mismatch in assignment to '" + name +
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
            addError("Undeclared procedure: '" + node.callee + "'");
        }
    } else if (sym->kind != SymbolKind::Procedure && sym->kind != SymbolKind::Function) {
        addError("'" + node.callee + "' is not a procedure");
    } else {
        // Check argument count
        if (node.arguments.size() != sym->params.size()) {
            addError("Procedure '" + node.callee + "' expects " +
                     std::to_string(sym->params.size()) + " argument(s), got " +
                     std::to_string(node.arguments.size()));
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
        addError("Undeclared for-loop iterator: '" + node.iterator + "'");
    } else if (!sym->type.isInteger()) {
        addError("For-loop iterator '" + node.iterator + "' must be integer, got " +
                 sym->type.toString());
    }
    if (node.startExpr) node.startExpr->accept(*this);
    if (node.endExpr)   node.endExpr->accept(*this);
    if (node.body)      node.body->accept(*this);
}

void SemanticAnalyzer::visitReadStmt(ReadStmtNode& node) {
    for (auto& v : node.variables) {
        if (v) {
            v->accept(*this);
            // read arguments should be variables
            auto* ve = dynamic_cast<VariableExprNode*>(v.get());
            if (!ve) addError("read() argument must be a variable");
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
        addError("Undeclared identifier: '" + node.name + "'");
    } else if (sym->type.isArray() && !node.indices.empty()) {
        // Check that indices are integer
        for (auto& idx : node.indices) {
            if (idx) {
                idx->accept(*this);
                TypeInfo idxType = inferType(idx.get());
                if (!idxType.isInteger())
                    addError("Array index for '" + node.name + "' must be integer");
            }
        }
    }
}

void SemanticAnalyzer::visitLiteralExpr(LiteralExprNode& /*node*/) {}

void SemanticAnalyzer::visitUnaryExpr(UnaryExprNode& node) {
    if (node.operand) node.operand->accept(*this);
}

void SemanticAnalyzer::visitBinaryExpr(BinaryExprNode& node) {
    if (node.left)  node.left->accept(*this);
    if (node.right) node.right->accept(*this);

    // Check operand types for integer-only operators
    if (node.op == "div" || node.op == "mod") {
        TypeInfo lt = inferType(node.left.get());
        TypeInfo rt = inferType(node.right.get());
        if (!lt.isInteger())
            addError("Left operand of '" + node.op + "' must be integer");
        if (!rt.isInteger())
            addError("Right operand of '" + node.op + "' must be integer");
    }
}

void SemanticAnalyzer::visitCallExpr(CallExprNode& node) {
    Symbol* sym = symTable_.lookup(node.callee);
    if (!sym) {
        addError("Undeclared function: '" + node.callee + "'");
    } else if (sym->kind != SymbolKind::Function) {
        addError("'" + node.callee + "' is not a function");
    } else {
        if (node.arguments.size() != sym->params.size()) {
            addError("Function '" + node.callee + "' expects " +
                     std::to_string(sym->params.size()) + " argument(s), got " +
                     std::to_string(node.arguments.size()));
        }
    }
    for (auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}
