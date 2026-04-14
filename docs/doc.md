# Pascal-S LL(1) Parser 文档

## 目录

1. [项目概览](#sec-1)
2. [目录结构](#sec-2)
3. [编译](#sec-3)
4. [运行方式](#sec-4)
5. [可执行文件说明](#sec-5)
6. [输入格式](#sec-6)
7. [主程序工作流程](#sec-7)
8. [AST 构建机制](#sec-8)
9. [当前 AST 支持范围](#sec-9)
10. [主要 AST 节点](#sec-10)
11. [词法分析器能力](#sec-11)
12. [语法语义分析器能力](#sec-12)
13. [`-` 的识别说明](#sec-13)
14. [输出说明](#sec-14)
15. [测试覆盖](#sec-15)
16. [已验证样例](#sec-16)
17. [当前测试体系](#sec-17)

<a id="sec-1"></a>
## 1. 项目概览

本项目实现了一个 Pascal-S 子集的编译前端与后续处理流水线，包含：

- 词法分析器 `Lexer`
- 基于 LL(1) 的语法分析器 `Parser`
- FIRST / FOLLOW 集计算
- 预测分析表构建
- 在语法分析过程中同步构建 AST
- 语义分析
- C 代码生成

当前主程序支持对 `.pas` 源文件或旧格式 `.tok` token 文件进行处理，并可输出：

- `tokens.txt`
- `parse_process.txt`
- `ast.txt`

<a id="sec-2"></a>
## 2. 目录结构

- `src/main.cpp`
  主程序入口，负责命令行参数解析与整体流程调度。
- `src/lexer/lexer.h/.cpp`
  词法分析器实现。
- `src/lexer/token.h`
  Token 类型定义，以及 token 显示 / 映射函数。
- `src/parser/grammar.h/.cpp`
  文法、FIRST/FOLLOW、预测分析表相关实现。
- `src/parser/parser.h/.cpp`
  LL(1) 预测分析器与语义动作实现。
- `src/parser/ast.h`
  AST 节点定义。
- `src/semantic/symbol_table.h/.cpp`
  符号表与类型信息实现。
- `src/semantic/semantic_analyzer.h/.cpp`
  语义分析器实现。
- `src/codegen/codegen.h/.cpp`
  Pascal-S 到 C 的代码生成器实现。
- `tests/test_lexer.cpp`
  词法分析单元测试。
- `tests/test_integration.cpp`
  统一集成测试入口：覆盖词法、语法、AST、语义分析、符号表与 C 代码生成对比测试。
- `tests/test_lexer.exe`
  词法分析测试可执行文件。
- `tests/test_integration.exe`
  统一集成测试可执行文件。
- `tests/run_tests.sh`
  统一命令行测试脚本：先运行 `test_integration.exe`，再运行基于 `tests/correct_test`、`tests/error_test_bison`、`tests/error_test_lex`、`tests/semantic_error_test` 的语料回归测试。

<a id="sec-3"></a>
## 3. 编译

### 环境要求

- 支持 C++17 的 `g++`
- Windows / PowerShell 环境下可直接编译运行

### 主编译器

`src/pascal_compiler.exe` 是项目的主编译器入口。它负责执行完整流水线：

- 词法分析
- 语法分析
- AST 构建
- 语义分析
- C 代码生成

进入项目根目录后执行：

```bash
g++ -std=c++17 -Isrc/lexer -Isrc/parser -Isrc/semantic -Isrc/codegen -o src/pascal_compiler.exe src/main.cpp src/lexer/lexer.cpp src/parser/grammar.cpp src/parser/parser.cpp src/semantic/symbol_table.cpp src/semantic/semantic_analyzer.cpp src/codegen/codegen.cpp
```

示例：

```bash
# 简单程序
src/pascal_compiler.exe tests/correct_test/correct1.pas

# 数组测试
src/pascal_compiler.exe tests/correct_test/correct5.pas

# 函数测试
src/pascal_compiler.exe tests/correct_test/correct2.pas

# 详细输出（打印 AST + 生成的 C 代码）
src/pascal_compiler.exe --all tests/correct_test/correct4.pas

# 指定输出文件
src/pascal_compiler.exe -o output.c tests/correct_test/correct1.pas

# 验证生成的 C 代码是否能编译运行
gcc -o test_output tests/correct_test/correct1.c
./test_output
```

### 调试版本

```bash
g++ -std=c++17 -g -Isrc/lexer -Isrc/parser -Isrc/semantic -Isrc/codegen -o src/pascal_compiler.exe src/main.cpp src/lexer/lexer.cpp src/parser/grammar.cpp src/parser/parser.cpp src/semantic/symbol_table.cpp src/semantic/semantic_analyzer.cpp src/codegen/codegen.cpp
```

### 测试程序

编译词法测试：

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser ../tests/test_lexer.cpp lexer/lexer.cpp -o ../tests/test_lexer.exe
```

编译统一集成测试：

```bash
cd src
g++ -std=c++17 -Wall -Wextra -O2 -I. -I./lexer -I./parser -I./semantic -I./codegen ../tests/test_integration.cpp lexer/lexer.cpp parser/grammar.cpp parser/parser.cpp semantic/symbol_table.cpp semantic/semantic_analyzer.cpp codegen/codegen.cpp -o ../tests/test_integration.exe
```

<a id="sec-4"></a>
## 4. 运行方式

### 主程序

主程序统一使用 `src/pascal_compiler.exe`：

```bash
src/pascal_compiler.exe --lex <source.pas>
src/pascal_compiler.exe <source.pas|tokens.tok>
src/pascal_compiler.exe --grammar
src/pascal_compiler.exe --first-follow
src/pascal_compiler.exe --table
src/pascal_compiler.exe --all <source.pas|tokens.tok>
```

### 测试程序

运行词法测试：

```bash
cd src
../tests/test_lexer.exe
```

运行统一集成测试：

```bash
cd src
../tests/test_integration.exe
```

运行统一命令行测试脚本：

```bash
cd ..
bash tests/run_tests.sh
```

<a id="sec-5"></a>
## 5. 可执行文件说明

- `pascal_compiler.exe`
  主编译器入口。用于对 `.pas` 或 `.tok` 文件执行词法分析、语法分析、AST 构建、语义分析，并在通过后生成 C 代码。
- `test_lexer(.exe)`
  词法分析测试程序。用于验证 token 切分、关键字识别、注释跳过、错误检测等词法行为。
- `test_integration(.exe)`
  统一集成测试程序。用于验证完整 Pascal-S 流水线，包括 lexer、parser、AST、semantic analysis、symbol table 以及 C 代码生成结果。

<a id="sec-6"></a>
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

<a id="sec-7"></a>
## 7. 主程序工作流程

当前主程序的处理流程如下：

1. 构建 Pascal-S 文法
2. 对文法执行左因子提取与左递归消除
3. 计算 FIRST 集与 FOLLOW 集
4. 构建 LL(1) 预测分析表
5. 对输入执行词法分析
6. 进行语法分析
7. 在语法分析过程中同步构建 AST
8. 进行语义分析
9. 生成 C 代码

对应源码：

- `src/parser/grammar.h/.cpp`：文法、FIRST/FOLLOW、预测分析表
- `src/parser/parser.h/.cpp`：预测分析器与语义动作
- `src/lexer/lexer.h/.cpp`、`src/lexer/token.h`：词法分析与 token 定义
- `src/parser/ast.h`：AST 节点体系
- `src/semantic/semantic_analyzer.h/.cpp`：语义分析
- `src/semantic/symbol_table.h/.cpp`：符号表
- `src/codegen/codegen.h/.cpp`：C 代码生成
- `src/main.cpp`：主流程入口

<a id="sec-8"></a>
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

<a id="sec-9"></a>
## 9. 当前 AST 支持范围

### 已支持

- 程序结构：`programstruct`、`program_head`、`program_body`
- 声明：`const_declarations`、`var_declarations`
- 语句：复合语句、赋值、过程调用、`if`、`for`、`read`、`write`
- 表达式：关系表达式、加法表达式、乘法表达式、一元 `not`、一元负号
- 因子：字面量、变量、数组下标、函数调用、括号表达式
- 辅助列表：`idlist`、`expression_list`、`variable_list`

### 当前仍较简化的部分

- `subprogram`
- `subprogram_head`
- `formal_parameter`
- `parameter_list`
- `parameter`
- `var_parameter`
- `value_parameter`

这些部分已经参与语法分析，但 AST 结构仍有一定简化。

<a id="sec-10"></a>
## 10. 主要 AST 节点

定义位于 `src/parser/ast.h`，当前核心节点包括：

- `ProgramNode`
- `BlockNode`
- `ConstDeclNode`
- `VarDeclNode`
- `SubprogramDeclNode`
- `ParamDeclNode`
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

<a id="sec-11"></a>
## 11. 词法分析器能力

当前词法分析器支持：

- Pascal-S 关键字识别
- 大小写不敏感关键字匹配
- 标识符识别
- 整数与实数识别
- 字符常量识别
- 字符串常量识别
- 运算符与界符识别
- `{ ... }` 注释
- `(* ... *)` 注释
- `// ...` 单行注释
- 行号、列号跟踪

补充说明：

- `DOWNTO` 已支持完整的 `for ... downto ... do ...` 语法、语义检查和 C 代码生成
- `RECORD` 已支持记录类型声明、字段访问、语义检查和对应的 C `struct` 代码生成

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
*** ERRORS: LEXER (N) ***
  [Lexer][Line x, Col y] error message
```

<a id="sec-12"></a>
## 12. 语法语义分析器能力

当前语法分析器与语义分析器已经支持以下能力：

- 基于 LL(1) 预测分析表进行语法分析
- 在语法分析过程中同步构建 AST
- 在出现语法错误时使用 panic mode 做基本恢复
- 在 AST 构建完成后进行语义检查
- 在语义错误存在时阻止代码生成

### 当前支持检测的语法错误

当前语法阶段可以稳定检测的典型错误包括：

1. 缺少分号
   例如语句之间或声明之间缺少 `;`

2. 程序头不完整
   例如缺少 `program`、程序名、`;` 等

3. 类型声明结构非法
   例如不符合当前文法的变量或数组声明

4. 表达式不完整
   例如 `x := ;`、`write(1 + )`

5. 语句结构不完整
   例如 `if` 缺少 `then`、`for` 缺少 `do`、`while` 缺少 `do`、`case` 缺少 `of` 或 `end`

6. 程序结束错误
   例如缺少最终 `.`，或程序结束后仍有多余 token

### 当前支持检测的语义错误

当前语义阶段已经实现并验证过的错误检查包括：

1. 未声明标识符
   例如使用未声明变量、调用未声明函数或过程

2. 重复声明
   包括常量、变量、子程序、参数名重复

3. 赋值类型不匹配
   当前支持整数到实数的隐式提升，不支持不兼容类型赋值

4. 常量赋值
   禁止对 `const` 声明的标识符赋值

5. 函数 / 过程参数个数不匹配

6. 函数 / 过程参数类型不匹配

7. `var` 参数实参非法
   按引用参数必须传变量，不能传字面量或表达式

8. 数组下标类型错误
   数组下标必须为整数

9. 数组声明上下界非法
   例如 `array[5..1] of integer`

10. 静态数组越界
   例如访问 `array[1..3]` 的 `a[4]`

11. 函数缺少返回赋值
   函数体中若从未对函数名赋值，会报错

12. `div` / `mod` 操作数类型错误
   两侧都必须为整数

13. `+ - * /` 操作数类型错误
   两侧必须为数值类型

14. `and / or / not` 操作数类型错误
   需要布尔类型操作数

15. 关系运算两侧类型不兼容

16. `read()` 参数非法
   `read()` 参数必须是变量

17. `break` 使用位置非法
   当前仅允许出现在 `for` 或 `case` 中

18. `continue` 使用位置非法
   当前仅允许出现在 `for` 或 `while` 循环中

19. `case` 标签类型不匹配

20. `case` 标签重复

21. `while` 条件类型非法
   `while` 条件必须为布尔类型

22. `for ... downto ...` 上下界类型非法
   `for` 循环的起始值、终止值以及迭代变量必须为整数类型

23. 记录字段访问非法
   包括访问不存在的字段，或对非记录类型使用字段访问

### 错误信息输出特点

当前词法、语法、语义错误输出遵循以下原则：

- 简洁明了，优先指出“哪里错了”和“为什么错”
- 三层错误统一使用 `[阶段][Line x, Col y] message` 的形式
- 语法错误带行号、列号、出错 token 和期望集合
- 语义错误以自然语言直接描述问题，并尽量附带源码位置
- 若语义分析失败，会明确提示跳过代码生成

当前统一的阶段前缀为：

- `Lexer`
- `Parser`
- `Semantic`

语法错误输出示例：

```text
*** ERRORS: PARSER (N) ***
  [Parser][Line x, Col y] Syntax error at 'token': expected {...} for 'non_terminal'
```

语义错误输出示例：

```text
*** ERRORS: SEMANTIC (N) ***
  [Semantic][Line x, Col y] Duplicate variable declaration: 'x'
  [Semantic][Line x, Col y] Cannot assign to constant 'PI'
  [Semantic][Line x, Col y] Argument 1 of 'f' expects integer, got char
```

<a id="sec-13"></a>
## 13. `-` 的识别说明

表达式中的 `-` 同时可能表示：

- 二元减号
- 一元负号

当前实现没有单独增加新的 token，而是在 parser 中根据上下文动态区分：

- 若当前位置需要匹配一元负号入口，则按 `- factor` 处理
- 若当前位置处于加法表达式尾部，则按 `addop` 中的减号处理

因此以下表达式均可正确识别：

- `d := -2`
- `a - b`
- `- -(c + 3)`

<a id="sec-14"></a>
## 14. 输出说明

### 成功时

- 标准输出打印分析结果
- 若使用 `--all`，会输出 `tokens.txt`、`parse_process.txt`、`ast.txt`
- 同时会在终端打印 AST
- 若语义分析通过，会继续生成 C 代码

### 失败时

- 若有词法错误，会打印统一格式的词法错误列表
- 若有语法错误，会打印统一格式的语法错误数量及位置
- 若有语义错误，会打印统一格式的语义错误列表，并跳过代码生成
- 语法分析器仍使用 panic mode 做基本错误恢复

统一错误输出格式如下：

```text
*** ERRORS: LEXER (N) ***
  [Lexer][Line x, Col y] ...

*** ERRORS: PARSER (N) ***
  [Parser][Line x, Col y] ...

*** ERRORS: SEMANTIC (N) ***
  [Semantic][Line x, Col y] ...
```

<a id="sec-15"></a>
## 15. 测试覆盖

### `test_lexer`

当前覆盖的词法测试包括：

- 关键字识别
- 大小写不敏感
- 标识符识别
- 整数与实数
- `1..10` 与实数区分
- 字符常量
- 字符串常量
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

当前 `test_integration` 是统一集成测试入口，已经覆盖三层错误与成功路径：

- 词法测试
  关键字、标识符、字符串、字符、注释、大小写不敏感、行列号，以及非法字符、非法标识符、非法数字、括号不匹配、未闭合字符 / 字符串 / 注释等错误
- 语法测试
  简单程序、无参调用、函数 / 子程序 AST 构建，以及缺少 `program`、缺少 `;`、缺少 `then`、缺少 `do`、缺少 `end`、缺少 `.`、类型声明不完整、表达式不完整、语句结构不完整
- 语义测试
  重复声明、未声明标识符、类型不匹配、常量赋值、参数个数 / 类型不匹配、`var` 参数检查、数组边界与下标检查、函数返回赋值检查、`break` / `continue` 非法位置、`while` 条件检查、`case` 标签检查、记录字段访问检查、一元 / 二元运算数类型检查
- 其余集成能力
  符号表作用域与遮蔽、注释跨阶段处理、关键字大小写不敏感、`while`、`for ... downto ...`、`record`、`case`、`break`、`continue`、零参数函数值引用、C 代码生成对比

<a id="sec-16"></a>
## 16. 已验证样例

你可以使用以下目录中的样例进行验证：

- `tests/pascal/`
- `tests/correct_test/`
- `tests/error_test_lex/`
- `tests/error_test_bison/`
- `tests/semantic_error_test/`

建议验证方式：

```bash
cd src
./pascal_compiler --all ../tests/correct_test/correct4.pas
./pascal_compiler --lex ../tests/error_test_lex/invalid_ascii_1.pas
```

<a id="sec-17"></a>
## 17. 当前测试体系

### `test_integration`

当前 `test_integration` 已经是统一集成测试入口，覆盖内容包括：

- 词法测试：字符串、字符、注释、关键字大小写不敏感、非法字符、非法标识符、非法数字、括号不匹配、未闭合字符 / 字符串 / 注释
- 语法测试：简单程序、无参调用、函数 / 子程序、`while`、`for ... downto ...`、`record` 字段访问、`case` / `break` / `continue`
- 语法错误测试：缺少 `program`、缺少 `;`、缺少 `then`、缺少 `do`、缺少 `end`、缺少 `.`、类型声明不完整、表达式不完整、语句结构不完整
- AST 测试：子程序参数与函数体构建
- 语义测试：重复声明、未声明标识符、类型不匹配、常量赋值、函数 / 过程参数个数与类型检查、`var` 参数检查、数组边界与下标检查、函数返回赋值检查、`break` / `continue` 上下文检查、`while` 条件类型检查、`case` 标签检查、记录字段检查、一元 / 二元运算数类型检查
- 符号表测试：作用域、遮蔽、当前作用域查找
- 代码生成对比测试：简单输出、函数调用、字符串输出、`string` 类型变量、`switch` 代码生成、`for ... downto ...` 循环、`record` 到 C `struct` 的映射
- 全流水线测试：注释处理、关键字大小写不敏感、零参数函数调用、零参数函数值引用、扩展 Pascal-S 特性

当前 `tests/test_integration.exe` 的测试结果为：

- 词法、语法、语义、符号表、代码生成统一集成测试：`68/68 passed`

### `run_tests.sh`

`tests/run_tests.sh` 是统一命令行测试脚本，执行顺序为：

1. 运行 `tests/test_integration.exe`
2. 运行 `src/pascal_compiler.exe` 对 `tests/correct_test/*.pas` 做成功语料回归测试
3. 运行 `src/pascal_compiler.exe` 对 `tests/error_test_bison/*.pas` 做失败语料回归测试
4. 运行 `src/pascal_compiler.exe` 对 `tests/error_test_lex/*.pas` 做失败语料回归测试
5. 运行 `src/pascal_compiler.exe` 对 `tests/semantic_error_test/*.pas` 做语义失败语料回归测试

脚本中的通过 / 失败判定以编译器进程退出码为准，而不是只看 `PARSE SUCCESSFUL` 文本。

