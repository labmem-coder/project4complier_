# Pascal-S LL(1) 语法分析器文档

## 1. 编译命令

### 环境要求
- C++17 兼容编译器（g++ 7+ 或 MSVC 2017+）
- Make 工具（可选，也可直接使用 g++ 命令）

### 编译方式

**使用 Make：**
```bash
cd src/parser
make
```

**直接编译（Windows/Linux 通用）：**
```bash
cd src/parser
g++ -std=c++17 -Wall -O2 -o pascal_parser main.cpp grammar.cpp parser.cpp
```

### 运行方式

```bash
# 解析 token 文件
./pascal_parser <token_file>

# 打印 LL(1) 文法
./pascal_parser --grammar

# 打印 FIRST/FOLLOW 集
./pascal_parser --first-follow

# 打印预测分析表
./pascal_parser --table

# 全部输出 + 解析（输出文法、FIRST/FOLLOW、分析表、分析过程到文件）
./pascal_parser --all <token_file>
```

`--all` 模式会生成以下文件：
- `ll1_grammar.txt` — 消除左递归/左公因子后的 LL(1) 文法
- `first_follow.txt` — 所有非终结符的 FIRST 集和 FOLLOW 集
- `parse_table.txt` — LL(1) 预测分析表
- `parse_process.txt` — 语法分析过程日志

### 运行测试

```bash
cd src/parser
make test
```

---

## 2. 词法分析输出 Token 定义

输入文件格式为每行一个 token，格式：
```
TOKEN_TYPE lexeme line column
```

### Token 类型定义表

| Token名 | 含义 | 示例 lexeme |
|---------|------|------------|
| **关键字** | | |
| PROGRAM | program 关键字 | program |
| VAR | var 关键字 | var |
| CONST | const 关键字 | const |
| PROCEDURE | procedure 关键字 | procedure |
| FUNCTION | function 关键字 | function |
| BEGIN | begin 关键字 | begin |
| END | end 关键字 | end |
| IF | if 关键字 | if |
| THEN | then 关键字 | then |
| ELSE | else 关键字 | else |
| FOR | for 关键字 | for |
| TO | to 关键字 | to |
| DO | do 关键字 | do |
| READ | read 关键字 | read |
| WRITE | write 关键字 | write |
| INTEGER | integer 类型关键字 | integer |
| REAL | real 类型关键字 | real |
| BOOLEAN | boolean 类型关键字 | boolean |
| CHAR | char 类型关键字 | char |
| ARRAY | array 关键字 | array |
| OF | of 关键字 | of |
| NOT | not 关键字 | not |
| DIV | div 整除运算符 | div |
| MOD | mod 取模运算符 | mod |
| AND | and 逻辑与 | and |
| OR | or 逻辑或 | or |
| **运算符** | | |
| ASSIGN | 赋值运算符 | := |
| PLUS | 加号 | + |
| MINUS | 减号 | - |
| MULTIPLY | 乘号 | * |
| DIVIDE | 除号 | / |
| EQ | 等于 | = |
| NE | 不等于 | <> |
| LT | 小于 | < |
| LE | 小于等于 | <= |
| GT | 大于 | > |
| GE | 大于等于 | >= |
| **界符** | | |
| LPAREN | 左圆括号 | ( |
| RPAREN | 右圆括号 | ) |
| LBRACKET | 左方括号 | [ |
| RBRACKET | 右方括号 | ] |
| SEMICOLON | 分号 | ; |
| COLON | 冒号 | : |
| COMMA | 逗号 | , |
| DOT | 句点 | . |
| DOTDOT | 范围符 | .. |
| **字面量** | | |
| NUM | 数字字面量 | 42, 3.14 |
| LETTER | 字符字面量 | a |
| ID | 标识符 | myVar |
| **特殊** | | |
| EOF / END_OF_FILE | 文件结束 | EOF |

### Token 到文法终结符的映射

| Token 类型 | 文法终结符 |
|-----------|-----------|
| EQ, NE, LT, LE, GT, GE | relop |
| PLUS, MINUS, OR | addop |
| MULTIPLY, DIVIDE, DIV, MOD, AND | mulop |
| ASSIGN | assignop |
| 其他 | 与关键字/界符名一致 |

> 注意：`-`（MINUS）在 `factor` 产生式中作为一元负号直接匹配 `-`，在 `simple_expression'` 中作为 `addop` 匹配。解析器通过栈顶符号自动区分。

---

## 3. 设计思路

### 3.1 整体架构

```
token文件 → [Token读取器] → token序列 → [LL(1)分析器] → 分析结果
                                            ↑
                              [文法构建] → [左递归/左公因子消除]
                                            ↓
                              [FIRST/FOLLOW计算] → [预测分析表]
```

### 3.2 文法变换

原始 Pascal-S 文法存在以下 LL(1) 不兼容问题：

1. **左递归**：`idlist`, `const_declaration`, `var_declaration`, `parameter_list`, `statement_list`, `expression_list`, `variable_list`, `simple_expression`, `term`, `period` 等产生式含有直接左递归。
2. **左公因子**：`statement` 中 `variable assignop expression`、`func_id assignop expression`、`procedure_call` 均以 `id` 开头；`factor` 中 `variable` 和 `id(expression_list)` 均以 `id` 开头。

**消除策略：**

- **左递归消除**：对形如 `A → Aα | β` 的产生式，引入 `A'`：
  - `A → β A'`
  - `A' → α A' | ε`

- **左公因子提取**：对形如 `A → αβ₁ | αβ₂` 的产生式，引入 `A'`：
  - `A → α A'`
  - `A' → β₁ | β₂`

代码中 `Grammar::eliminateLeftRecursion()` 和 `Grammar::eliminateLeftFactoring()` 实现了通用算法。但因 Pascal-S 文法的 `statement` 和 `factor` 中 id 前缀的歧义较为复杂，在 `buildPascalSGrammar()` 中进行了**手动预变换**（引入 `statement_id_tail` 和 `factor_id_tail`），然后再运行自动变换处理剩余情况。

### 3.3 FIRST/FOLLOW 集计算

标准不动点算法：
- **FIRST(X)**：若 X 是终结符则 FIRST(X)={X}；若 X→ε 则加入 ε；若 X→Y₁Y₂...Yₙ 则依次加入 FIRST(Yᵢ)-{ε}，若所有 Yᵢ 都能推导出 ε 则加入 ε。
- **FOLLOW(S)**：初始加入 $。对 A→αBβ，将 FIRST(β)-{ε} 加入 FOLLOW(B)；若 β⇒*ε 则将 FOLLOW(A) 加入 FOLLOW(B)。

### 3.4 预测分析表构造

对每个产生式 `A → α`：
- 对 FIRST(α) 中每个终结符 a ≠ ε，令 M[A, a] = 该产生式
- 若 ε ∈ FIRST(α)，则对 FOLLOW(A) 中每个终结符 b，令 M[A, b] = 该产生式
- 若存在冲突（同一格有多个产生式），报告 LL(1) 冲突警告

### 3.5 LL(1) 分析过程

使用经典的**表驱动预测分析**：
1. 初始化栈：`$ S`（S 为开始符号）
2. 循环：
   - 栈顶为终结符：与输入匹配则弹出并前进，否则报错
   - 栈顶为非终结符：查表得到产生式，弹出栈顶，将产生式右部逆序压栈
   - 栈顶为 $ 且输入为 $：接受
3. 记录每一步的栈状态、剩余输入和执行动作

### 3.6 错误检测与恢复

采用 **Panic Mode** 错误恢复：
- 当分析表无匹配条目时，报告错误并给出期望的终结符集合
- 若当前输入在 FOLLOW(栈顶非终结符) 中，弹出该非终结符（假设其推导为 ε）
- 否则跳过输入 token 直到找到 FIRST 或 FOLLOW 中的 token
- 终结符不匹配时弹出期望的终结符继续分析

### 3.7 文件结构

```
project_/
├── docs/
│   ├── grammar.md              # 原始文法
│   ├── expected_input_example.md # token 输入示例
│   └── parser_doc.md           # 本文档
├── src/parser/
│   ├── token.h                 # Token 类型定义与映射
│   ├── grammar.h/.cpp          # 文法表示、变换、FIRST/FOLLOW、分析表
│   ├── parser.h/.cpp           # LL(1) 分析器、token读取、错误恢复
│   ├── main.cpp                # 主程序入口
│   └── Makefile                # 构建脚本
└── tests/
    ├── test_simple.tok         # 最简程序
    ├── test_vars.tok           # 变量声明与表达式
    ├── test_control.tok        # 控制流 (if/for)
    ├── test_subprogram.tok     # 子程序 (function)
    ├── test_error.tok          # 错误检测
    └── test_complex.tok        # 复杂程序 (数组、过程、循环)
```
