# ============== Makefile ==============
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
SRC      := Lexer.cpp parser.cpp main.cpp
OBJ      := $(SRC:.cpp=.o)
TARGET   := parser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 기존 lexer 단독 실행기가 필요하면:
lex: Lexer.cpp main_lex.cpp
	$(CXX) $(CXXFLAGS) Lexer.cpp main_lex.cpp -o lexer

run: $(TARGET)
	./$(TARGET) ../tests/test1.rat25f

clean:
	rm -f $(OBJ) $(TARGET) lexer

.PHONY: all clean run lex
