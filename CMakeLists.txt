cmake_minimum_required(VERSION 3.16)

project(PascalSCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(pascal_compiler
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/grammar.cpp
    src/parser/parser.cpp
    src/semantic/symbol_table.cpp
    src/semantic/semantic_analyzer.cpp
    src/codegen/codegen.cpp
)

target_include_directories(pascal_compiler PRIVATE
    src/lexer
    src/parser
    src/semantic
    src/codegen
)

set_target_properties(pascal_compiler PROPERTIES
    OUTPUT_NAME "pascal_compiler.exe"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/src"
)
