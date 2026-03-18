# Pascal-S LL(1) Parser 文档

## 1. 构建与运行

### 环境要求
- C++17 编译器
- Windows 下可直接使用 `g++`
- `make` 可选

### 编译

```bash
cd src/parser
g++ -std=c++17 -Wall -Wextra -O2 main.cpp grammar.cpp parser.cpp -o pascal_parser
```

### 运行

```bash
./pascal_parser <token_file>
./pascal_parser --grammar
./pascal_parser --first-follow
./pascal_parser --table
./pascal_parser --all <token_file>
```

`--all` 模式会输出：
- `ll1_grammar.txt`
- `first_follow.txt`
- `parse_table.txt`
- `parse_process.txt`
- `ast.txt`

## 2. 输入格式

Token 文件每行一个 token，格式如下：

```text
TOKEN_TYPE lexeme line column
```

例如：

```text
PROGRAM program 1 1
ID test 1 9
SEMICOLON ; 1 13
```

## 3. 解析器结构

当前实现流程：

1. 构建 Pascal-S 文法
2. 执行左因子提取与左递归消除
3. 计算 FIRST / FOLLOW 集
4. 生成 LL(1) 预测分析表
5. 基于预测分析表进行表驱动语法分析
6. 在语法分析过程中同步构建 AST

对应源码：
- `src/parser/grammar.h/.cpp`：文法、FIRST/FOLLOW、预测分析表
- `src/parser/parser.h/.cpp`：表驱动分析器与 AST 语义动作
- `src/parser/ast.h`：AST 节点体系
- `src/parser/main.cpp`：命令行入口与结果输出

## 4. AST 构建机制

AST 构建严格绑定在 LL(1) 预测分析流程中，没有绕过预测分析表。

实现方式：

1. 分析栈仍然按原有方式由预测分析表驱动
2. 当某条产生式被选中时，解析器除了压入 RHS 之外，还会压入一个内部语义动作标记
3. 当 RHS 完成匹配后，动作标记触发规约
4. 规约时从语义栈弹出该产生式对应的语义值，拼装父节点，再压回语义栈
5. 最终在 `programstruct` 规约完成时得到 AST 根节点

这套机制的关键点：
- 语法判断仍完全依赖 `parseTable`
- AST 构建只发生在 `PREDICT -> MATCH -> REDUCE` 这条链路中
- 终结符匹配时会把 token 放入语义栈
- 非终结符规约时会把 token、子表达式、语句、声明等合并为更高层节点

## 5. 当前 AST 支持范围

### 已完整支持
- 程序结构：`programstruct`、`program_head`、`program_body`
- 声明：`const_declarations`、`var_declarations`
- 语句：`compound_statement`、赋值、过程调用、`if`、`for`、`read`、`write`
- 表达式：关系表达式、加法表达式、乘法表达式、一元 `not`、一元负号
- 因子：字面量、变量、数组下标、函数调用、括号表达式
- 辅助列表：`idlist`、`expression_list`、`variable_list`

### 当前先保留占位/未细化
- `subprogram`
- `subprogram_head`
- `formal_parameter`
- `parameter_list`
- `parameter`
- `var_parameter`
- `value_parameter`

这些部分仍参与语法分析，但 AST 细节尚未完全展开，当前实现重点是先验证“预测分析表 + 语义栈 + AST 规约”这条架构可行。

## 6. 主要 AST 节点

定义位于 `src/parser/ast.h`，核心节点包括：

- `ProgramNode`
- `BlockNode`
- `ConstDeclNode`
- `VarDeclNode`
- `CompoundStmtNode`
- `AssignStmtNode`
- `CallStmtNode`
- `IfStmtNode`
- `ForStmtNode`
- `ReadStmtNode`
- `WriteStmtNode`
- `VariableExprNode`
- `LiteralExprNode`
- `UnaryExprNode`
- `BinaryExprNode`
- `CallExprNode`

## 7. 输出说明

解析成功时：
- 标准输出打印分析过程
- 标准输出打印 AST
- `ast.txt` 保存 AST 文本结构

解析失败时：
- 保留分析过程日志
- 输出错误数量与位置信息
- 错误恢复仍采用 panic mode

## 8. 已验证用例

已用以下样例验证当前实现：

- `tests/test_simple.tok`
- `tests/test_vars.tok`
- `tests/test_control.tok`
- `tests/test_subprogram.tok`
- `tests/test_complex.tok`
- `tests/test_error.tok`

验证结果：
- 正确样例可以完成语法分析并输出 AST
- 错误样例仍能正常报错，不影响原有 LL(1) 分析逻辑
