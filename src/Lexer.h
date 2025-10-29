// Lexer.h
#pragma once
#include <istream>
#include <string>
#include "Token.h"

class Lexer {
public:
    explicit Lexer(std::istream& input);
    Token nextToken();

private:
    std::istream& in;
    char   current{};
    bool   eof = false;
    size_t line = 1, col = 0;

    void advance();
    char peek() const { return static_cast<char>(in.peek()); }

    // identifier helpers (first must be letter; rest can be letter/digit/$)
    static bool isLetter(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }
    // add '_' support
    static bool isIdentRest(char c) {
        return isLetter(c) || std::isdigit(static_cast<unsigned char>(c)) || c == '$' || c == '_';
    }

    // declare a string scanner
    void skipSpace();

    // FSMs
    Token scanIdentifierFSM();     // [A-Za-z][A-Za-z0-9$]*
    Token scanNumberFSM();         // Integer [0-9]+  or Real [0-9]+.[0-9]+
    Token scanRealStartingWithDot(); // .[0-9]+
    Token scanString();   // " ... "


    // operators / separators
    bool  isSeparator(char c) const;
    bool  isOperatorStart(char c) const;
    Token scanOpOrSep();
};
