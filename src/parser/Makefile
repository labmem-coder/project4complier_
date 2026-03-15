CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = pascal_parser
SRCS = main.cpp grammar.cpp parser.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp grammar.h parser.h token.h
grammar.o: grammar.cpp grammar.h
parser.o: parser.cpp parser.h grammar.h token.h

clean:
	rm -f $(OBJS) $(TARGET) *.txt

test: $(TARGET)
	@echo "=== Test 1: Valid simple program ==="
	./$(TARGET) --all ../../tests/test_simple.tok
	@echo ""
	@echo "=== Test 2: Variable declarations and expressions ==="
	./$(TARGET) ../../tests/test_vars.tok
	@echo ""
	@echo "=== Test 3: Control flow (if/for) ==="
	./$(TARGET) ../../tests/test_control.tok
	@echo ""
	@echo "=== Test 4: Subprograms ==="
	./$(TARGET) ../../tests/test_subprogram.tok
	@echo ""
	@echo "=== Test 5: Error detection ==="
	./$(TARGET) ../../tests/test_error.tok
	@echo ""
	@echo "=== Test 6: Complex program ==="
	./$(TARGET) ../../tests/test_complex.tok

.PHONY: all clean test
