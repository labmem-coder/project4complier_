#ifndef AST_H
#define AST_H

#include "token.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class ASTNodeKind {
    Program,
    Block,
    ConstDecl,
    VarDecl,
    SubprogramDecl,
    ParamDecl,
    CompoundStmt,
    AssignStmt,
    CallStmt,
    IfStmt,
    ForStmt,
    WhileStmt,
    CaseStmt,
    BreakStmt,
    ContinueStmt,
    ReadStmt,
    WriteStmt,
    EmptyStmt,
    VariableExpr,
    LiteralExpr,
    UnaryExpr,
    BinaryExpr,
    CallExpr,
    Placeholder
};

inline std::string astKindToString(ASTNodeKind kind) {
    switch (kind) {
        case ASTNodeKind::Program: return "Program";
        case ASTNodeKind::Block: return "Block";
        case ASTNodeKind::ConstDecl: return "ConstDecl";
        case ASTNodeKind::VarDecl: return "VarDecl";
        case ASTNodeKind::SubprogramDecl: return "SubprogramDecl";
        case ASTNodeKind::ParamDecl: return "ParamDecl";
        case ASTNodeKind::CompoundStmt: return "CompoundStmt";
        case ASTNodeKind::AssignStmt: return "AssignStmt";
        case ASTNodeKind::CallStmt: return "CallStmt";
        case ASTNodeKind::IfStmt: return "IfStmt";
        case ASTNodeKind::ForStmt: return "ForStmt";
        case ASTNodeKind::WhileStmt: return "WhileStmt";
        case ASTNodeKind::CaseStmt: return "CaseStmt";
        case ASTNodeKind::BreakStmt: return "BreakStmt";
        case ASTNodeKind::ContinueStmt: return "ContinueStmt";
        case ASTNodeKind::ReadStmt: return "ReadStmt";
        case ASTNodeKind::WriteStmt: return "WriteStmt";
        case ASTNodeKind::EmptyStmt: return "EmptyStmt";
        case ASTNodeKind::VariableExpr: return "VariableExpr";
        case ASTNodeKind::LiteralExpr: return "LiteralExpr";
        case ASTNodeKind::UnaryExpr: return "UnaryExpr";
        case ASTNodeKind::BinaryExpr: return "BinaryExpr";
        case ASTNodeKind::CallExpr: return "CallExpr";
        case ASTNodeKind::Placeholder: return "Placeholder";
    }
    return "Unknown";
}

inline std::string indentString(int indent) {
    return std::string(indent, ' ');
}

// Forward declaration for visitor pattern (used by semantic analysis / codegen)
class ASTVisitor;

struct ASTNode {
    ASTNodeKind kind;
    int line = 0;
    int column = 0;
    explicit ASTNode(ASTNodeKind kind) : kind(kind) {}
    virtual ~ASTNode() = default;
    virtual void print(std::ostream& os, int indent = 0) const = 0;
    virtual void accept(ASTVisitor& visitor) = 0;
    void setLocation(int lineValue, int columnValue) {
        line = lineValue;
        column = columnValue;
    }
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

struct DeclNode : ASTNode {
    explicit DeclNode(ASTNodeKind kind) : ASTNode(kind) {}
};
using DeclNodePtr = std::shared_ptr<DeclNode>;
using DeclNodeList = std::vector<DeclNodePtr>;

struct StmtNode : ASTNode {
    explicit StmtNode(ASTNodeKind kind) : ASTNode(kind) {}
};
using StmtNodePtr = std::shared_ptr<StmtNode>;
using StmtNodeList = std::vector<StmtNodePtr>;

struct ExprNode : ASTNode {
    explicit ExprNode(ASTNodeKind kind) : ASTNode(kind) {}
};
using ExprNodePtr = std::shared_ptr<ExprNode>;
using ExprNodeList = std::vector<ExprNodePtr>;

struct PlaceholderNode : ASTNode {
    std::string label;
    explicit PlaceholderNode(std::string label)
        : ASTNode(ASTNodeKind::Placeholder), label(std::move(label)) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "Placeholder(" << label << ")\n";
    }
    void accept(ASTVisitor& visitor) override;
};

struct BlockNode : ASTNode {
    DeclNodeList constDecls;
    DeclNodeList varDecls;
    DeclNodeList subprogramDecls;
    StmtNodePtr compoundStmt;

    BlockNode() : ASTNode(ASTNodeKind::Block) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "Block\n";

        os << indentString(indent + 2) << "ConstDecls[" << constDecls.size() << "]\n";
        for (const auto& decl : constDecls) {
            if (decl) decl->print(os, indent + 4);
        }

        os << indentString(indent + 2) << "VarDecls[" << varDecls.size() << "]\n";
        for (const auto& decl : varDecls) {
            if (decl) decl->print(os, indent + 4);
        }

        os << indentString(indent + 2) << "SubprogramDecls[" << subprogramDecls.size() << "]\n";
        for (const auto& decl : subprogramDecls) {
            if (decl) decl->print(os, indent + 4);
        }

        os << indentString(indent + 2) << "Body\n";
        if (compoundStmt) {
            compoundStmt->print(os, indent + 4);
        } else {
            os << indentString(indent + 4) << "null\n";
        }
    }
};
using BlockNodePtr = std::shared_ptr<BlockNode>;

struct ProgramNode : ASTNode {
    std::string name;
    std::vector<std::string> parameters;
    BlockNodePtr block;

    ProgramNode() : ASTNode(ASTNodeKind::Program) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "Program(name=" << name << ")\n";
        os << indentString(indent + 2) << "Parameters";
        if (parameters.empty()) {
            os << ": []\n";
        } else {
            os << ":\n";
            for (const auto& param : parameters) {
                os << indentString(indent + 4) << param << "\n";
            }
        }
        os << indentString(indent + 2) << "Block\n";
        if (block) {
            block->print(os, indent + 4);
        } else {
            os << indentString(indent + 4) << "null\n";
        }
    }
};
using ProgramNodePtr = std::shared_ptr<ProgramNode>;

struct ConstDeclNode : DeclNode {
    std::string name;
    ExprNodePtr value;

    ConstDeclNode() : DeclNode(ASTNodeKind::ConstDecl) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ConstDecl(name=" << name << ")\n";
        os << indentString(indent + 2) << "Value\n";
        if (value) value->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct VarDeclNode : DeclNode {
    std::vector<std::string> names;
    std::string typeName;

    VarDeclNode() : DeclNode(ASTNodeKind::VarDecl) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "VarDecl(type=" << typeName << ")\n";
        for (const auto& name : names) {
            os << indentString(indent + 2) << name << "\n";
        }
    }
};

struct SubprogramDeclNode : DeclNode {
    std::string headerKind;
    std::string name;
    DeclNodeList parameters;
    std::string returnType;
    BlockNodePtr body;

    SubprogramDeclNode() : DeclNode(ASTNodeKind::SubprogramDecl) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "SubprogramDecl(kind=" << headerKind
           << ", name=" << name;
        if (!returnType.empty()) os << ", returnType=" << returnType;
        os << ")\n";

        os << indentString(indent + 2) << "Parameters[" << parameters.size() << "]\n";
        for (const auto& param : parameters) {
            if (param) param->print(os, indent + 4);
        }

        os << indentString(indent + 2) << "Body\n";
        if (body) body->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct ParamDeclNode : DeclNode {
    bool byReference = false;
    std::vector<std::string> names;
    std::string typeName;

    ParamDeclNode() : DeclNode(ASTNodeKind::ParamDecl) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ParamDecl(mode="
           << (byReference ? "var" : "value")
           << ", type=" << typeName << ")\n";
        for (const auto& name : names) {
            os << indentString(indent + 2) << name << "\n";
        }
    }
};

struct CompoundStmtNode : StmtNode {
    std::vector<StmtNodePtr> statements;

    CompoundStmtNode() : StmtNode(ASTNodeKind::CompoundStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "CompoundStmt[" << statements.size() << "]\n";
        for (const auto& stmt : statements) {
            if (stmt) stmt->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

struct CaseBranchNode {
    ExprNodeList labels;
    StmtNodePtr statement;

    void print(std::ostream& os, int indent = 0) const {
        os << indentString(indent) << "CaseBranch\n";
        os << indentString(indent + 2) << "Labels[" << labels.size() << "]\n";
        for (const auto& label : labels) {
            if (label) label->print(os, indent + 4);
            else os << indentString(indent + 4) << "null\n";
        }
        os << indentString(indent + 2) << "Statement\n";
        if (statement) statement->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct CaseStmtNode : StmtNode {
    ExprNodePtr expression;
    std::vector<CaseBranchNode> branches;

    CaseStmtNode() : StmtNode(ASTNodeKind::CaseStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "CaseStmt\n";
        os << indentString(indent + 2) << "Expression\n";
        if (expression) expression->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Branches[" << branches.size() << "]\n";
        for (const auto& branch : branches) {
            branch.print(os, indent + 4);
        }
    }
};

struct BreakStmtNode : StmtNode {
    BreakStmtNode() : StmtNode(ASTNodeKind::BreakStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "BreakStmt\n";
    }
};

struct ContinueStmtNode : StmtNode {
    ContinueStmtNode() : StmtNode(ASTNodeKind::ContinueStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ContinueStmt\n";
    }
};

struct EmptyStmtNode : StmtNode {
    EmptyStmtNode() : StmtNode(ASTNodeKind::EmptyStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "EmptyStmt\n";
    }
};

struct AssignStmtNode : StmtNode {
    ExprNodePtr target;
    ExprNodePtr value;

    AssignStmtNode() : StmtNode(ASTNodeKind::AssignStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "AssignStmt\n";
        os << indentString(indent + 2) << "Target\n";
        if (target) target->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Value\n";
        if (value) value->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct CallStmtNode : StmtNode {
    std::string callee;
    std::vector<ExprNodePtr> arguments;

    CallStmtNode() : StmtNode(ASTNodeKind::CallStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "CallStmt(name=" << callee << ")\n";
        for (const auto& arg : arguments) {
            if (arg) arg->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

struct IfStmtNode : StmtNode {
    ExprNodePtr condition;
    StmtNodePtr thenStmt;
    StmtNodePtr elseStmt;

    IfStmtNode() : StmtNode(ASTNodeKind::IfStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "IfStmt\n";
        os << indentString(indent + 2) << "Condition\n";
        if (condition) condition->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Then\n";
        if (thenStmt) thenStmt->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Else\n";
        if (elseStmt) elseStmt->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct ForStmtNode : StmtNode {
    std::string iterator;
    bool descending = false;
    ExprNodePtr startExpr;
    ExprNodePtr endExpr;
    StmtNodePtr body;

    ForStmtNode() : StmtNode(ASTNodeKind::ForStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ForStmt(iterator=" << iterator
           << ", direction=" << (descending ? "downto" : "to") << ")\n";
        os << indentString(indent + 2) << "Start\n";
        if (startExpr) startExpr->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "End\n";
        if (endExpr) endExpr->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Body\n";
        if (body) body->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct WhileStmtNode : StmtNode {
    ExprNodePtr condition;
    StmtNodePtr body;

    WhileStmtNode() : StmtNode(ASTNodeKind::WhileStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "WhileStmt\n";
        os << indentString(indent + 2) << "Condition\n";
        if (condition) condition->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Body\n";
        if (body) body->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct ReadStmtNode : StmtNode {
    std::vector<ExprNodePtr> variables;

    ReadStmtNode() : StmtNode(ASTNodeKind::ReadStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ReadStmt\n";
        for (const auto& var : variables) {
            if (var) var->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

struct WriteStmtNode : StmtNode {
    std::vector<ExprNodePtr> expressions;

    WriteStmtNode() : StmtNode(ASTNodeKind::WriteStmt) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "WriteStmt\n";
        for (const auto& expr : expressions) {
            if (expr) expr->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

struct VariableExprNode : ExprNode {
    std::string name;
    std::vector<ExprNodePtr> indices;
    std::vector<std::string> fields;

    VariableExprNode() : ExprNode(ASTNodeKind::VariableExpr) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "VariableExpr(name=" << name << ")\n";
        for (const auto& index : indices) {
            os << indentString(indent + 2) << "Index\n";
            if (index) index->print(os, indent + 4);
            else os << indentString(indent + 4) << "null\n";
        }
        for (const auto& field : fields) {
            os << indentString(indent + 2) << "Field(" << field << ")\n";
        }
    }
};

struct LiteralExprNode : ExprNode {
    std::string value;
    std::string literalType;

    LiteralExprNode() : ExprNode(ASTNodeKind::LiteralExpr) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "LiteralExpr(type=" << literalType
           << ", value=" << value << ")\n";
    }
};

struct UnaryExprNode : ExprNode {
    std::string op;
    ExprNodePtr operand;

    UnaryExprNode() : ExprNode(ASTNodeKind::UnaryExpr) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "UnaryExpr(op=" << op << ")\n";
        if (operand) operand->print(os, indent + 2);
        else os << indentString(indent + 2) << "null\n";
    }
};

struct BinaryExprNode : ExprNode {
    std::string op;
    ExprNodePtr left;
    ExprNodePtr right;

    BinaryExprNode() : ExprNode(ASTNodeKind::BinaryExpr) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "BinaryExpr(op=" << op << ")\n";
        os << indentString(indent + 2) << "Left\n";
        if (left) left->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
        os << indentString(indent + 2) << "Right\n";
        if (right) right->print(os, indent + 4);
        else os << indentString(indent + 4) << "null\n";
    }
};

struct CallExprNode : ExprNode {
    std::string callee;
    std::vector<ExprNodePtr> arguments;

    CallExprNode() : ExprNode(ASTNodeKind::CallExpr) {}
    void accept(ASTVisitor& visitor) override;

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "CallExpr(name=" << callee << ")\n";
        for (const auto& arg : arguments) {
            if (arg) arg->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

inline void printAST(const ASTNodePtr& node, std::ostream& os, int indent = 0) {
    if (!node) {
        os << indentString(indent) << "null\n";
        return;
    }
    node->print(os, indent);
}

// ---------------------------------------------------------------------------
// ASTVisitor — override the visit methods you need.
// Extend this for semantic analysis, type checking, C code generation, etc.
// ---------------------------------------------------------------------------
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visitProgram(ProgramNode& /*node*/) {}
    virtual void visitBlock(BlockNode& /*node*/) {}
    virtual void visitConstDecl(ConstDeclNode& /*node*/) {}
    virtual void visitVarDecl(VarDeclNode& /*node*/) {}
    virtual void visitSubprogramDecl(SubprogramDeclNode& /*node*/) {}
    virtual void visitParamDecl(ParamDeclNode& /*node*/) {}
    virtual void visitCompoundStmt(CompoundStmtNode& /*node*/) {}
    virtual void visitAssignStmt(AssignStmtNode& /*node*/) {}
    virtual void visitCallStmt(CallStmtNode& /*node*/) {}
    virtual void visitIfStmt(IfStmtNode& /*node*/) {}
    virtual void visitForStmt(ForStmtNode& /*node*/) {}
    virtual void visitWhileStmt(WhileStmtNode& /*node*/) {}
    virtual void visitCaseStmt(CaseStmtNode& /*node*/) {}
    virtual void visitBreakStmt(BreakStmtNode& /*node*/) {}
    virtual void visitContinueStmt(ContinueStmtNode& /*node*/) {}
    virtual void visitReadStmt(ReadStmtNode& /*node*/) {}
    virtual void visitWriteStmt(WriteStmtNode& /*node*/) {}
    virtual void visitEmptyStmt(EmptyStmtNode& /*node*/) {}
    virtual void visitVariableExpr(VariableExprNode& /*node*/) {}
    virtual void visitLiteralExpr(LiteralExprNode& /*node*/) {}
    virtual void visitUnaryExpr(UnaryExprNode& /*node*/) {}
    virtual void visitBinaryExpr(BinaryExprNode& /*node*/) {}
    virtual void visitCallExpr(CallExprNode& /*node*/) {}
    virtual void visitPlaceholder(PlaceholderNode& /*node*/) {}
};

// accept() implementations
inline void PlaceholderNode::accept(ASTVisitor& v)    { v.visitPlaceholder(*this); }
inline void BlockNode::accept(ASTVisitor& v)          { v.visitBlock(*this); }
inline void ProgramNode::accept(ASTVisitor& v)        { v.visitProgram(*this); }
inline void ConstDeclNode::accept(ASTVisitor& v)      { v.visitConstDecl(*this); }
inline void VarDeclNode::accept(ASTVisitor& v)        { v.visitVarDecl(*this); }
inline void SubprogramDeclNode::accept(ASTVisitor& v) { v.visitSubprogramDecl(*this); }
inline void ParamDeclNode::accept(ASTVisitor& v)      { v.visitParamDecl(*this); }
inline void CompoundStmtNode::accept(ASTVisitor& v)   { v.visitCompoundStmt(*this); }
inline void EmptyStmtNode::accept(ASTVisitor& v)      { v.visitEmptyStmt(*this); }
inline void AssignStmtNode::accept(ASTVisitor& v)     { v.visitAssignStmt(*this); }
inline void CallStmtNode::accept(ASTVisitor& v)       { v.visitCallStmt(*this); }
inline void IfStmtNode::accept(ASTVisitor& v)         { v.visitIfStmt(*this); }
inline void ForStmtNode::accept(ASTVisitor& v)        { v.visitForStmt(*this); }
inline void WhileStmtNode::accept(ASTVisitor& v)      { v.visitWhileStmt(*this); }
inline void CaseStmtNode::accept(ASTVisitor& v)       { v.visitCaseStmt(*this); }
inline void BreakStmtNode::accept(ASTVisitor& v)      { v.visitBreakStmt(*this); }
inline void ContinueStmtNode::accept(ASTVisitor& v)   { v.visitContinueStmt(*this); }
inline void ReadStmtNode::accept(ASTVisitor& v)       { v.visitReadStmt(*this); }
inline void WriteStmtNode::accept(ASTVisitor& v)      { v.visitWriteStmt(*this); }
inline void VariableExprNode::accept(ASTVisitor& v)   { v.visitVariableExpr(*this); }
inline void LiteralExprNode::accept(ASTVisitor& v)    { v.visitLiteralExpr(*this); }
inline void UnaryExprNode::accept(ASTVisitor& v)      { v.visitUnaryExpr(*this); }
inline void BinaryExprNode::accept(ASTVisitor& v)     { v.visitBinaryExpr(*this); }
inline void CallExprNode::accept(ASTVisitor& v)       { v.visitCallExpr(*this); }

#endif
