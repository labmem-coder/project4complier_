# Pascal-S 词法分析翻译表

本文档给出当前项目词法分析器中“单词类别、内部编码、属性值及说明”的正式翻译表。

## 1. 设计说明

当前项目的词法单元由 [token.h](/d:/Project/project4complier/src/lexer/token.h) 中的 `Token` 结构体表示：

```cpp
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};
```

其中：

- `type`：单词类别的内部编码
- `lexeme`：属性值，保存该单词在源程序中的原始词素
- `line`：该单词起始行号
- `column`：该单词起始列号

本项目使用 `TokenType` 枚举作为内部编码。

## 2. 通用属性值约定

### 2.1 属性值含义

不同类别单词的属性值统一保存在 `lexeme` 中：

- 对关键字：属性值为关键字原始文本，例如 `begin`
- 对标识符：属性值为标识符名称，例如 `count`
- 对常数：属性值为字面文本，例如 `123`、`3.14`、`'A'`、`'Hello'`
- 对运算符和界符：属性值为符号自身，例如 `:=`、`+`、`;`
- 对文件结束符：属性值为 `EOF`

### 2.2 内部编码映射辅助表

当前实现中还提供了以下辅助翻译函数：

- `buildTokenNameMap()`：字符串名称到 `TokenType`
- `tokenTypeToTerminal()`：`TokenType` 到文法终结符名
- `tokenTypeToString()`：`TokenType` 到显示名称

## 3. 翻译表

| 单词类别 | 内部编码 `TokenType` | 典型属性值 `lexeme` | 说明 |
|---|---|---|---|
| 保留字 | `PROGRAM` | `program` | 程序头关键字 |
| 保留字 | `VAR` | `var` | 变量声明关键字 |
| 保留字 | `CONST` | `const` | 常量声明关键字 |
| 保留字 | `PROCEDURE` | `procedure` | 过程声明关键字 |
| 保留字 | `FUNCTION` | `function` | 函数声明关键字 |
| 保留字 | `BEGIN` | `begin` | 复合语句开始 |
| 保留字 | `END` | `end` | 复合语句结束 |
| 保留字 | `IF` | `if` | 条件语句关键字 |
| 保留字 | `THEN` | `then` | 条件分支关键字 |
| 保留字 | `ELSE` | `else` | 条件分支关键字 |
| 保留字 | `FOR` | `for` | `for` 循环关键字 |
| 保留字 | `TO` | `to` | 递增方向关键字 |
| 保留字 | `DOWNTO` | `downto` | 递减方向关键字 |
| 保留字 | `DO` | `do` | 循环体开始关键字 |
| 保留字 | `READ` | `read` | 输入关键字 |
| 保留字 | `WRITE` | `write` | 输出关键字 |
| 保留字 | `WHILE` | `while` | `while` 循环关键字 |
| 保留字 | `REPEAT` | `repeat` | 重复循环关键字 |
| 保留字 | `UNTIL` | `until` | 循环结束条件关键字 |
| 基本类型关键字 | `INTEGER_KW` | `integer` | 整型类型 |
| 基本类型关键字 | `REAL_KW` | `real` | 实型类型 |
| 基本类型关键字 | `BOOLEAN_KW` | `boolean` | 布尔类型 |
| 基本类型关键字 | `CHAR_KW` | `char` | 字符类型 |
| 基本类型关键字 | `STRING_KW` | `string` | 字符串类型 |
| 保留字 | `ARRAY` | `array` | 数组类型关键字 |
| 保留字 | `OF` | `of` | 数组/记录组成关键字 |
| 保留字 | `NOT` | `not` | 逻辑非 |
| 保留字 | `CASE` | `case` | `case` 语句关键字 |
| 保留字 | `BREAK` | `break` | 跳出语句 |
| 保留字 | `CONTINUE` | `continue` | 继续下一轮循环 |
| 保留字 | `RECORD` | `record` | 记录类型关键字 |
| 运算符 | `ASSIGN` | `:=` | 赋值号 |
| 运算符 | `PLUS` | `+` | 加号 |
| 运算符 | `MINUS` | `-` | 减号/一元负号 |
| 运算符 | `MULTIPLY` | `*` | 乘号 |
| 运算符 | `DIVIDE` | `/` | 实数除法 |
| 运算符关键字 | `DIV_KW` | `div` | 整数除法 |
| 运算符关键字 | `MOD` | `mod` | 取模 |
| 运算符关键字 | `AND_KW` | `and` | 逻辑与 |
| 运算符关键字 | `OR_KW` | `or` | 逻辑或 |
| 关系运算符 | `EQ` | `=` | 等于 |
| 关系运算符 | `NE` | `<>` | 不等于 |
| 关系运算符 | `LT` | `<` | 小于 |
| 关系运算符 | `LE` | `<=` | 小于等于 |
| 关系运算符 | `GT` | `>` | 大于 |
| 关系运算符 | `GE` | `>=` | 大于等于 |
| 界符 | `LPAREN` | `(` | 左圆括号 |
| 界符 | `RPAREN` | `)` | 右圆括号 |
| 界符 | `LBRACKET` | `[` | 左方括号 |
| 界符 | `RBRACKET` | `]` | 右方括号 |
| 界符 | `SEMICOLON` | `;` | 分号 |
| 界符 | `COLON` | `:` | 冒号 |
| 界符 | `COMMA` | `,` | 逗号 |
| 界符 | `DOT` | `.` | 句点 |
| 界符 | `DOTDOT` | `..` | 数组下标范围 |
| 数值常量 | `NUM` | `123`、`3.14` | 整数或实数字面量 |
| 字符常量 | `LETTER` | `'A'` | 单字符字面量 |
| 字符串常量 | `STRING` | `'Hello'` | 字符串字面量 |
| 标识符 | `ID` | `count`、`sum1` | 用户定义标识符 |
| 文件结束符 | `END_OF_FILE` | `EOF` | 输入结束标志 |

## 4. 文法终结符翻译关系

除内部编码外，词法分析器还需要把 token 提供给语法分析器。当前项目通过 `tokenTypeToTerminal()` 把内部编码翻译成文法终结符，例如：

| 内部编码 | 文法终结符 |
|---|---|
| `PROGRAM` | `program` |
| `VAR` | `var` |
| `ID` | `id` |
| `NUM` | `num` |
| `LETTER` | `letter` |
| `STRING` | `string` |
| `ASSIGN` | `assignop` |
| `DIV_KW` | `div` |
| `DOWNTO` | `downto` |
| `END_OF_FILE` | `$` |

这说明当前词法分析器已经满足“作为语法分析器调用函数，返回单词类别编码和属性值”的要求。

## 5. 位置属性

本项目还为每个单词记录源程序位置：

- `line`：起始行号
- `column`：起始列号

这部分信息已经用于：

- 词法错误定位
- 语法错误定位
- 语义错误定位

示例：

```text
[Lexer][Line 4, Col 8] Invalid real literal '123.': missing digits after decimal point
[Parser][Line 1, Col 2] Syntax error at 'main': expected {'program'} for 'programstruct'
[Semantic][Line 18, Col 3] Cannot assign to constant 'b'
```

## 6. 结论

当前项目在词法翻译表方面已经明确给出：

- 单词类别
- 内部编码
- 属性值
- 文法终结符翻译关系
- 行列位置信息

因此可以作为该项目词法分析器“内部编码及其属性值（翻译表）”的正式文档。
