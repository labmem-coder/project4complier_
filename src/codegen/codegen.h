#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// CCodeGenerator — translates the validated AST into C source code.
//
// Design: implements ASTVisitor.  Traverses the AST produced by the parser,
// using its own SymbolTable (rebuilt during traversal) to resolve types for
// format specifiers, array-offset calculations, and function return values.
//
// Extensibility: override or add visit methods when new AST node kinds are
// introduced.  The generator is completely independent of the Parser and
// SemanticAnalyzer modules.
// ---------------------------------------------------------------------------
class CCodeGenerator : public ASTVisitor {
public:
    // Generate C code from the AST.  Returns the complete C source string.
    std::string generate(ProgramNodePtr ast);

    // ----- ASTVisitor overrides -----
    void visitProgram(ProgramNode& node) override;
    void visitBlock(BlockNode& node) override;
    void visitConstDecl(ConstDeclNode& node) override;
    void visitVarDecl(VarDeclNode& node) override;
    void visitSubprogramDecl(SubprogramDeclNode& node) override;
    void visitParamDecl(ParamDeclNode& node) override;
    void visitCompoundStmt(CompoundStmtNode& node) override;
    void visitAssignStmt(AssignStmtNode& node) override;
    void visitCallStmt(CallStmtNode& node) override;
    void visitIfStmt(IfStmtNode& node) override;
    void visitForStmt(ForStmtNode& node) override;
    void visitReadStmt(ReadStmtNode& node) override;
    void visitWriteStmt(WriteStmtNode& node) override;
    void visitEmptyStmt(EmptyStmtNode& node) override;
    void visitVariableExpr(VariableExprNode& node) override;
    void visitLiteralExpr(LiteralExprNode& node) override;
    void visitUnaryExpr(UnaryExprNode& node) override;
    void visitBinaryExpr(BinaryExprNode& node) override;
    void visitCallExpr(CallExprNode& node) override;

private:
    std::ostringstream out_;       // accumulated output
    int indent_ = 0;               // current indentation level
    SymbolTable symTable_;          // local type lookup (rebuilt during traversal)
    std::string currentFunction_;   // enclosing function name (for return-value pattern)
    bool insideExpr_ = false;       // true when emitting an expression (suppress ';')

    // Indentation helpers
    void emitIndent();
    void emit(const std::string& s);
    void emitLine(const std::string& s);

    // Type helpers
    std::string pascalTypeToCType(const std::string& pascalType) const;
    std::string formatSpecifier(const TypeInfo& ti, bool forScanf = false) const;
    TypeInfo inferType(ExprNode* expr);

    // Emit a variable/array access expression (handles array offset)
    void emitVarAccess(VariableExprNode& node);

    // Emit an expression into the output stream
    void emitExpr(ExprNode* expr);
};

#endif
