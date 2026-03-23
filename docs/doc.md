# Pascal-S LL(1) Parser 文档

## 1. 项目概览

本项目实现了一个 Pascal-S 子集的编译前端，包含：

- 词法分析器 `Lexer`
- 基于 LL(1) 的语法分析器 `Parser`
- FIRST / FOLLOW 集计算
- 预测分析表构造
- 语法分析过程中同步构建 AST

当前主程序支持对 `.pas` 源文件或旧格式 `.tok` token 文件进行处理，并可输出：

- `tokens.txt`
- `parse_process.txt`
- `ast.txt`

## 2. 目录结构

- `src/main.cpp`
  主程序入口，负责命令行参数解析与整体流程调度。
- `src/lexer/lexer.h/.cpp`
  词法分析器实现。
- `src/lexer/token.h`
  Token 类型定义与 token 显示/映射函数。
- `src/parser/grammar.h/.cpp`
  文法、FIRST/FOLLOW、预测分析表相关实现。
- `src/parser/parser.h/.cpp`
  LL(1) 预测分析器与语义动作实现。
- `src/parser/ast.h`
  AST 节点定义。
- `tests/test_lexer.cpp`
  词法分析单元测试。
- `tests/test_integration.cpp`
  词法分析 + 语法分析 + AST 的集成测试。

## 3. 编译

### 环境要求

- 支持 C++17 的 `g++`
- Windows / PowerShell 环境下可直接编译运行

### 主程序

进入 `src` 目录后执行：
## 生成C代码
```bash
cd src
g++ -std=c++17 -Isrc/lexer -Isrc/parser -Isrc/semantic -Isrc/codegen -o src/pascal_compiler.exe src/main.cpp src/lexer/lexer.cpp src/parser/grammar.cpp src/parser/parser.cpp src/semantic/symbol_table.cpp src/semantic/semantic_analyzer.cpp src/codegen/codegen.cpp

# 简单程序
src/pascal_compiler.exe tests/correct_test/correct1.pas

# 数组测试
src/pascal_compiler.exe tests/correct_test/correct5.pas

# 函数测试
src/pascal_compiler.exe tests/correct_test/correct2.pas

# 详细输出 (打印 AST + 生成的 C 代码)
src/pascal_compiler.exe --all tests/correct_test/correct4.pas

# 指定输出文件
src/pascal_compiler.exe -o output.c tests/correct_test/correct1.pas

# 验证生成的 C 代码能否编译运行
gcc -o test_output tests/correct_test/correct1.c
./test_output
```

## 词法语法分析部分
```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser main.cpp lexer/lexer.cpp parser/grammar.cpp parser/parser.cpp -o pascal_compiler
```

如果你希望输出文件名叫 `pascal_parser`，只需改 `-o` 参数：

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser main.cpp lexer/lexer.cpp parser/grammar.cpp parser/parser.cpp -o pascal_parser
```

说明：

- `pascal_compiler(.exe)` 与 `pascal_parser(.exe)` 由同一套源码生成
- 唯一差别只是输出可执行文件名不同
- 两者本质上是同一个主程序

### 调试版本

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -g -I. -I./lexer -I./parser main.cpp lexer/lexer.cpp parser/grammar.cpp parser/parser.cpp -o pascal_compiler
```

### 测试程序

编译词法测试：

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser ../tests/test_lexer.cpp lexer/lexer.cpp -o test_lexer
```

编译集成测试：

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser ../tests/test_integration.cpp lexer/lexer.cpp parser/grammar.cpp parser/parser.cpp -o test_integration
```

## 4. 运行方式

### 主程序

若编译输出名为 `pascal_compiler`：

```bash
./pascal_compiler --lex <source.pas>
./pascal_compiler <source.pas|tokens.tok>
./pascal_compiler --grammar
./pascal_compiler --first-follow
./pascal_compiler --table
./pascal_compiler --all <source.pas|tokens.tok>
```

若编译输出名为 `pascal_parser`：

```bash
./pascal_parser --lex <source.pas>
./pascal_parser <source.pas|tokens.tok>
./pascal_parser --grammar
./pascal_parser --first-follow
./pascal_parser --table
./pascal_parser --all <source.pas|tokens.tok>
```

### 测试程序

运行词法测试：

```bash
cd src
./test_lexer
```

运行集成测试：

```bash
cd src
./test_integration
```

## 5. 可执行文件说明

- `pascal_compiler(.exe)` / `pascal_parser(.exe)`
  主程序。用于对 `.pas` 或 `.tok` 文件执行词法分析、语法分析，并输出分析结果。
- `test_lexer(.exe)`
  词法分析测试程序。用于验证 token 切分、关键字识别、注释跳过、错误检测等词法行为。
- `test_integration(.exe)`
  集成测试程序。用于验证完整 Pascal-S 程序能否通过 lexer、parser，并成功构建 AST。

## 6. 输入格式

### Pascal-S 源文件

主输入形式为 `.pas` 文件，例如：

```pascal
program test(input, output);
begin
end.
```

### Token 文件

兼容旧格式 `.tok` 文件，每行一个 token，格式如下：

```text
TOKEN_TYPE lexeme line column
```

例如：

```text
PROGRAM program 1 1
ID test 1 9
SEMICOLON ; 1 13
```

## 7. 主程序工作流程

当前主程序的处理流程如下：

1. 构建 Pascal-S 文法
2. 对文法执行左因子提取与左递归消除
3. 计算 FIRST 集与 FOLLOW 集
4. 构造 LL(1) 预测分析表
5. 对输入执行词法分析
6. 进行语法分析
7. 在语法分析过程中同步构建 AST

对应源码：

- `src/parser/grammar.h/.cpp`：文法、FIRST/FOLLOW、预测分析表
- `src/parser/parser.h/.cpp`：预测分析器与语义动作
- `src/lexer/lexer.h/.cpp`、`src/lexer/token.h`：词法分析与 token 定义
- `src/parser/ast.h`：AST 节点体系
- `src/main.cpp`：主流程入口

## 8. AST 构建机制

AST 构建与 LL(1) 预测分析过程绑定，不是额外单独遍历生成。

大致过程如下：

1. 分析栈根据预测分析表进行推导
2. 产生式被选中后，除压入右部符号外，还会压入内部动作标记
3. 当右部完成匹配后，动作标记触发规约
4. 规约时从语义栈取出对应右部语义值，组装新的 AST 节点
5. 最终在 `programstruct` 规约完成时得到整棵 AST 根节点

关键点：

- 语法判断仍完全依赖预测分析表
- AST 构建发生在 `PREDICT -> MATCH -> REDUCE` 这条链路中
- 终结符匹配时会把 token 放入语义栈
- 非终结符规约时会把 token、表达式、语句、声明等组合成更高层 AST 节点

## 9. 当前 AST 支持范围

### 已支持

- 程序结构：`programstruct`、`program_head`、`program_body`
- 声明：`const_declarations`、`var_declarations`
- 语句：复合语句、赋值、过程调用、`if`、`for`、`read`、`write`
- 表达式：关系表达式、加法表达式、乘法表达式、一元 `not`、一元负号
- 因子：字面量、变量、数组下标、函数调用、括号表达式
- 辅助列表：`idlist`、`expression_list`、`variable_list`

### 目前仍较简化的部分

- `subprogram`
- `subprogram_head`
- `formal_parameter`
- `parameter_list`
- `parameter`
- `var_parameter`
- `value_parameter`

这些部分已经参与语法分析，但 AST 结构仍偏简化。

## 10. 主要 AST 节点

定义位于 `src/parser/ast.h`，当前核心节点包括：

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

## 11. 词法分析器能力

当前词法分析器支持：

- Pascal-S 关键字识别
- 大小写不敏感关键字匹配
- 标识符识别
- 整数与实数识别
- 字符常量识别
- 运算符与界符识别
- `{ ... }` 注释
- `(* ... *)` 注释
- 行号、列号跟踪

### 当前支持检测的词法错误

1. 非法标识符
   例如 `1a`、`2temp`
   规则：标识符不能以数字开头

2. 非法数字格式
   例如 `1.2.3`
   规则：一个数字中不能出现多个小数点

3. 非法字符
   例如中文括号、中文分号、书名号等非 ASCII 字符

4. 括号不匹配
   例如 `( ]`、`[ )`

5. 左括号未闭合
   例如 `(` 或 `[` 后直到文件结束都没有匹配的右括号

6. 注释未闭合
   例如 `{ ...` 或 `(* ...` 没有结束符

### 错误信息输出方式

词法错误会记录：

- 行号
- 列号
- 错误消息

在 `--lex` 模式下，出现词法错误时主程序会打印：

```text
*** LEXER ERRORS ***
  [Line x, Col y] error message
```

## 12. `-` 的识别说明

表达式中的 `-` 同时可能表示：

- 二元减号
- 一元负号

当前实现没有修改文法，而是在 parser 中根据当前预测分析表上下文动态决定：

- 若当前位置需要匹配一元负号入口，则按 `- factor` 处理
- 若当前位置处于加法表达式尾部，则按 `addop` 中的减号处理

因此以下表达式均可正确识别：

- `d := -2`
- `a - b`
- `- -(c + 3)`

## 13. 输出说明

### 成功时

- 标准输出打印分析结果
- 若使用 `--all`，会输出 `tokens.txt`、`parse_process.txt`、`ast.txt`
- 同时会在终端打印 AST

### 失败时

- 若有词法错误，会打印词法错误列表
- 若有语法错误，会打印语法错误数量及位置
- 语法分析器仍采用 panic mode 做基本错误恢复

## 14. 测试覆盖

### `test_lexer`

当前覆盖的词法测试包括：

- 关键字识别
- 大小写不敏感
- 标识符识别
- 整数与实数
- `1..10` 与实数区分
- 字符常量
- 运算符与界符
- 注释跳过
- 行列号
- 未闭合注释
- 非法字符
- 非法标识符
- 多小数点非法数字
- 非 ASCII 字符
- 括号错配
- 左括号未闭合

### `test_integration`

当前覆盖的集成测试包括：

- 简单程序
- 无参数程序
- 常量与变量声明
- `if / else`
- `for`
- 表达式优先级
- 一元负号
- 数组访问
- 函数/子程序
- 注释
- 字符常量
- AST 输出
- 关键字大小写不敏感

## 15. 已验证样例

你可以使用以下目录中的样例进行验证：

- `tests/pascal/`
- `tests/correct_test/`
- `tests/error_test_lex/`
- `tests/error_test_bison/`

建议验证方式：

```bash
cd src
./pascal_compiler --all ../tests/correct_test/correct4.pas
./pascal_compiler --lex ../tests/error_test_lex/invalid_ascii_1.pas
```
