#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "symbol_table.h"
#include "ast.h"
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// SemanticError — a single error found during semantic analysis
// ---------------------------------------------------------------------------
struct SemanticError {
    int line = 0;
    int column = 0;
    std::string message;
};

// ---------------------------------------------------------------------------
// SemanticAnalyzer — validates the AST built during parsing.
//
// Design: implements ASTVisitor so it can be invoked right after the parser
// constructs the AST.  It builds the symbol table, checks declarations,
// and performs basic type checking.  The SymbolTable it produces is then
// available for downstream phases (e.g. code generation).
//
// Extensibility: override visit methods or add new ones when the grammar
// and AST are extended with new node kinds.
// ---------------------------------------------------------------------------
class SemanticAnalyzer : public ASTVisitor {
public:
    // Run the analysis.  Returns true if no errors were found.
    bool analyze(ProgramNodePtr ast);

    const std::vector<SemanticError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

    // Expose the symbol table so the code generator can reuse type info.
    SymbolTable& getSymbolTable() { return symTable_; }

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
    void visitWhileStmt(WhileStmtNode& node) override;
    void visitCaseStmt(CaseStmtNode& node) override;
    void visitBreakStmt(BreakStmtNode& node) override;
    void visitContinueStmt(ContinueStmtNode& node) override;
    void visitReadStmt(ReadStmtNode& node) override;
    void visitWriteStmt(WriteStmtNode& node) override;
    void visitEmptyStmt(EmptyStmtNode& node) override;
    void visitVariableExpr(VariableExprNode& node) override;
    void visitLiteralExpr(LiteralExprNode& node) override;
    void visitUnaryExpr(UnaryExprNode& node) override;
    void visitBinaryExpr(BinaryExprNode& node) override;
    void visitCallExpr(CallExprNode& node) override;

private:
    SymbolTable symTable_;
    std::vector<SemanticError> errors_;
    std::string currentFunction_;   // name of the enclosing function (for return-value assignments)
    int loopDepth_ = 0;
    int caseDepth_ = 0;
    std::vector<bool> functionAssignedStack_;

    void addError(const std::string& msg);
    void addError(const ASTNode& node, const std::string& msg);
    void markCurrentFunctionAssigned();
    bool isCurrentFunctionResultName(const std::string& name) const;
    std::optional<std::string> constantValueKey(ExprNode* expr) const;

    // Type inference: determine the result type of an expression subtree.
    TypeInfo inferType(ExprNode* expr);

    // Check type compatibility for assignment (lhs := rhs).
    // Returns true if compatible (possibly with implicit promotion).
    bool typesCompatible(const TypeInfo& lhs, const TypeInfo& rhs) const;
};

#endif
