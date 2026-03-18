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

struct ASTNode {
    ASTNodeKind kind;
    explicit ASTNode(ASTNodeKind kind) : kind(kind) {}
    virtual ~ASTNode() = default;
    virtual void print(std::ostream& os, int indent = 0) const = 0;
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
};

struct BlockNode : ASTNode {
    DeclNodeList constDecls;
    DeclNodeList varDecls;
    DeclNodeList subprogramDecls;
    StmtNodePtr compoundStmt;

    BlockNode() : ASTNode(ASTNodeKind::Block) {}

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

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "CompoundStmt[" << statements.size() << "]\n";
        for (const auto& stmt : statements) {
            if (stmt) stmt->print(os, indent + 2);
            else os << indentString(indent + 2) << "null\n";
        }
    }
};

struct EmptyStmtNode : StmtNode {
    EmptyStmtNode() : StmtNode(ASTNodeKind::EmptyStmt) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "EmptyStmt\n";
    }
};

struct AssignStmtNode : StmtNode {
    ExprNodePtr target;
    ExprNodePtr value;

    AssignStmtNode() : StmtNode(ASTNodeKind::AssignStmt) {}

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
    ExprNodePtr startExpr;
    ExprNodePtr endExpr;
    StmtNodePtr body;

    ForStmtNode() : StmtNode(ASTNodeKind::ForStmt) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "ForStmt(iterator=" << iterator << ")\n";
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

struct ReadStmtNode : StmtNode {
    std::vector<ExprNodePtr> variables;

    ReadStmtNode() : StmtNode(ASTNodeKind::ReadStmt) {}

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

    VariableExprNode() : ExprNode(ASTNodeKind::VariableExpr) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "VariableExpr(name=" << name << ")\n";
        for (const auto& index : indices) {
            os << indentString(indent + 2) << "Index\n";
            if (index) index->print(os, indent + 4);
            else os << indentString(indent + 4) << "null\n";
        }
    }
};

struct LiteralExprNode : ExprNode {
    std::string value;
    std::string literalType;

    LiteralExprNode() : ExprNode(ASTNodeKind::LiteralExpr) {}

    void print(std::ostream& os, int indent = 0) const override {
        os << indentString(indent) << "LiteralExpr(type=" << literalType
           << ", value=" << value << ")\n";
    }
};

struct UnaryExprNode : ExprNode {
    std::string op;
    ExprNodePtr operand;

    UnaryExprNode() : ExprNode(ASTNodeKind::UnaryExpr) {}

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

#endif
