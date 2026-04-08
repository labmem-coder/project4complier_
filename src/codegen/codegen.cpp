#include "codegen.h"
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void CCodeGenerator::emitIndent() {
    for (int i = 0; i < indent_; ++i) out_ << "    ";
}

void CCodeGenerator::emit(const std::string& s) {
    out_ << s;
}

void CCodeGenerator::emitLine(const std::string& s) {
    emitIndent();
    out_ << s << "\n";
}

std::string CCodeGenerator::pascalTypeToCType(const std::string& pascalType) const {
    if (pascalType == "integer") return "int";
    if (pascalType == "real")    return "double";
    if (pascalType == "boolean") return "int";
    if (pascalType == "char")    return "char";
    if (pascalType == "string")  return "const char*";
    return "int";  // fallback
}

std::string CCodeGenerator::formatSpecifier(const TypeInfo& ti, bool forScanf) const {
    if (ti.isArray()) {
        // Shouldn't happen for format specifiers, but handle element type
        return formatSpecifier(TypeInfo::makeSimple(ti.elementType), forScanf);
    }
    if (ti.baseType == "integer" || ti.baseType == "boolean") return "%d";
    if (ti.baseType == "real")    return forScanf ? "%lf" : "%f";
    if (ti.baseType == "char")    return "%c";
    if (ti.baseType == "string")  return "%s";
    return "%d";
}

// ---------------------------------------------------------------------------
// Type inference (mirrors SemanticAnalyzer::inferType)
// ---------------------------------------------------------------------------
TypeInfo CCodeGenerator::inferType(ExprNode* expr) {
    if (!expr) return TypeInfo::makeSimple("void");

    switch (expr->kind) {
        case ASTNodeKind::VariableExpr: {
            auto* ve = static_cast<VariableExprNode*>(expr);
            Symbol* sym = symTable_.lookup(ve->name);
            if (!sym) return TypeInfo::makeSimple("integer");
            if (sym->kind == SymbolKind::Function && ve->indices.empty()) {
                return TypeInfo::makeSimple(sym->returnType);
            }
            if (sym->type.isArray() && !ve->indices.empty())
                return TypeInfo::makeSimple(sym->type.elementType);
            return sym->type;
        }
        case ASTNodeKind::LiteralExpr: {
            auto* le = static_cast<LiteralExprNode*>(expr);
            if (le->literalType == "integer" || le->literalType == "num") {
                if (le->value.find('.') != std::string::npos)
                    return TypeInfo::makeSimple("real");
                return TypeInfo::makeSimple("integer");
            }
            if (le->literalType == "real") return TypeInfo::makeSimple("real");
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
            if (be->op == "=" || be->op == "<>" || be->op == "<" ||
                be->op == "<=" || be->op == ">" || be->op == ">=")
                return TypeInfo::makeSimple("boolean");
            if (be->op == "and" || be->op == "or")
                return TypeInfo::makeSimple("boolean");
            if (be->op == "/") return TypeInfo::makeSimple("real");
            if (be->op == "div" || be->op == "mod")
                return TypeInfo::makeSimple("integer");
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
            return TypeInfo::makeSimple("integer");
    }
}

// ---------------------------------------------------------------------------
// Expression emission
// ---------------------------------------------------------------------------
void CCodeGenerator::emitVarAccess(VariableExprNode& node) {
    emit(node.name);
    if (!node.indices.empty()) {
        Symbol* sym = symTable_.lookup(node.name);
        for (auto& idx : node.indices) {
            emit("[");
            emitExpr(idx.get());
            // Subtract array lower bound for offset
            if (sym && sym->type.isArray() && sym->type.arrayLow != 0) {
                emit(" - " + std::to_string(sym->type.arrayLow));
            }
            emit("]");
        }
    }
}

void CCodeGenerator::emitExpr(ExprNode* expr) {
    if (!expr) { emit("0"); return; }
    bool savedInsideExpr = insideExpr_;
    insideExpr_ = true;
    expr->accept(*this);
    insideExpr_ = savedInsideExpr;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
std::string CCodeGenerator::generate(ProgramNodePtr ast) {
    out_.str("");
    out_.clear();
    indent_ = 0;
    if (ast) ast->accept(*this);
    return out_.str();
}

// ---------------------------------------------------------------------------
// Visitor implementations
// ---------------------------------------------------------------------------
void CCodeGenerator::visitProgram(ProgramNode& node) {
    emitLine("#include <stdio.h>");
    emit("\n");

    symTable_.enterScope();

    // Register program params
    for (auto& p : node.parameters) {
        Symbol s; s.name = p; s.kind = SymbolKind::Variable;
        s.type = TypeInfo::makeSimple("integer");
        symTable_.declare(s);
    }

    if (node.block) {
        // Emit global declarations (const, var, subprograms) before main
        for (auto& d : node.block->constDecls)      if (d) d->accept(*this);
        for (auto& d : node.block->varDecls)        if (d) d->accept(*this);
        for (auto& d : node.block->subprogramDecls) if (d) d->accept(*this);

        emit("\n");
        emitLine("int main() {");
        indent_++;

        if (node.block->compoundStmt) {
            // Emit the statements inside compound_statement (skip the { } wrapper)
            auto* cs = dynamic_cast<CompoundStmtNode*>(node.block->compoundStmt.get());
            if (cs) {
                for (auto& stmt : cs->statements)
                    if (stmt) stmt->accept(*this);
            } else {
                node.block->compoundStmt->accept(*this);
            }
        }

        emitLine("return 0;");
        indent_--;
        emitLine("}");
    } else {
        emit("\n");
        emitLine("int main() {");
        indent_++;
        emitLine("return 0;");
        indent_--;
        emitLine("}");
    }

    symTable_.exitScope();
}

void CCodeGenerator::visitBlock(BlockNode& node) {
    // Block inside a subprogram — emit local declarations + compound stmt
    for (auto& d : node.constDecls) if (d) d->accept(*this);
    for (auto& d : node.varDecls)   if (d) d->accept(*this);
    if (node.compoundStmt) {
        auto* cs = dynamic_cast<CompoundStmtNode*>(node.compoundStmt.get());
        if (cs) {
            for (auto& stmt : cs->statements)
                if (stmt) stmt->accept(*this);
        } else {
            node.compoundStmt->accept(*this);
        }
    }
}

void CCodeGenerator::visitConstDecl(ConstDeclNode& node) {
    // Infer type from value
    TypeInfo ti = node.value ? inferType(node.value.get()) : TypeInfo::makeSimple("integer");
    std::string cType = pascalTypeToCType(ti.baseType);

    Symbol sym;
    sym.name = node.name; sym.kind = SymbolKind::Constant; sym.type = ti;
    if (auto* lit = dynamic_cast<LiteralExprNode*>(node.value.get()))
        sym.constValue = lit->value;
    symTable_.declare(sym);

    emitIndent();
    emit("const " + cType + " " + node.name + " = ");
    if (node.value) emitExpr(node.value.get());
    else emit("0");
    emit(";\n");
}

void CCodeGenerator::visitVarDecl(VarDeclNode& node) {
    TypeInfo ti = TypeInfo::fromString(node.typeName);

    // Register in symbol table
    for (auto& name : node.names) {
        Symbol sym;
        sym.name = name; sym.kind = SymbolKind::Variable; sym.type = ti;
        symTable_.declare(sym);
    }

    if (ti.isArray()) {
        std::string cElemType = pascalTypeToCType(ti.elementType);
        int size = ti.arrayHigh - ti.arrayLow + 1;
        for (auto& name : node.names) {
            emitLine(cElemType + " " + name + "[" + std::to_string(size) + "];");
        }
    } else {
        std::string cType = pascalTypeToCType(ti.baseType);
        if (ti.baseType == "string") {
            for (auto& name : node.names) {
                emitLine(cType + " " + name + ";");
            }
        } else {
            emitIndent();
            emit(cType + " ");
            for (size_t i = 0; i < node.names.size(); ++i) {
                if (i > 0) emit(", ");
                emit(node.names[i]);
            }
            emit(";\n");
        }
    }
}

void CCodeGenerator::visitSubprogramDecl(SubprogramDeclNode& node) {
    // Register in outer scope
    Symbol sym;
    sym.name = node.name;
    sym.kind = (node.headerKind == "function") ? SymbolKind::Function : SymbolKind::Procedure;
    sym.returnType = node.returnType;

    for (auto& p : node.parameters) {
        auto* pd = dynamic_cast<ParamDeclNode*>(p.get());
        if (pd) {
            TypeInfo pType = TypeInfo::fromString(pd->typeName);
            for (auto& pname : pd->names)
                sym.params.push_back({pname, pType, pd->byReference});
        }
    }
    symTable_.declare(sym);

    // Emit function signature
    std::string retType = (sym.kind == SymbolKind::Function)
                          ? pascalTypeToCType(node.returnType)
                          : "void";

    emitIndent();
    emit(retType + " " + node.name + "(");

    // Parameters
    bool first = true;
    for (auto& p : node.parameters) {
        auto* pd = dynamic_cast<ParamDeclNode*>(p.get());
        if (!pd) continue;
        std::string cType = pascalTypeToCType(pd->typeName);
        for (auto& pname : pd->names) {
            if (!first) emit(", ");
            if (pd->byReference)
                emit(cType + " *" + pname);
            else
                emit(cType + " " + pname);
            first = false;
        }
    }
    emit(") {\n");
    indent_++;

    // Enter subprogram scope
    symTable_.enterScope();
    currentFunction_ = (sym.kind == SymbolKind::Function) ? node.name : "";

    // Declare parameters in scope
    for (auto& p : node.parameters) {
        auto* pd = dynamic_cast<ParamDeclNode*>(p.get());
        if (!pd) continue;
        TypeInfo pType = TypeInfo::fromString(pd->typeName);
        for (auto& pname : pd->names) {
            Symbol paramSym;
            paramSym.name = pname; paramSym.kind = SymbolKind::Parameter;
            paramSym.type = pType; paramSym.byReference = pd->byReference;
            symTable_.declare(paramSym);
        }
    }

    // For functions, declare a local return-value variable
    if (sym.kind == SymbolKind::Function) {
        std::string cRetType = pascalTypeToCType(node.returnType);
        emitLine(cRetType + " " + node.name + "_result;");
        Symbol retSym;
        retSym.name = node.name; retSym.kind = SymbolKind::Variable;
        retSym.type = TypeInfo::makeSimple(node.returnType);
        symTable_.declare(retSym);
    }

    // Body
    if (node.body) node.body->accept(*this);

    // For functions, emit return statement
    if (sym.kind == SymbolKind::Function) {
        emitLine("return " + node.name + "_result;");
    }

    currentFunction_ = "";
    symTable_.exitScope();

    indent_--;
    emitLine("}");
    emit("\n");
}

void CCodeGenerator::visitParamDecl(ParamDeclNode& /*node*/) {
    // Handled inside visitSubprogramDecl
}

void CCodeGenerator::visitCompoundStmt(CompoundStmtNode& node) {
    emitLine("{");
    indent_++;
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    indent_--;
    emitLine("}");
}

void CCodeGenerator::visitAssignStmt(AssignStmtNode& node) {
    emitIndent();

    auto* target = dynamic_cast<VariableExprNode*>(node.target.get());
    if (target) {
        // Check if this is a function return-value assignment (funcName := expr)
        if (!currentFunction_.empty() && target->name == currentFunction_ && target->indices.empty()) {
            emit(currentFunction_ + "_result = ");
            if (node.value) emitExpr(node.value.get());
            emit(";\n");
            return;
        }
        emitVarAccess(*target);
    } else if (node.target) {
        emitExpr(node.target.get());
    }

    emit(" = ");
    if (node.value) emitExpr(node.value.get());
    emit(";\n");
}

void CCodeGenerator::visitCallStmt(CallStmtNode& node) {
    std::string lower = node.callee;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (lower == "write" || lower == "writeln") {
        // printf with format specifiers
        emitIndent();
        emit("printf(\"");
        // Build format string
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            if (i > 0) emit(" ");
            TypeInfo ti = node.arguments[i] ? inferType(node.arguments[i].get())
                                            : TypeInfo::makeSimple("integer");
            emit(formatSpecifier(ti));
        }
        if (lower == "writeln") emit("\\n");
        emit("\"");
        for (auto& arg : node.arguments) {
            emit(", ");
            if (arg) emitExpr(arg.get());
        }
        emit(");\n");
        return;
    }

    if (lower == "read" || lower == "readln") {
        // scanf with format specifiers
        emitIndent();
        emit("scanf(\"");
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            TypeInfo ti = node.arguments[i] ? inferType(node.arguments[i].get())
                                            : TypeInfo::makeSimple("integer");
            emit(formatSpecifier(ti, true));
        }
        emit("\"");
        for (auto& arg : node.arguments) {
            emit(", &");
            if (arg) emitExpr(arg.get());
        }
        emit(");\n");
        return;
    }

    // Regular procedure call
    emitIndent();
    emit(node.callee + "(");
    Symbol* sym = symTable_.lookup(node.callee);
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        if (i > 0) emit(", ");
        // If parameter is by-reference, pass address
        if (sym && i < sym->params.size() && sym->params[i].byReference) {
            emit("&");
        }
        if (node.arguments[i]) emitExpr(node.arguments[i].get());
    }
    emit(");\n");
}

void CCodeGenerator::visitIfStmt(IfStmtNode& node) {
    emitIndent();
    emit("if (");
    if (node.condition) emitExpr(node.condition.get());
    emit(") ");

    // Then branch
    if (node.thenStmt) {
        auto* cs = dynamic_cast<CompoundStmtNode*>(node.thenStmt.get());
        if (cs) {
            emit("{\n");
            indent_++;
            for (auto& s : cs->statements) if (s) s->accept(*this);
            indent_--;
            emitIndent();
            emit("}");
        } else {
            emit("{\n");
            indent_++;
            node.thenStmt->accept(*this);
            indent_--;
            emitIndent();
            emit("}");
        }
    }

    // Else branch
    if (node.elseStmt) {
        emit(" else ");
        auto* cs = dynamic_cast<CompoundStmtNode*>(node.elseStmt.get());
        if (cs) {
            emit("{\n");
            indent_++;
            for (auto& s : cs->statements) if (s) s->accept(*this);
            indent_--;
            emitIndent();
            emit("}");
        } else {
            // Check for else-if chain
            auto* elseIf = dynamic_cast<IfStmtNode*>(node.elseStmt.get());
            if (elseIf) {
                emit(""); // visitIfStmt will emit "if (...)"
                node.elseStmt->accept(*this);
                return;
            }
            emit("{\n");
            indent_++;
            node.elseStmt->accept(*this);
            indent_--;
            emitIndent();
            emit("}");
        }
    }
    emit("\n");
}

void CCodeGenerator::visitForStmt(ForStmtNode& node) {
    emitIndent();
    emit("for (" + node.iterator + " = ");
    if (node.startExpr) emitExpr(node.startExpr.get());
    emit("; " + node.iterator + " <= ");
    if (node.endExpr) emitExpr(node.endExpr.get());
    emit("; " + node.iterator + "++) ");

    if (node.body) {
        auto* cs = dynamic_cast<CompoundStmtNode*>(node.body.get());
        if (cs) {
            emit("{\n");
            indent_++;
            for (auto& s : cs->statements) if (s) s->accept(*this);
            indent_--;
            emitLine("}");
        } else {
            emit("{\n");
            indent_++;
            node.body->accept(*this);
            indent_--;
            emitLine("}");
        }
    } else {
        emit("{ }\n");
    }
}

void CCodeGenerator::visitWhileStmt(WhileStmtNode& node) {
    emitIndent();
    emit("while (");
    if (node.condition) emitExpr(node.condition.get());
    emit(") ");

    if (node.body) {
        auto* cs = dynamic_cast<CompoundStmtNode*>(node.body.get());
        if (cs) {
            emit("{\n");
            indent_++;
            for (auto& s : cs->statements) if (s) s->accept(*this);
            indent_--;
            emitLine("}");
        } else {
            emit("{\n");
            indent_++;
            node.body->accept(*this);
            indent_--;
            emitLine("}");
        }
    } else {
        emit("{ }\n");
    }
}

void CCodeGenerator::visitCaseStmt(CaseStmtNode& node) {
    emitIndent();
    emit("switch (");
    if (node.expression) emitExpr(node.expression.get());
    emit(") {\n");
    indent_++;
    for (auto& branch : node.branches) {
        for (auto& label : branch.labels) {
            emitIndent();
            emit("case ");
            if (label) emitExpr(label.get());
            emit(":\n");
        }
        indent_++;
        if (branch.statement) branch.statement->accept(*this);
        emitLine("break;");
        indent_--;
    }
    indent_--;
    emitLine("}");
}

void CCodeGenerator::visitBreakStmt(BreakStmtNode& /*node*/) {
    emitLine("break;");
}

void CCodeGenerator::visitContinueStmt(ContinueStmtNode& /*node*/) {
    emitLine("continue;");
}

void CCodeGenerator::visitReadStmt(ReadStmtNode& node) {
    emitIndent();
    emit("scanf(\"");
    for (size_t i = 0; i < node.variables.size(); ++i) {
        TypeInfo ti = node.variables[i] ? inferType(node.variables[i].get())
                                        : TypeInfo::makeSimple("integer");
        emit(formatSpecifier(ti, true));
    }
    emit("\"");
    for (auto& v : node.variables) {
        emit(", &");
        if (v) emitExpr(v.get());
    }
    emit(");\n");
}

void CCodeGenerator::visitWriteStmt(WriteStmtNode& node) {
    emitIndent();
    emit("printf(\"");
    for (size_t i = 0; i < node.expressions.size(); ++i) {
        if (i > 0) emit(" ");
        TypeInfo ti = node.expressions[i] ? inferType(node.expressions[i].get())
                                          : TypeInfo::makeSimple("integer");
        emit(formatSpecifier(ti));
    }
    emit("\"");
    for (auto& e : node.expressions) {
        emit(", ");
        if (e) emitExpr(e.get());
    }
    emit(");\n");
}

void CCodeGenerator::visitEmptyStmt(EmptyStmtNode& /*node*/) {
    // No output for empty statements
}

// ---------------------------------------------------------------------------
// Expression visitors — emit inline expressions
// ---------------------------------------------------------------------------
void CCodeGenerator::visitVariableExpr(VariableExprNode& node) {
    // If inside a function and the variable is the function name, use _result
    if (!currentFunction_.empty() && node.name == currentFunction_ && node.indices.empty()) {
        emit(currentFunction_ + "_result");
        return;
    }

    Symbol* sym = symTable_.lookup(node.name);
    if (sym && sym->kind == SymbolKind::Function && node.indices.empty()) {
        emit(node.name + "()");
        return;
    }
    // If parameter passed by reference, dereference
    if (sym && sym->kind == SymbolKind::Parameter && sym->byReference) {
        if (node.indices.empty()) {
            emit("(*" + node.name + ")");
            return;
        }
    }
    emitVarAccess(node);
}

void CCodeGenerator::visitLiteralExpr(LiteralExprNode& node) {
    if (node.literalType == "string" && node.value.size() >= 2 &&
        node.value.front() == '\'' && node.value.back() == '\'') {
        std::string content = node.value.substr(1, node.value.size() - 2);
        std::string cString = "\"";
        for (char ch : content) {
            if (ch == '\\' || ch == '"') cString.push_back('\\');
            cString.push_back(ch);
        }
        cString.push_back('"');
        emit(cString);
        return;
    }
    emit(node.value);
}

void CCodeGenerator::visitUnaryExpr(UnaryExprNode& node) {
    if (node.op == "not") {
        emit("!");
    } else if (node.op == "-" || node.op == "minus") {
        emit("-");
    } else if (node.op == "+" || node.op == "plus") {
        emit("+");
    } else {
        emit(node.op);
    }
    emit("(");
    if (node.operand) emitExpr(node.operand.get());
    emit(")");
}

void CCodeGenerator::visitBinaryExpr(BinaryExprNode& node) {
    // Map Pascal operators to C operators
    std::string op = node.op;
    if (op == "and")     op = "&&";
    else if (op == "or") op = "||";
    else if (op == "div") op = "/";
    else if (op == "mod") op = "%";
    else if (op == "=")  op = "==";
    else if (op == "<>") op = "!=";
    // <, <=, >, >= and +, -, *, / stay the same

    emit("(");
    if (node.left) emitExpr(node.left.get());
    emit(" " + op + " ");
    if (node.right) emitExpr(node.right.get());
    emit(")");
}

void CCodeGenerator::visitCallExpr(CallExprNode& node) {
    emit(node.callee + "(");
    Symbol* sym = symTable_.lookup(node.callee);
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        if (i > 0) emit(", ");
        if (sym && i < sym->params.size() && sym->params[i].byReference)
            emit("&");
        if (node.arguments[i]) emitExpr(node.arguments[i].get());
    }
    emit(")");
}
