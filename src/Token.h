#pragma once
#include <string>

enum class TokenType {
    Keyword,
    Identifier,
    Integer,
    Real,
    Operator,
    Separator,
    String,
    Unknown,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    size_t line;
    size_t col;
};
