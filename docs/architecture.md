# Pascal-S 编译器架构说明与扩展指南

## 目录

1. [项目结构](#1-项目结构)
2. [整体架构](#2-整体架构)
3. [各模块详解](#3-各模块详解)
4. [问题一：如何扩写文法](#4-问题一如何扩写文法)
5. [问题二：如何进行后续设计（语义分析与 C 代码生成）](#5-问题二如何进行后续设计语义分析与-c-代码生成)

---

## 1. 项目结构

```
src/
├── lexer/
│   ├── token.h          # TokenType 枚举、Token 结构体、辅助函数
│   ├── lexer.h          # Lexer 类声明
│   └── lexer.cpp        # Lexer 实现
├── parser/
│   ├── grammar.h        # Grammar 类、Production 结构体
│   ├── grammar.cpp      # 文法定义 (buildPascalSGrammar)、LL(1) 变换、FIRST/FOLLOW、分析表
│   ├── ast.h            # AST 节点层次结构、ASTVisitor 基类
│   ├── parser.h         # Parser 类、SemanticValue、SemanticAction 类型定义
│   └── parser.cpp       # LL(1) 驱动、语义动作注册表 (registerDefaultActions)
├── semantic/
│   ├── symbol_table.h   # TypeInfo、Symbol、SymbolTable 定义
│   ├── symbol_table.cpp # SymbolTable 实现（作用域栈）
│   ├── semantic_analyzer.h   # SemanticAnalyzer 声明 (ASTVisitor)
│   └── semantic_analyzer.cpp # 语义检查实现（声明/类型/作用域）
├── codegen/
│   ├── codegen.h        # CCodeGenerator 声明 (ASTVisitor)
│   └── codegen.cpp      # Pascal-S → C 代码生成实现
├── main.cpp             # 命令行入口、编译 pipeline (lex→parse→semantic→codegen)
tests/
├── correct_test/        # 期望解析成功的 .pas 测试文件
├── error_test_bison/    # 期望有语法错误的 .pas 测试文件
├── error_test_lex/      # 期望有词法错误的 .pas 测试文件
├── test_lexer.cpp       # 词法分析器单元测试
├── test_integration.cpp # 词法 + 语法集成测试
└── run_tests.sh         # 批量测试脚本
```

---

## 2. 整体架构

```
源代码 (.pas)
    │
    ▼
┌──────────┐  nextToken()   ┌──────────┐   AST    ┌───────────────┐
│  Lexer   │ ◄───────────── │  Parser  │ ───────► │  AST 节点树    │
│ (按需扫描) │               │ (LL(1))  │          │               │
└──────────┘                └──────────┘          └───────┬───────┘
                                                          │ accept(visitor)
                                                          ▼
                                                  ┌───────────────┐
                                                  │  ASTVisitor   │
                                                  │ (语义分析/代码生成)│
                                                  └───────────────┘
```

**核心设计：词法分析嵌入语法分析。** Parser 在语法分析过程中通过 `lexer->nextToken()` 按需获取 token，不存在独立的词法预处理阶段。词法分析与语法分析交替进行。

---

## 3. 各模块详解

### 3.1 Lexer (`lexer.h` / `lexer.cpp`)

| 方法 | 说明 |
|------|------|
| `nextToken()` | **核心方法。** 每次调用从源码中扫描并返回一个 Token。源码耗尽时返回 `END_OF_FILE`。 |
| `tokenize()` | 便利方法，内部循环调用 `nextToken()` 收集完整 token 列表。 |

内部流程：`nextToken()` → `skipWhitespaceAndComments()` → `scanToken()` → 返回 `Token`。

所有 scan 方法（`scanToken`、`scanNumber`、`scanIdentifierOrKeyword`、`scanCharLiteral`）均直接返回 `Token`，不依赖内部缓冲区。

### 3.2 Grammar (`grammar.h` / `grammar.cpp`)

- **`Production`** 结构体：`{ lhs: string, rhs: vector<string> }`，表示一条产生式。
- **`Grammar`** 类：
  - `buildPascalSGrammar()`：定义所有原始文法产生式。
  - `transformToLL1()`：消除左递归、提取左公因子。
  - `computeFirstSets()` / `computeFollowSets()`：计算 FIRST 和 FOLLOW 集。
  - `buildParseTable()`：构建 LL(1) 分析表 `parseTable[非终结符][终结符] → 产生式编号`。

### 3.3 AST (`ast.h`)

节点层次结构：

```
ASTNode (抽象基类, 含 kind + accept)
├── DeclNode (声明)
│   ├── ConstDeclNode        常量声明
│   ├── VarDeclNode          变量声明
│   ├── SubprogramDeclNode   子程序（过程/函数）声明
│   └── ParamDeclNode        形参声明
├── StmtNode (语句)
│   ├── CompoundStmtNode     复合语句 begin...end
│   ├── AssignStmtNode       赋值语句
│   ├── CallStmtNode         过程调用语句
│   ├── IfStmtNode           if-then-else
│   ├── ForStmtNode          for 循环
│   ├── ReadStmtNode         read 语句
│   ├── WriteStmtNode        write 语句
│   └── EmptyStmtNode        空语句
├── ExprNode (表达式)
│   ├── VariableExprNode     变量引用（含数组下标）
│   ├── LiteralExprNode      字面量（整数、实数、字符）
│   ├── UnaryExprNode        一元表达式（not、负号）
│   ├── BinaryExprNode       二元表达式（算术、关系、逻辑）
│   └── CallExprNode         函数调用表达式
├── BlockNode                程序块
├── ProgramNode              程序根节点
└── PlaceholderNode          占位符（调试用）
```

**Visitor 模式：** 每个节点实现 `accept(ASTVisitor&)`，`ASTVisitor` 基类提供所有 `visitXxx` 虚函数的默认空实现。子类只需 override 关心的方法。

### 3.4 Parser (`parser.h` / `parser.cpp`)

- **LL(1) 表驱动分析器**，使用分析栈 + 语义栈。
- 构造函数 `Parser(Grammar&, Lexer&)`，通过 `lexer->nextToken()` 按需获取 token。
- **语义动作注册表**（`actionTable`）：
  - 类型：`unordered_map<string, vector<SemanticAction>>`
  - 键为产生式左部非终结符名，值为该非终结符对应的语义动作列表。
  - `registerDefaultActions()` 注册所有内置的 AST 构建动作。
  - `registerAction(lhs, action)` 允许外部追加新动作。
  - `buildSemanticValue()` 按注册顺序依次尝试各 handler，第一个返回非 `monostate` 的结果被采用。

---

## 4. 问题一：如何扩写文法

假设要新增一个语法结构（例如 `while` 循环语句 `while <condition> do <statement>`），需要改动以下文件：

### 步骤 1：新增 Token（如果需要新关键字）

**文件：`token.h`**

如果新语法引入了新的关键字或运算符：

1. 在 `TokenType` 枚举中添加新项（如 `WHILE` 已存在，则跳过）。
2. 在 `tokenTypeToString()` 中添加对应的字符串映射。
3. 在 `tokenTypeToTerminal()` 中添加文法终结符映射。

**文件：`lexer.cpp`**

在 `Lexer::keywordMap()` 中添加新关键字到 `TokenType` 的映射。如果是新运算符，在 `scanToken()` 的 switch 中添加分支。

### 步骤 2：添加文法产生式

**文件：`grammar.cpp`**

在 `buildPascalSGrammar()` 函数中添加新的产生式。例如：

```cpp
// 在 statement 产生式附近添加
g.addProduction("statement", {"while", "expression", "do", "statement"});
```

注意：添加后 `transformToLL1()` 会自动消除左递归和提取左公因子，但你需要确保新产生式不会引入 LL(1) 冲突。可通过 `--table` 命令检查分析表。

### 步骤 3：新增 AST 节点（如果需要）

**文件：`ast.h`**

1. 在 `ASTNodeKind` 枚举中添加新项（如 `WhileStmt`）。
2. 在 `astKindToString()` 中添加映射。
3. 定义新的节点结构体：

```cpp
struct WhileStmtNode : StmtNode {
    ExprNodePtr condition;
    StmtNodePtr body;
    WhileStmtNode() : StmtNode(ASTNodeKind::WhileStmt) {}
    void accept(ASTVisitor& visitor) override;
    void print(std::ostream& os, int indent = 0) const override {
        // ...打印逻辑
    }
};
```

4. 在 `ASTVisitor` 类中添加 `virtual void visitWhileStmt(WhileStmtNode&) {}`。
5. 添加 `accept()` 实现：`inline void WhileStmtNode::accept(ASTVisitor& v) { v.visitWhileStmt(*this); }`

### 步骤 4：注册语义动作

**文件：`parser.cpp`**

在 `registerDefaultActions()` 中为新产生式注册 AST 构建逻辑：

```cpp
registerAction("statement", [](const Production& prod, const std::vector<SemanticValue>& rhs) -> SemanticValue {
    // while expression do statement
    if (prod.rhs.size() >= 4 && prod.rhs[0] == "while") {
        auto node = std::make_shared<WhileStmtNode>();
        node->condition = std::get<ExprNodePtr>(rhs[1]);
        // rhs[2] 是 "do" 终结符 (Token)
        node->body = std::get<StmtNodePtr>(rhs[3]);
        return std::static_pointer_cast<StmtNode>(node);
    }
    return std::monostate{};  // 不匹配，交给下一个 handler
});
```

**如果新产生式引入了新的语义值类型**，需要在 `parser.h` 的 `SemanticValue` variant 中添加对应类型。

### 步骤 5：验证

```bash
# 检查分析表是否有冲突
src/pascal_compiler.exe --grammar
src/pascal_compiler.exe --table

# 编写测试用例并运行
src/pascal_compiler.exe tests/correct_test/new_test.pas
```

### 修改清单总结

| 修改内容 | 涉及文件 | 条件 |
|---------|---------|------|
| 新增 TokenType | `token.h` | 新关键字/运算符时 |
| 新增关键字映射 | `lexer.cpp` (keywordMap) | 新关键字时 |
| 新增运算符扫描 | `lexer.cpp` (scanToken) | 新运算符时 |
| 添加产生式 | `grammar.cpp` (buildPascalSGrammar) | **必须** |
| 新增 AST 节点 | `ast.h` | 新语法结构时 |
| 新增 Visitor 方法 | `ast.h` (ASTVisitor) | 新 AST 节点时 |
| 注册语义动作 | `parser.cpp` (registerDefaultActions) | **必须** |
| 扩展 SemanticValue | `parser.h` | 新语义值类型时 |

---

## 5. 问题二：如何进行后续设计（语义分析与 C 代码生成）

### 5.1 总体 Pipeline

```
源代码 → Lexer → Parser → AST
                            │
                            ├── [阶段 3] 语义分析 (SemanticAnalyzer : ASTVisitor)
                            │     ├── 符号表构建
                            │     ├── 类型检查
                            │     └── 作用域检查
                            │
                            └── [阶段 4] C 代码生成 (CodeGenerator : ASTVisitor)
                                  └── 遍历 AST，输出 C 代码
```

两个阶段都通过 **继承 `ASTVisitor`** 来实现，无需修改 Parser 或 Lexer。

### 5.2 语义分析

#### 需要新增的文件

```
src/
├── semantic/
│   ├── symbol_table.h     # 符号表定义
│   ├── symbol_table.cpp   # 符号表实现
│   ├── semantic.h         # SemanticAnalyzer 声明
│   └── semantic.cpp       # SemanticAnalyzer 实现
```

#### 符号表设计

```cpp
// symbol_table.h
enum class SymbolKind { Constant, Variable, Parameter, Function, Procedure };

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::string type;        // "integer", "real", "boolean", "char", "array[...]"
    int scopeLevel;
    // 函数/过程专用
    std::vector<Symbol> params;
    std::string returnType;  // 函数返回类型（过程为空）
};

class SymbolTable {
public:
    void enterScope();       // 进入新作用域
    void exitScope();        // 退出作用域
    bool declare(const Symbol& sym);  // 声明符号（重复声明返回 false）
    Symbol* lookup(const std::string& name);        // 查找（含外层作用域）
    Symbol* lookupCurrent(const std::string& name);  // 仅查当前作用域
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
};
```

#### 语义分析器设计

```cpp
// semantic.h
struct SemanticError {
    int line, column;
    std::string message;
};

class SemanticAnalyzer : public ASTVisitor {
public:
    bool analyze(ProgramNodePtr& ast);
    const std::vector<SemanticError>& getErrors() const;

    // override 所有需要的 visit 方法
    void visitProgram(ProgramNode& node) override;
    void visitBlock(BlockNode& node) override;
    void visitConstDecl(ConstDeclNode& node) override;
    void visitVarDecl(VarDeclNode& node) override;
    void visitSubprogramDecl(SubprogramDeclNode& node) override;
    void visitAssignStmt(AssignStmtNode& node) override;
    void visitCallStmt(CallStmtNode& node) override;
    void visitIfStmt(IfStmtNode& node) override;
    void visitForStmt(ForStmtNode& node) override;
    void visitVariableExpr(VariableExprNode& node) override;
    void visitBinaryExpr(BinaryExprNode& node) override;
    void visitCallExpr(CallExprNode& node) override;
    // ... 其他

private:
    SymbolTable symTable;
    std::vector<SemanticError> errors;
};
```

#### 需要检查的语义规则

- **符号声明与引用**：变量/常量/函数/过程必须先声明后使用
- **重复声明**：同一作用域内不能重复声明
- **类型检查**：赋值两侧类型兼容、运算符操作数类型正确、函数参数类型匹配
- **作用域**：子程序内可访问外层变量，但不能访问同级其他子程序的局部变量
- **函数返回值**：函数体内必须对函数名赋值
- **数组维度**：数组下标表达式的个数必须与声明一致

### 5.3 C 代码生成

#### 需要新增的文件

```
src/
├── codegen/
│   ├── codegen.h          # CodeGenerator 声明
│   └── codegen.cpp        # CodeGenerator 实现
```

#### 代码生成器设计

```cpp
// codegen.h
class CodeGenerator : public ASTVisitor {
public:
    std::string generate(ProgramNodePtr& ast);

    void visitProgram(ProgramNode& node) override;
    void visitBlock(BlockNode& node) override;
    void visitConstDecl(ConstDeclNode& node) override;
    void visitVarDecl(VarDeclNode& node) override;
    void visitSubprogramDecl(SubprogramDeclNode& node) override;
    void visitCompoundStmt(CompoundStmtNode& node) override;
    void visitAssignStmt(AssignStmtNode& node) override;
    void visitCallStmt(CallStmtNode& node) override;
    void visitIfStmt(IfStmtNode& node) override;
    void visitForStmt(ForStmtNode& node) override;
    void visitReadStmt(ReadStmtNode& node) override;
    void visitWriteStmt(WriteStmtNode& node) override;
    void visitVariableExpr(VariableExprNode& node) override;
    void visitLiteralExpr(LiteralExprNode& node) override;
    void visitUnaryExpr(UnaryExprNode& node) override;
    void visitBinaryExpr(BinaryExprNode& node) override;
    void visitCallExpr(CallExprNode& node) override;

private:
    std::ostringstream output;
    int indentLevel = 0;
    void emit(const std::string& code);
    void emitLine(const std::string& code);
    // 需要语义分析结果中的符号表来确定类型
    SymbolTable* symTable = nullptr;
};
```

#### Pascal-S 到 C 的主要映射规则

| Pascal-S | C |
|----------|---|
| `program name(input,output);` | `#include <stdio.h>` + `int main()` |
| `const PI = 3;` | `const int PI = 3;` (需类型推断) |
| `var x : integer;` | `int x;` |
| `var x : real;` | `double x;` |
| `var x : boolean;` | `int x;` (0/1) |
| `var x : char;` | `char x;` |
| `var a : array[1..10] of integer;` | `int a[10];` (注意下标偏移) |
| `begin ... end` | `{ ... }` |
| `x := expr` | `x = expr;` |
| `if c then s1 else s2` | `if (c) s1 else s2` |
| `for i := a to b do s` | `for (i = a; i <= b; i++) s` |
| `read(x)` | `scanf("%d", &x);` (根据类型选格式符) |
| `write(x)` | `printf("%d", x);` (根据类型选格式符) |
| `procedure p(x:integer);` | `void p(int x) { ... }` |
| `function f(x:integer):integer;` | `int f(int x) { ... return f_result; }` |
| `div` | `/` (整除) |
| `mod` | `%` |
| `and` | `&&` |
| `or` | `\|\|` |
| `not` | `!` |

#### 数组下标偏移处理

Pascal 数组 `array[low..high]` 下标从 `low` 开始，C 数组下标从 0 开始。访问 `a[i]` 需转换为 `a[i - low]`。这要求代码生成器能从符号表中查到数组的 `low` 值。

### 5.4 集成到 main.cpp

```cpp
// main.cpp 中 runPipeline 函数的扩展
if (success) {
    // 阶段 3：语义分析
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(parser.getASTRoot())) {
        // 打印语义错误
        for (auto& err : analyzer.getErrors()) {
            std::cerr << "[Line " << err.line << "] " << err.message << "\n";
        }
        return false;
    }

    // 阶段 4：C 代码生成
    CodeGenerator codegen;
    std::string cCode = codegen.generate(parser.getASTRoot());
    std::ofstream fout("output.c");
    fout << cCode;
    std::cout << "C code written to output.c\n";
}
```

### 5.5 修改清单总结

| 任务 | 新增/修改文件 | 是否需要修改现有代码 |
|------|-------------|-------------------|
| 符号表 | 新增 `semantic/symbol_table.h/.cpp` | 否 |
| 语义分析 | 新增 `semantic/semantic.h/.cpp` | 否 |
| C 代码生成 | 新增 `codegen/codegen.h/.cpp` | 否 |
| Pipeline 集成 | 修改 `main.cpp` | 是（添加调用） |
| 新增 AST 节点（如需要）| 修改 `ast.h` | 仅当新增语法时 |
| 新增 Visitor 方法 | 修改 `ast.h` (ASTVisitor) | 仅当新增 AST 节点时 |

**关键结论：语义分析和代码生成完全通过 Visitor 模式扩展，不需要修改 Lexer、Grammar、Parser 的任何代码。** 这正是当前架构设计的扩展性优势。
