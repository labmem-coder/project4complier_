#set page(width: 16cm, height: 9cm, margin: 0cm)
#set text(size: 12.2pt, lang: "zh")
#set par(justify: false, leading: 0.58em)
#show raw: set text(size: 6.4pt)

#let navy = rgb("#0f1f2e")
#let ink = rgb("#243244")
#let muted = rgb("#66768a")
#let pale = rgb("#f5f7fb")
#let paper = rgb("#ffffff")
#let line = rgb("#d8e0ea")
#let blue = rgb("#2563eb")
#let teal = rgb("#0f766e")
#let green = rgb("#16a34a")
#let amber = rgb("#d97706")
#let red = rgb("#dc2626")
#let violet = rgb("#7c3aed")
#let slate = rgb("#334155")

#let footer = [Pascal-S 编译器验收展示]

#let pill(label, fill: blue) = box(
  inset: (x: 7pt, y: 3pt),
  radius: 4pt,
  fill: fill.lighten(88%),
  stroke: 0.55pt + fill.lighten(50%),
  text(size: 7.4pt, fill: fill, weight: "semibold")[#label],
)

#let card(body, title: none, fill: paper, border: line) = rect(
  width: 100%,
  radius: 5pt,
  fill: fill,
  stroke: 0.7pt + border,
  inset: 8pt,
)[
  #if title != none [
    #text(size: 9.7pt, fill: navy, weight: "bold")[#title]
    #v(4pt)
  ]
  #text(size: 8.25pt, fill: ink)[#body]
]

#let tiny-card(body, title: none, fill: paper, border: line) = rect(
  width: 100%,
  radius: 5pt,
  fill: fill,
  stroke: 0.65pt + border,
  inset: 6.6pt,
)[
  #if title != none [
    #text(size: 8.2pt, fill: navy, weight: "bold")[#title]
    #v(3pt)
  ]
  #text(size: 7.15pt, fill: ink)[#body]
]

#let code-card(body, title: none) = rect(
  width: 100%,
  radius: 5pt,
  fill: rgb("#0b1220"),
  stroke: 0.7pt + rgb("#1e293b"),
  inset: 8pt,
)[
  #if title != none [
    #text(size: 8pt, fill: rgb("#93c5fd"), weight: "bold")[#title]
    #v(4pt)
  ]
  #text(fill: rgb("#e5eefb"))[#body]
]

#let metric(num, label, fill: blue) = rect(
  radius: 5pt,
  fill: fill.lighten(90%),
  stroke: 0.7pt + fill.lighten(55%),
  inset: 8pt,
)[
  #text(size: 17pt, fill: fill, weight: "bold")[#num]
  #v(2pt)
  #text(size: 7.2pt, fill: ink)[#label]
]

#let band(title, subtitle: none, fill: blue) = rect(
  width: 100%,
  radius: 5pt,
  fill: fill.lighten(90%),
  stroke: 0.7pt + fill.lighten(50%),
  inset: 8pt,
)[
  #text(size: 10pt, weight: "bold", fill: fill)[#title]
  #if subtitle != none [
    #v(3pt)
    #text(size: 7.8pt, fill: ink)[#subtitle]
  ]
]

#let slide-frame(title, body, section: none) = [
  #rect(width: 100%, height: 100%, fill: pale)[
    #place(top + left, rect(width: 100%, height: 6pt, fill: blue))
    #pad(x: 31pt, y: 23pt)[
      #if section != none [
        #text(size: 7.4pt, fill: blue, weight: "bold")[#section]
        #v(3pt)
      ]
      #text(size: 20.6pt, fill: navy, weight: "bold")[#title]
      #v(10pt)
      #body
    ]
    #place(bottom + left, dx: 31pt, dy: -15pt, text(size: 6.8pt, fill: muted)[#footer])
    #place(bottom + right, dx: -31pt, dy: -15pt, text(size: 6.8pt, fill: muted)[2026 · Compiler Project])
  ]
]

#let slide(title, body, section: none) = [
  #slide-frame(title, body, section: section)
  #pagebreak()
]

#let final-slide(title, body, section: none) = [
  #slide-frame(title, body, section: section)
]

#let section-slide(kicker, title, subtitle, fill: blue) = [
  #rect(width: 100%, height: 100%, fill: fill.darken(12%))[
    #place(top + left, rect(width: 100%, height: 7pt, fill: fill.lighten(22%)))
    #pad(x: 45pt, y: 45pt)[
      #text(size: 8.5pt, fill: fill.lighten(58%), weight: "bold")[#kicker]
      #v(18pt)
      #text(size: 29pt, fill: white, weight: "bold")[#title]
      #v(12pt)
      #text(size: 12pt, fill: fill.lighten(70%))[#subtitle]
    ]
    #place(bottom + left, dx: 45pt, dy: -32pt, text(size: 7.5pt, fill: fill.lighten(65%))[#footer])
  ]
  #pagebreak()
]

#let title-slide() = [
  #rect(width: 100%, height: 100%, fill: navy)[
    #place(top + left, rect(width: 100%, height: 7pt, fill: blue))
    #pad(x: 46pt, y: 40pt)[
      #text(size: 8.8pt, fill: blue.lighten(40%), weight: "bold")[编译原理课程项目验收]
      #v(18pt)
      #text(size: 31pt, fill: white, weight: "bold")[Pascal-S 编译器]
      #v(8pt)
      #text(size: 14pt, fill: rgb("#cbd5e1"))[从词法分析到语义检查与 C 代码生成的完整流水线]
      #v(20pt)
      #grid(
        columns: (auto, auto, auto, auto, auto),
        gutter: 7pt,
        pill(fill: blue)[Lexer],
        pill(fill: teal)[LL(1)],
        pill(fill: violet)[AST],
        pill(fill: green)[Semantic],
        pill(fill: amber)[C Codegen],
      )
    ]
    #place(bottom + left, dx: 46pt, dy: -35pt, text(size: 8.2pt, fill: rgb("#94a3b8"))[源码入口：`src/main.cpp` · 幻灯片：`docs/slides.typ`])
  ]
  #pagebreak()
]

#title-slide()

#slide(
  [验收讲解路线],
  section: [00 AGENDA],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 8pt,
    tiny-card(title: [1. 结构与流程], fill: rgb("#eff6ff"))[
      项目目录、主流程、输入输出文件、演示命令。
    ],
    tiny-card(title: [2. 符号表], fill: rgb("#f0fdfa"))[
      表项结构、作用域栈、查找、插入、定位与重定位。
    ],
    tiny-card(title: [3. 词法分析], fill: rgb("#fff7ed"))[
      token 翻译表、注释处理、错误检测、与 Parser 接口。
    ],
    tiny-card(title: [4. 语法分析], fill: rgb("#f5f3ff"))[
      LL(1) 实现、FIRST/FOLLOW、预测表、AST 构建、语法错误。
    ],
    tiny-card(title: [5. 语义分析], fill: rgb("#ecfdf5"))[
      支持类型、表达式/数组/参数/调用/语句检查。
    ],
    tiny-card(title: [6. 代码生成与测试], fill: rgb("#fef2f2"))[
      C 代码生成策略、错误恢复、测试用例编译过程与结果。
    ],
  )
  #v(9pt)
  #band(
    [答辩讲法],
    subtitle: [每位成员围绕自己负责的模块讲：目标 → 数据结构 / 算法 → 关键代码入口 → 测试样例。],
    fill: slate,
  )
]

#slide(
  [项目总体结构],
  section: [01 OVERVIEW],
)[
  #grid(
    columns: (1.03fr, 0.97fr),
    gutter: 11pt,
    card(title: [目录组织])[
      - `src/lexer`：Token、扫描器、词法错误
      - `src/parser`：文法、LL(1) 预测分析、AST
      - `src/semantic`：符号表与语义检查
      - `src/codegen`：Pascal-S 到 C 的生成器
      - `tests`：正确样例、词法错误、语法错误、语义错误、集成测试
    ],
    card(title: [主程序入口])[
      `src/main.cpp` 负责命令行解析，并按阶段调度：
      - 构造文法并转换为 LL(1)
      - 词法 + 语法分析并构建 AST
      - 语义分析通过后生成 C
      - 输出 `tokens.txt`、`parse_process.txt`、`ast.txt`、目标 `.c`
    ],
  )
  #v(9pt)
  #grid(
    columns: (1fr, 1fr, 1fr, 1fr),
    gutter: 7pt,
    metric(fill: blue)[`Lexer`][按需扫描 token],
    metric(fill: violet)[`Parser`][表驱动 LL(1)],
    metric(fill: green)[`ASTVisitor`][语义 / 生成复用],
    metric(fill: amber)[`C`][可继续由 gcc 编译],
  )
]

#slide(
  [完整编译流水线],
  section: [01 OVERVIEW],
)[
  #grid(
    columns: (1fr, 1fr, 1fr, 1fr, 1fr),
    gutter: 6pt,
    tiny-card(title: [1 源程序], fill: rgb("#eff6ff"))[
      `.pas` 文件；也保留旧 token 文件兼容入口。
    ],
    tiny-card(title: [2 词法], fill: rgb("#eefcf8"))[
      `Lexer::nextToken()` 跳过空白/注释并返回带行列号的 token。
    ],
    tiny-card(title: [3 语法], fill: rgb("#f5f3ff"))[
      `Parser::parse()` 根据预测分析表进行 `PREDICT / MATCH / REDUCE`。
    ],
    tiny-card(title: [4 语义], fill: rgb("#ecfdf5"))[
      `SemanticAnalyzer::analyze()` 遍历 AST，构建符号表并检查类型。
    ],
    tiny-card(title: [5 生成], fill: rgb("#fff7ed"))[
      `CCodeGenerator::generate()` 输出可编译 C 源码。
    ],
  )
  #v(10pt)
  #code-card(title: [运行示例])[
```bash
src/pascal_compiler.exe -i tests/correct_test/correct_all.pas -o /private/tmp/correct_all_demo.c
```
  ]
  #v(7pt)
  #band(
    [验收输出],
    subtitle: [`*** PARSE SUCCESSFUL ***` → `*** SEMANTIC ANALYSIS PASSED ***` → `*** C CODE GENERATED: ... ***`],
    fill: green,
  )
]

#slide(
  [讲解分工建议],
  section: [01 OVERVIEW],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    card(title: [成员 A：词法与接口], fill: rgb("#eff6ff"))[
      - 讲 `token.h` 的 TokenType / 翻译表
      - 讲 `lexer.cpp` 的注释跳过、数字/字符串/标识符扫描
      - 演示词法错误与 token 输出
    ],
    card(title: [成员 B：语法与 AST], fill: rgb("#f5f3ff"))[
      - 讲 `grammar.cpp` 的文法改写、FIRST/FOLLOW、预测表
      - 讲 `parser.cpp` 的分析栈、语义栈、规约动作
      - 演示 `parse_process.txt` 与 `ast.txt`
    ],
    card(title: [成员 C：符号表与语义], fill: rgb("#ecfdf5"))[
      - 讲 `symbol_table.*` 的作用域栈与表项结构
      - 讲 `semantic_analyzer.cpp` 的类型推断和错误检查
      - 演示语义错误样例
    ],
    card(title: [成员 D：代码生成与测试], fill: rgb("#fff7ed"))[
      - 讲 `codegen.cpp` 的 Visitor 输出策略
      - 讲 Pascal 类型、数组、record、调用到 C 的映射
      - 演示生成 C 文件和回归测试
    ],
  )
  #v(8pt)
  #text(size: 7.5pt, fill: muted)[把“成员 A/B/C/D”替换成实际姓名即可；人数更多时按模块继续拆分。]
]

#section-slide([02 SYMBOL TABLE], [符号表], [内容、结构，以及查找 / 插入 / 定位 / 重定位操作], fill: teal)

#slide(
  [符号表内容与表项结构],
  section: [02 SYMBOL TABLE],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [`TypeInfo`：类型描述])[
      - `Category`：`Simple`、`Array`、`Record`
      - 简单类型：`integer`、`real`、`boolean`、`char`、`string`
      - 数组：`arrayLow`、`arrayHigh`、`elementType`
      - 记录：`recordFields = [(fieldName, TypeInfo)]`
      - 提供 `fromString()` / `toString()` / `findField()`
    ],
    card(title: [`Symbol`：符号表项])[
      - `name`：标识符名，词法层已统一为小写
      - `kind`：常量、变量、参数、函数、过程、程序
      - `type`：完整 TypeInfo
      - `scopeLevel`：声明所在层级
      - `params` / `returnType`：函数过程签名
      - `constValue`、`byReference`：常量值与 var 参数
    ],
  )
  #v(9pt)
  #code-card(title: [核心文件])[
```text
src/semantic/symbol_table.h
src/semantic/symbol_table.cpp
src/semantic/semantic_analyzer.cpp
```
  ]
]

#slide(
  [符号表的组织方式],
  section: [02 SYMBOL TABLE],
)[
  #grid(
    columns: (0.9fr, 1.1fr),
    gutter: 12pt,
    card(title: [作用域栈])[
      `SymbolTable` 内部是：
```cpp
vector<unordered_map<string, Symbol>> scopes_;
```
      每个 `unordered_map` 表示一个作用域；栈顶是当前作用域。
    ],
    card(title: [为什么这样设计])[
      - 插入只发生在当前作用域
      - 查找从栈顶向外层扫描，天然支持遮蔽
      - 进入函数/过程时 `enterScope()`
      - 离开函数/过程时 `exitScope()`
      - 当前层级由 `getCurrentLevel()` 统一计算
    ],
  )
  #v(8pt)
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [全局作用域], fill: rgb("#eef2ff"))[
      program 参数、全局 const / var、子程序名。
    ],
    tiny-card(title: [子程序作用域], fill: rgb("#f0fdfa"))[
      参数、局部变量、函数返回值变量。
    ],
    tiny-card(title: [临时定位信息], fill: rgb("#fff7ed"))[
      AST 节点保存 line / column，错误信息可回指源码位置。
    ],
  )
]

#slide(
  [符号表操作：查找与插入],
  section: [02 SYMBOL TABLE],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [插入 declare])[
      - 只向 `scopes_.back()` 插入
      - 若当前作用域已有同名符号，返回 `false`
      - 插入前补上 `scopeLevel`
      - 用于检测重复常量、变量、参数、子程序声明
    ],
    card(title: [查找 lookup])[
      - 从最内层作用域向外层作用域扫描
      - 命中第一个符号即返回
      - `lookupCurrent()` 只查当前层
      - `lookupCallable()` 只接受 Function / Procedure
      - `lookupFunction()` 用于函数表达式
    ],
  )
  #v(9pt)
  #code-card(title: [讲代码时可指向这几个函数])[
```cpp
bool SymbolTable::declare(const Symbol& sym);
Symbol* SymbolTable::lookup(const string& name);
Symbol* SymbolTable::lookupCurrent(const string& name);
Symbol* SymbolTable::lookupCallable(const string& name);
```
  ]
]

#slide(
  [符号表操作：定位与重定位],
  section: [02 SYMBOL TABLE],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [定位])[
      - `enterScope()` 创建新的当前作用域
      - `scopeLevel` 标记声明层级
      - 使用 `lookup()` 将变量引用定位到最近的可见声明
      - 函数内部把函数名声明为局部返回值变量
      - `line/column` 让错误定位到 AST 节点来源
    ],
    card(title: [重定位])[
      - `exitScope()` 弹出当前作用域
      - 同名标识符自动恢复到外层绑定
      - 这相当于离开作用域后的符号重定位
      - 代码生成阶段进一步做地址级调整：
        数组下界偏移、`var` 参数取地址/解引用
    ],
  )
  #v(8pt)
  #band(
    [示例],
    subtitle: [`array[1..5] of integer` 在 C 中声明为长度 5，访问 `a[i]` 时生成 `a[i - 1]`；`var x` 形参生成 `int *x`，读写时使用 `*x`。],
    fill: teal,
  )
]

#section-slide([03 LEXER], [词法分析], [翻译表、注释处理、错误检测，以及与语法分析器的接口], fill: blue)

#slide(
  [词法翻译表],
  section: [03 LEXER],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [`TokenType` 到文法终结符])[
      `token.h` 中的 `tokenTypeToTerminal()` 把枚举映射到 LL(1) 文法终结符：
      - `PROGRAM` → `program`
      - `ASSIGN` → `assignop`
      - `NUM` → `num`
      - `STRING_KW` → `string_kw`
      - `END_OF_FILE` → `$`
    ],
    card(title: [关键字表])[
      `Lexer::keywordMap()` 统一维护关键字：
      - 控制流：`if`、`for`、`while`、`case`
      - 类型：`integer`、`real`、`boolean`、`char`、`string`
      - 扩展：`downto`、`break`、`continue`、`record`
      - 运算：`div`、`mod`、`and`、`or`、`not`
    ],
  )
  #v(8pt)
  #band([大小写处理], subtitle: [扫描标识符后统一转小写，因此关键字和用户标识符都按 Pascal-S 的大小写不敏感规则处理。], fill: blue)
]

#slide(
  [词法扫描流程],
  section: [03 LEXER],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [1. 预处理], fill: rgb("#eff6ff"))[
      `skipWhitespaceAndComments()` 跳过空白和注释。
    ],
    tiny-card(title: [2. 分类扫描], fill: rgb("#f0fdfa"))[
      数字、标识符/关键字、字符/字符串、运算符和界符。
    ],
    tiny-card(title: [3. 产出 token], fill: rgb("#fff7ed"))[
      每个 token 都携带 `lexeme`、`line`、`column`。
    ],
  )
  #v(10pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [注释处理])[
      支持三类注释：
      - `{ ... }`
      - `(* ... *)`
      - `// ...`

      未闭合注释在词法阶段报告错误。
    ],
    card(title: [错误检测])[
      - 标识符不能以数字开头
      - 实数小数点后必须有数字
      - 多个小数点非法
      - 非 ASCII 字符非法
      - 括号/方括号不匹配或未闭合
      - 字符串/字符字面量未闭合
    ],
  )
]

#slide(
  [词法与语法分析器接口],
  section: [03 LEXER],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [按需扫描])[
      Parser 持有非拥有指针 `Lexer* lexer`，构造时读取第一个 lookahead：
```cpp
currentTok = lexer->nextToken();
```
      匹配终结符成功后调用 `advance()`，再取下一个 token。
    ],
    card(title: [接口收益])[
      - 不需要先生成完整 token 序列
      - Parser 可同步收集已消费 token
      - 词法错误保留在 `lexer.getErrors()`
      - verbose 模式写出 `tokens.txt`
    ],
  )
  #v(8pt)
  #code-card(title: [词法错误演示])[
```text
*** ERRORS: LEXER (1) ***
  [Lexer][Line 4, Col 8] Invalid number format '1.2.3': multiple decimal points
```
  ]
]

#section-slide([04 PARSER], [语法分析], [LL(1) 表驱动实现、AST 构建、可检测语法错误和输出], fill: violet)

#slide(
  [语法分析实现方法],
  section: [04 PARSER],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [文法准备])[
      `Grammar` 保存产生式：
```cpp
struct Production { string lhs; vector<string> rhs; };
```
      编译启动时执行：
      - 提取左公因子
      - 消除左递归
      - 计算 FIRST / FOLLOW
      - 构造 LL(1) 预测分析表
    ],
    card(title: [表驱动分析])[
      `Parser::parse()` 使用分析栈：
      - 栈顶是终结符：尝试 `MATCH`
      - 栈顶是非终结符：查表 `PREDICT`
      - 栈顶是动作标记：执行 `REDUCE`
      - 输入结束且栈顶 `$`：`ACCEPT`
    ],
  )
  #v(8pt)
  #band([关键输出], subtitle: [`--grammar`、`--first-follow`、`--table` 可分别打印转换后文法、FIRST/FOLLOW 集和预测分析表。], fill: violet)
]

#slide(
  [AST 构建机制],
  section: [04 PARSER],
)[
  #grid(
    columns: (0.96fr, 1.04fr),
    gutter: 11pt,
    card(title: [不是事后补树])[
      AST 构建嵌入 LL(1) 分析：
      - 终结符匹配后把 token 压入语义栈
      - 产生式右部完成后触发动作标记
      - 从语义栈取出右部值并规约
      - 最终 `programstruct` 规约成 AST 根
    ],
    card(title: [节点覆盖])[
      - 声明：const、var、parameter、subprogram
      - 语句：assign、call、if、for、while、case、break、continue、read、write
      - 表达式：literal、variable、unary、binary、call
      - 复合结构：array index、record field、函数/过程实参
    ],
  )
  #v(8pt)
  #code-card(title: [语义动作注册表])[
```cpp
unordered_map<string, vector<SemanticAction>> actionTable;
registerAction(lhs, handler);
buildSemanticValue(production, rhsValues);
```
  ]
]

#slide(
  [能检测哪些语法错误],
  section: [04 PARSER],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [程序结构错误], fill: rgb("#f5f3ff"))[
      缺少 `program`、缺少 `.`、缺少 `end`、多余 token。
    ],
    tiny-card(title: [语句结构错误], fill: rgb("#fff7ed"))[
      缺少 `then`、缺少 `do`、不完整赋值、非法语句开头。
    ],
    tiny-card(title: [表达式/类型声明错误], fill: rgb("#fef2f2"))[
      表达式不完整、括号不闭合、数组/类型声明不完整。
    ],
  )
  #v(9pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [错误输出])[
      每条错误包含：
      - 阶段：`Parser`
      - 源码位置：Line / Col
      - 当前 token
      - 当前非终结符的期望集合
    ],
    card(title: [恢复策略])[
      采用 panic mode：
      - 若当前 token 在 FOLLOW 集中，弹出当前非终结符
      - 否则跳过输入，直到遇到 FIRST 或 FOLLOW 中的同步 token
      - 尽量继续报告后续错误
    ],
  )
]

#slide(
  [语法错误输出示例],
  section: [04 PARSER],
)[
  #code-card(title: [`tests/error_test_bison/missing_then_syntax.pas`])[
```text
*** ERRORS: PARSER (3) ***
  [Parser][Line 6, Col 5] Syntax error at 'write':
    expected {')', ',', ';', ']', 'addop', 'do', 'downto',
              'else', 'end', 'mulop', 'of', 'relop', 'then', 'to'} for 'term''
  [Parser][Line 6, Col 12] Expected 'then', got ')'
  [Parser][Line 6, Col 12] Syntax error at ')':
    expected {';', 'begin', 'break', 'case', 'continue', 'else', 'end',
              'for', 'id', 'if', 'read', 'while', 'write'} for 'statement'
```
  ]
  #v(8pt)
  #band(
    [讲解重点],
    subtitle: [错误恢复后不立即停止，能继续暴露同一源码附近的多个语法问题；这比只报第一个错误更利于调试。],
    fill: red,
  )
]

#section-slide([05 SEMANTIC], [语义分析], [实现方法、支持类型、类型检查范围], fill: green)

#slide(
  [语义分析实现方法],
  section: [05 SEMANTIC],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [Visitor 遍历 AST])[
      `SemanticAnalyzer : ASTVisitor`
      - `analyze(ast)` 清空错误并访问根节点
      - 声明节点负责插入符号表
      - 表达式节点负责类型推断和操作数检查
      - 语句节点负责上下文规则检查
    ],
    card(title: [状态信息])[
      - `symTable_`：作用域栈
      - `currentFunction_`：当前函数名
      - `functionAssignedStack_`：函数是否给返回值赋值
      - `loopDepth_`：break / continue 上下文
      - `caseDepth_`：case 中允许 break
    ],
  )
  #v(8pt)
  #code-card(title: [入口函数])[
```cpp
bool SemanticAnalyzer::analyze(ProgramNodePtr ast);
TypeInfo SemanticAnalyzer::inferType(ExprNode* expr);
bool SemanticAnalyzer::typesCompatible(const TypeInfo& lhs, const TypeInfo& rhs) const;
```
  ]
]

#slide(
  [支持的类型系统],
  section: [05 SEMANTIC],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [简单类型], fill: rgb("#ecfdf5"))[
      `integer`、`real`、`boolean`、`char`、`string`
    ],
    tiny-card(title: [数组类型], fill: rgb("#eff6ff"))[
      `array[low..high] of basic_type`，支持多维范围编码。
    ],
    tiny-card(title: [记录类型], fill: rgb("#fff7ed"))[
      `record ... end` 编码为 `record{field:type;...}`。
    ],
  )
  #v(10pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [类型推断])[
      - 数字带小数点推断为 `real`
      - `+ - *` 根据操作数提升到 `real`
      - `/` 结果为 `real`
      - `div` / `mod` 结果为 `integer`
      - 关系运算结果为 `boolean`
      - 函数调用结果来自函数返回类型
    ],
    card(title: [赋值兼容])[
      - 同类型可赋值
      - `integer` 可提升到 `real`
      - 数组整体不可赋值
      - 其他跨类型赋值报错
      - record 字段访问后按字段 TypeInfo 检查
    ],
  )
]

#slide(
  [语义检查范围],
  section: [05 SEMANTIC],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    card(title: [声明与作用域])[
      - 重复常量 / 变量 / 参数 / 子程序声明
      - 未声明标识符、未声明函数、未声明过程
      - 函数返回标识符与参数冲突
      - 函数缺少返回值赋值
      - 常量被赋值
    ],
    card(title: [表达式与运算])[
      - 算术运算要求数值类型
      - `div` / `mod` 要求整数
      - `and` / `or` 要求布尔
      - 一元 `+` / `-` 要求数值
      - 关系运算要求两侧兼容
    ],
    card(title: [数组与 record])[
      - 数组下标必须为整数
      - 常量下标越界直接报错
      - 非数组不能使用下标
      - 记录字段不能重复
      - 非 record 不能访问字段
      - 不存在的字段报错
    ],
    card(title: [参数、调用、语句])[
      - 参数个数匹配
      - 参数类型匹配
      - `var` 参数必须传变量
      - `for` 变量/起止表达式必须为整数
      - `while` 条件必须为布尔
      - `break` / `continue` 上下文检查
      - case 标签类型和重复标签检查
    ],
  )
]

#slide(
  [语义错误输出示例],
  section: [05 SEMANTIC],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    code-card(title: [参数个数错误])[
```text
*** PARSE SUCCESSFUL ***

*** ERRORS: SEMANTIC (1) ***
  [Semantic][Line 9, Col 9]
  Function 'add' expects 2 argument(s), got 1

Semantic analysis failed.
Code generation skipped.
```
    ],
    card(title: [为什么先输出 Parse Successful])[
      语法正确不代表程序可编译；主流程先完成 LL(1) 语法分析并得到 AST，然后再进入语义阶段。

      若语义阶段失败，主程序明确跳过代码生成，避免产生错误目标代码。
    ],
  )
]

#section-slide([06 CODEGEN], [代码生成], [输入、采用技术、映射规则与算法思想], fill: amber)

#slide(
  [代码生成输入与技术],
  section: [06 CODEGEN],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [输入])[
      - 已通过语义分析的 AST
      - 代码生成器内部维护一份符号表
      - 类型信息来自声明节点和 `TypeInfo::fromString`
      - 表达式生成时复用类型推断来选择格式符
    ],
    card(title: [采用技术])[
      `CCodeGenerator : ASTVisitor`
      - 遍历 AST 节点并输出 C 代码
      - `ostringstream out_` 累积结果
      - `indent_` 管理缩进
      - helper 函数处理类型映射、变量访问、表达式输出
    ],
  )
  #v(8pt)
  #code-card(title: [入口函数])[
```cpp
string CCodeGenerator::generate(ProgramNodePtr ast);
void CCodeGenerator::visitProgram(ProgramNode& node);
void CCodeGenerator::emitExpr(ExprNode* expr);
```
  ]
]

#slide(
  [代码生成算法思想],
  section: [06 CODEGEN],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [1. 注册声明], fill: rgb("#fff7ed"))[
      遍历 const / var / subprogram 时同步登记到生成器符号表。
    ],
    tiny-card(title: [2. 生成结构], fill: rgb("#eff6ff"))[
      全局声明和子程序先输出，再输出 `int main()`。
    ],
    tiny-card(title: [3. 递归表达式], fill: rgb("#f0fdfa"))[
      表达式节点递归访问，运算符转换成 C 语义。
    ],
  )
  #v(9pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [类型映射])[
      - `integer` → `int`
      - `real` → `float`
      - `boolean` → `int`
      - `char` → `char`
      - `string` → `const char*`
      - `record` → 匿名 `struct`
    ],
    card(title: [运算与语句映射])[
      - `and` / `or` → `&&` / `||`
      - `not boolean` → `!`
      - `div` / `mod` → `/` / `%`
      - `for to` → `i <= end; i++`
      - `for downto` → `i >= end; i--`
      - `case` → `switch`
    ],
  )
]

#slide(
  [数组、参数与 I/O 生成],
  section: [06 CODEGEN],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [数组下界重定位])[
      Pascal 数组可从任意下界开始；C 数组从 0 开始。
```cpp
array[1..5] of integer
```
      声明为长度 5；访问时生成：
```cpp
a[i - 1]
```
    ],
    card(title: [`var` 参数])[
      Pascal 的引用参数在 C 中转为指针：
      - 形参：`int *x`
      - 调用：`&a`
      - 使用：`(*x)`
    ],
    card(title: [read / write])[
      `read` / `write` / `readln` / `writeln` 映射为：
      - `scanf`
      - `printf`

      根据表达式类型选择 `%d`、`%f`、`%c`、`%s`。
    ],
    card(title: [函数返回值])[
      函数体内把函数名、`result` 或 `<name>_result` 看作返回值存储；离开函数前生成：
```cpp
return <result_storage>;
```
    ],
  )
]

#slide(
  [代码生成示例],
  section: [06 CODEGEN],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    code-card(title: [Pascal-S])[
```pascal
for i := 5 downto 1 do
begin
  write(i);
end;
```
    ],
    code-card(title: [生成 C])[
```c
for (i = 5; i >= 1; i--) {
    printf("%d", i);
}
```
    ],
  )
  #v(8pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    code-card(title: [record 字段访问])[
```pascal
person.age := 20;
write(person.age);
```
    ],
    code-card(title: [生成 C])[
```c
person.age = 20;
printf("%d", person.age);
```
    ],
  )
]

#section-slide([07 DIAGNOSTICS], [错误处理与恢复策略], [词法、语法、语义三阶段分层报告], fill: red)

#slide(
  [错误处理总览],
  section: [07 DIAGNOSTICS],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [词法错误], fill: rgb("#fef2f2"))[
      记录在 `Lexer::errors`；扫描尽量继续，最终输出 token 和错误列表。
    ],
    tiny-card(title: [语法错误], fill: rgb("#fff7ed"))[
      Parser 输出期望集合，并利用 FIRST/FOLLOW 做 panic mode 恢复。
    ],
    tiny-card(title: [语义错误], fill: rgb("#ecfdf5"))[
      AST 遍历阶段集中收集；失败时停止代码生成。
    ],
  )
  #v(10pt)
  #card(title: [统一格式])[
```text
*** ERRORS: <PHASE> (N) ***
  [<Phase>][Line x, Col y] <message>
```
    这样验收时可以清楚说明：错误来自哪一阶段、定位到哪里、为什么错。
  ]
]

#slide(
  [恢复策略细节],
  section: [07 DIAGNOSTICS],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [Lexer 层])[
      - 遇到非法字符或非法字面量时记录错误
      - 返回哨兵 token 后继续尝试扫描
      - EOF 前统一检查未闭合括号
      - 未闭合注释/字符串会保留起始位置
    ],
    card(title: [Parser 层])[
      - 终结符不匹配时报告 expected / got
      - 预测表无项时列出当前非终结符可接受 token
      - 若当前 token 属于 FOLLOW，弹出非终结符
      - 否则跳过输入直到 FIRST/FOLLOW 同步点
    ],
  )
  #v(8pt)
  #band([Semantic 层], subtitle: [语义阶段不做“猜测修复”，只收集所有可确定的错误；一旦存在错误，主流程输出 `Code generation skipped`。], fill: red)
]

#section-slide([08 DEMO], [运行程序演示], [测试用例的编译过程和结果], fill: slate)

#slide(
  [演示用例安排],
  section: [08 DEMO],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    card(title: [成功样例：完整流水线], fill: rgb("#ecfdf5"))[
```bash
src/pascal_compiler.exe -i tests/correct_test/correct_all.pas -o /private/tmp/correct_all_demo.c
```
      展示 parse、semantic、C codegen 全部通过。
    ],
    card(title: [词法错误样例], fill: rgb("#fef2f2"))[
```bash
src/pascal_compiler.exe --lex tests/error_test_lex/invalid_number_3.pas
```
      展示非法数字 `1.2.3` 的定位与错误信息。
    ],
    card(title: [语法错误样例], fill: rgb("#fff7ed"))[
```bash
src/pascal_compiler.exe tests/error_test_bison/missing_then_syntax.pas
```
      展示缺少 `then` 后的预测集合与恢复。
    ],
    card(title: [语义错误样例], fill: rgb("#eff6ff"))[
```bash
src/pascal_compiler.exe tests/semantic_error_test/argument_count_mismatch.pas
```
      展示函数参数数量不匹配，以及跳过代码生成。
    ],
  )
]

#slide(
  [成功编译结果],
  section: [08 DEMO],
)[
  #code-card(title: [运行输出])[
```text
*** PARSE SUCCESSFUL ***

*** SEMANTIC ANALYSIS PASSED ***

*** C CODE GENERATED: /private/tmp/correct_all_demo.c ***
```
  ]
  #v(9pt)
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    tiny-card(title: [覆盖语法], fill: rgb("#ecfdf5"))[
      `string`、`char`、函数、`case`、`break`、`continue`、`for`。
    ],
    tiny-card(title: [输出产物], fill: rgb("#eff6ff"))[
      指定 `-o` 后输出 C 源文件；`--all` 模式还会输出 token、AST、分析过程。
    ],
    tiny-card(title: [继续验证], fill: rgb("#fff7ed"))[
      可用 `gcc` 编译生成的 `.c`，验证运行时结果。
    ],
  )
]

#slide(
  [测试覆盖与当前状态],
  section: [08 DEMO],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 7pt,
    metric(fill: green)[`correct_test`][正确程序完整流水线成功],
    metric(fill: red)[`error_test_lex`][非法字符、数字、注释、括号等],
    metric(fill: amber)[`error_test_bison`][缺少 then/do/end/dot 等语法错误],
  )
  #v(8pt)
  #grid(
    columns: (1fr, 1fr),
    gutter: 11pt,
    card(title: [语义错误语料])[
      覆盖重复声明、未声明函数/过程、参数个数/类型、数组下标、for/while/case、break/continue 上下文、record 字段访问等。
    ],
    card(title: [集成测试])[
      `tests/test_integration.cpp` 覆盖 lexer、parser、AST、semantic、symbol table、codegen 的统一行为。

      答辩前建议运行：
```bash
bash tests/run_tests.sh
```
    ],
  )
  #v(6pt)
  #text(size: 7.2pt, fill: muted)[注：若修复或调整语义规则，请在本页替换为最新测试统计。]
]

#slide(
  [源码讲解索引],
  section: [09 CODE WALKTHROUGH],
)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 10pt,
    card(title: [词法])[
      - `Lexer::keywordMap()`
      - `Lexer::skipWhitespaceAndComments()`
      - `Lexer::scanNumber()`
      - `Lexer::scanIdentifierOrKeyword()`
      - `Lexer::scanCharOrStringLiteral()`
      - `tokenTypeToTerminal()`
    ],
    card(title: [语法 / AST])[
      - `buildPascalSGrammar()`
      - `Grammar::transformToLL1()`
      - `Grammar::computeFirstSets()`
      - `Grammar::buildParseTable()`
      - `Parser::parse()`
      - `Parser::registerDefaultActions()`
    ],
    card(title: [语义 / 符号表])[
      - `SymbolTable::declare()`
      - `SymbolTable::lookup()`
      - `TypeInfo::fromString()`
      - `SemanticAnalyzer::visitVarDecl()`
      - `SemanticAnalyzer::inferType()`
      - `SemanticAnalyzer::visitCallExpr()`
    ],
    card(title: [代码生成])[
      - `CCodeGenerator::generate()`
      - `CCodeGenerator::visitProgram()`
      - `CCodeGenerator::visitSubprogramDecl()`
      - `CCodeGenerator::emitVarAccess()`
      - `CCodeGenerator::visitBinaryExpr()`
      - `CCodeGenerator::visitReadStmt()` / `visitWriteStmt()`
    ],
  )
]

#final-slide(
  [总结],
  section: [10 SUMMARY],
)[
  #grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 8pt,
    card(title: [完整流水线], fill: rgb("#eff6ff"))[
      从源程序读取、词法、语法、AST、语义检查到 C 代码生成，主流程可端到端运行。
    ],
    card(title: [可解释实现], fill: rgb("#f0fdfa"))[
      通过 token、parse process、AST、错误阶段前缀和源码位置，便于验收现场讲清楚每一步。
    ],
    card(title: [可扩展结构], fill: rgb("#fff7ed"))[
      文法、AST、语义分析和代码生成都按模块拆分；新增语言结构时可以沿同一链路扩展。
    ],
  )
  #v(20pt)
  #align(center)[
    #text(size: 20pt, weight: "bold", fill: navy)[Pascal-S 编译器项目展示]
    #v(6pt)
    #text(size: 10pt, fill: muted)[符号表 · 词法 · 语法 · 语义 · 代码生成 · 错误恢复 · 测试演示]
  ]
]
