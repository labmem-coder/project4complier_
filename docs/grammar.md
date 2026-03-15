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