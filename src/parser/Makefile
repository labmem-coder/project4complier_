CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = pascal_compiler
SRCS = main.cpp lexer.cpp grammar.cpp parser.cpp
OBJS = $(SRCS:.cpp=.o)
TEST_DIR = ../../tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp grammar.h parser.h token.h lexer.h ast.h
lexer.o: lexer.cpp lexer.h token.h
grammar.o: grammar.cpp grammar.h
parser.o: parser.cpp parser.h grammar.h token.h ast.h lexer.h

# --- Lexer test ---
test_lexer: lexer.o $(TEST_DIR)/test_lexer.cpp token.h lexer.h
	$(CXX) $(CXXFLAGS) -I. $(TEST_DIR)/test_lexer.cpp lexer.o -o test_lexer

# --- Integration test (lexer + parser) ---
test_integration: lexer.o grammar.o parser.o $(TEST_DIR)/test_integration.cpp
	$(CXX) $(CXXFLAGS) -I. $(TEST_DIR)/test_integration.cpp lexer.o grammar.o parser.o -o test_integration

clean:
	rm -f $(OBJS) $(TARGET) test_lexer test_integration *.txt *.exe

# Run lexer tests
test-lexer: test_lexer
	@echo "========== Lexer Tests =========="
	./test_lexer
	@echo ""

# Run integration tests (lexer -> parser -> AST)
test-integration: test_integration
	@echo "========== Integration Tests =========="
	./test_integration
	@echo ""

# Run all tests
test: test-lexer test-integration
	@echo "========== All tests completed =========="

# Quick smoke test with .pas files
test-pas: $(TARGET)
	@echo "=== Lex: test_simple.pas ==="
	./$(TARGET) --lex $(TEST_DIR)/pascal/test_simple.pas
	@echo ""
	@echo "=== Parse: test_simple.pas ==="
	./$(TARGET) $(TEST_DIR)/pascal/test_simple.pas
	@echo ""
	@echo "=== Parse: test_complex.pas ==="
	./$(TARGET) $(TEST_DIR)/pascal/test_complex.pas
	@echo ""

# Legacy: run with .tok files
test-tok: $(TARGET)
	@echo "=== Test (tok): test_simple.tok ==="
	./$(TARGET) $(TEST_DIR)/test_simple.tok
	@echo ""
	@echo "=== Test (tok): test_complex.tok ==="
	./$(TARGET) $(TEST_DIR)/test_complex.tok

.PHONY: all clean test test-lexer test-integration test-pas test-tok
