# Pascal-S语言完整文法产生式
以下是从文档中提取的**全部Pascal-S语言文法产生式**，按原文分块逻辑整理，无遗漏且保留原始符号与推导关系：

## 1. 程序结构相关（1/5）
programstruct → program_head ; program_body .
program_head → program id ( idlist ) | program id
program_body → const_declarations var_declarations subprogram_declarations compound_statement
idlist → id | idlist , id
const_declarations → ε | const const_declaration ;
const_declaration → id = const_value | const_declaration ; id = const_value
const_value → + num | - num | num | ′ letter ′

## 2. 变量与子程序声明相关（2/5）
var_declarations → ε | var var_declaration ;
var_declaration → idlist : type | var_declaration ; idlist : type
type → basic_type | array [ period ] of basic_type
basic_type → integer | real | boolean | char
period → digits .. digits | period , digits .. digits
subprogram_declarations → ε | subprogram_declarations subprogram ;
subprogram → subprogram_head ; subprogram_body
subprogram_head → procedure id formal_parameter | function id formal_parameter : basic_type
formal_parameter → ε | ( parameter_list )
parameter_list → parameter | parameter_list ; parameter

## 3. 子程序参数与复合语句相关（3/5）
parameter → var_parameter | value_parameter
var_parameter → var value_parameter
value_parameter → idlist : basic_type
subprogram_body → const_declarations var_declarations compound_statement
compound_statement → begin statement_list end
statement_list → statement | statement_list ; statement

## 4. 语句与变量相关（4/5）
statement → ε | variable assignop expression | func_id assignop expression | procedure_call | compound_statement | if expression then statement else_part | for id assignop expression to expression do statement | read ( variable_list ) | write ( expression_list )
variable_list → variable | variable_list , variable
variable → id id_varpart
id_varpart → ε | [ expression_list ]

## 5. 过程调用与表达式相关（5/5）
procedure_call → id | id ( expression_list )
else_part → ε | else statement
expression_list → expression | expression_list , expression
expression → simple_expression | simple_expression relop simple_expression
simple_expression → term | simple_expression addop term
term → factor | term mulop factor
factor → num | variable | ( expression ) | id ( expression_list ) | not factor | uminus factor
## 6. 修改的文法和添加的文法

本节记录在当前项目实现过程中，相比上面原始文法说明已经实际修改或补充到代码中的内容。

### 6.1 expression_list 可为空

为支持无参函数/过程调用，例如 `ifWhile()`，将实参列表从“至少一个表达式”扩展为“可以为空”：

```text
expression_list → ε | expression expression_list'
expression_list' → , expression expression_list' | ε
```

这项修改影响了以下调用形式：

```text
procedure_call → id | id ( expression_list )
factor → ... | id ( expression_list ) | ...
statement_id_tail → ... | ( expression_list ) | ...
factor_id_tail → ... | ( expression_list ) | ...
```

### 6.2 factor 新增 string 字面量

为支持字符串字面量，例如：

```pascal
writeln('Hello, world!');
```

在表达式因子中补充 `string`：

```text
factor → num | string | variable | ( expression ) | id ( expression_list ) | not factor | uminus factor
```

对应到当前实现中的等价 LL(1) 形式，可写为：

```text
factor → num | string | id factor_id_tail | ( expression ) | not factor | - factor
factor_id_tail → [ expression_list ] | ( expression_list ) | ε
```

### 6.3 相关词法补充说明

以下内容不是文法产生式本身，但为了让上述文法能够正常工作，词法层也做了对应扩展：

```text
1. 新增 STRING token，对应单引号字符串字面量
2. 保留 LETTER token，对应单字符字面量，如 'A'
3. 新增 // 单行注释支持
4. 已支持的注释形式共有三种：
   { ... }
   (* ... *)
   // ...
```

### 6.4 当前代码中的实际对应关系

当前代码中与本节修改直接对应的实现位置如下：

```text
src/parser/grammar.cpp
  expression_list → ε | expression expression_list'
  factor → num | string | id factor_id_tail | ( expression ) | not factor | - factor

src/lexer/token.h
  新增 STRING token

src/lexer/lexer.cpp
  支持字符串字面量扫描
  支持 // 单行注释
```

### 6.5 本轮新增文法扩展

为支持 `correct_all.pas`，新增或补充如下文法：

```text
basic_type -> integer | real | boolean | char | string_kw

statement -> epsilon
           | id statement_id_tail
           | begin statement_list end
           | if expression then statement else_part
           | for id assignop expression to expression do statement
           | case expression of case_branch_list end
           | break
           | continue
           | read ( variable_list )
           | write ( expression_list )

case_branch_list -> case_branch case_branch_list'
case_branch_list' -> ; case_branch case_branch_list' | ; | epsilon
case_branch -> case_label_list : statement
case_label_list -> const_value case_label_list'
case_label_list' -> , const_value case_label_list' | epsilon

factor -> num | string | letter | id factor_id_tail | ( expression ) | not factor | - factor
```

说明：

```text
1. string_kw 是类型关键字 string，与字符串字面量 token string 分离
2. factor 增加 letter 后，ch := '!' 这类表达式可以正常分析
3. 新增 case / break / continue 语句支持
```

### 6.6 本轮相关词法补充

```text
1. 新增关键字：string、case、break、continue
2. 非法实数 123. 现在会在词法阶段报错，要求小数点后至少一位数字
```

### 6.7 while 语句扩展

为支持 `while <expression> do <statement>`，在 `statement` 中补充：

```text
statement -> epsilon
           | id statement_id_tail
           | begin statement_list end
           | if expression then statement else_part
           | for id assignop expression to expression do statement
           | while expression do statement
           | case expression of case_branch_list end
           | break
           | continue
           | read ( variable_list )
           | write ( expression_list )
```

对应实现说明：

```text
1. while 关键字的词法识别原本已存在
2. 本轮补充的是文法、AST、语义分析、C 代码生成和集成测试
3. 语义规则：while 条件必须为 boolean
```

### 6.8 `downto` 方向的 `for` 语句扩展

为支持 `for i := start downto finish do statement`，将原先固定递增方向的产生式扩展为：

```text
statement -> ... | for id assignop expression for_direction expression do statement | ...
for_direction -> to | downto
```

对应实现说明：

1. 词法阶段将 `downto` 识别为关键字
2. AST 中 `ForStmtNode` 通过 `descending` 标记区分 `to` 与 `downto`
3. 语义阶段要求迭代变量、起始表达式和终止表达式都为 `integer`
4. C 代码生成时：
   `to` 映射为 `for (...; i <= end; i++)`
   `downto` 映射为 `for (...; i >= end; i--)`

### 6.9 `record` 类型与字段访问扩展

为支持记录类型声明与字段访问，补充以下文法：

```text
type -> basic_type
      | array [ period ] of basic_type
      | record record_field_decl_list end

record_field_decl_list -> idlist : type ; record_field_decl_list_tail
record_field_decl_list_tail -> idlist : type ; record_field_decl_list_tail | epsilon

id_varpart -> [ expression_list ] field_chain
           | field_chain
           | epsilon

field_chain -> . id field_chain | epsilon

statement_id_tail -> ( expression_list )
                   | id_varpart assignop expression
                   | epsilon

factor_id_tail -> id_varpart
                | ( expression_list )
```

对应实现说明：

1. 记录类型在 parser 内部编码为统一的字符串表示，例如：
   `record{name:string;age:integer}`
2. 语义分析会检查：
   记录字段是否重复
   字段是否存在
   是否对非记录值进行字段访问
3. C 代码生成时，`record` 会映射为匿名 `struct`
4. 当前实现已支持：
   记录变量声明
   记录字段赋值
   在表达式、`write`、`read` 中访问字段
