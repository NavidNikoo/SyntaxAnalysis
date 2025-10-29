#include "Lexer.h"
#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <vector>

Lexer::Lexer(std::istream& input) : in(input) { advance(); }

void Lexer::advance() {
    int ch = in.get();
    if (ch == EOF) { eof = true; current = '\0'; return; }
    current = static_cast<char>(ch);
    if (current == '\n') { line++; col = 0; } else { col++; }
}

void Lexer::skipSpace() {
    while (!eof && std::isspace(static_cast<unsigned char>(current))) {
        advance();
    }
}


Token Lexer::scanString() {
    // current == '"'
    std::string lex;
    advance(); // skip opening quote
    while (!eof && current != '"') {
        lex.push_back(current);
        advance();
    }
    if (!eof) advance(); // skip closing quote
    return { TokenType::String, lex, line, col };
}


// --------- keyword table (compare case-insensitively) ----------
static bool isKeyword(const std::string& s) {
    static const std::unordered_set<std::string> K = {
        // include both per class notes and what you used in tests
        "integer","int","real","if","else","fi","while","return","get","put"
    };
    std::string k = s;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c){ return std::tolower(c); });
    return K.count(k) != 0;
}

// ----------------------- FSM: Identifier -----------------------
Token Lexer::scanIdentifierFSM() {
    std::string lex;
    // first must be letter
    lex.push_back(current);
    advance();
    // rest: letters/digits/$
    while (!eof && isIdentRest(current)) { lex.push_back(current); advance(); }
    if (isKeyword(lex)) return { TokenType::Keyword, lex, line, col };
    return { TokenType::Identifier, lex, line, col };
}

// ------------------------ FSM: Numbers -------------------------
// Integer [0-9]+, Real [0-9]+.[0-9]+, and also .[0-9]+ (teacher allows)
Token Lexer::scanNumberFSM() {
    std::string lex;
    // consume 1+ digits
    while (!eof && std::isdigit(static_cast<unsigned char>(current))) {
        lex.push_back(current);
        advance();
    }
    // look for '.' followed by 1+ digits
    if (!eof && current == '.' && std::isdigit(static_cast<unsigned char>(peek()))) {
        lex.push_back(current);  // consume '.'
        advance();
        while (!eof && std::isdigit(static_cast<unsigned char>(current))) {
            lex.push_back(current);
            advance();
        }
        return { TokenType::Real, lex, line, col };
    }
    // if '.' not followed by a digit, DO NOT eat it (123. is not a real per notes)
    return { TokenType::Integer, lex, line, col };
}

Token Lexer::scanRealStartingWithDot() {
    // we are at '.' and peek is a digit (e.g., .001)
    std::string lex;
    lex.push_back(current); // '.'
    advance();
    while (!eof && std::isdigit(static_cast<unsigned char>(current))) {
        lex.push_back(current);
        advance();
    }
    return { TokenType::Real, lex, line, col };
}

// -------------------- Operators / Separators -------------------
bool Lexer::isSeparator(char c) const {
    static const std::unordered_set<char> S = { '(',')','{','}','[',']',',',';' };
    return S.count(c) != 0;
}
bool Lexer::isOperatorStart(char c) const {
    // include ! for '!=' and = < > + - * / as basics
    static const std::unordered_set<char> O = { '+','-','*','/','=','<','>','!','&','|' };
    return O.count(c) != 0;
}

Token Lexer::scanOpOrSep() {
    // try multi-char ops first
    char p = peek();
    if ((current == '<' && p == '=') ||
        (current == '>' && p == '=') ||
        (current == '=' && p == '=') ||
        (current == '!' && p == '=') ||
        (current == '&' && p == '&') ||
        (current == '|' && p == '|')) {
        std::string op; op.push_back(current); op.push_back(p);
        advance(); advance();
        return { TokenType::Operator, op, line, col };
    }
    // single-char operator?
    if (isOperatorStart(current) && !(current == '!' || current == '&' || current == '|')) {
        char c = current; advance();
        return { TokenType::Operator, std::string(1,c), line, col };
    }
    // separator?
    if (isSeparator(current)) {
        char c = current; advance();
        return { TokenType::Separator, std::string(1,c), line, col };
    }
    // lone '!' / '&' / '|' that didn't form an operator
    if (current=='!' || current=='&' || current=='|') {
        char c = current; advance();
        return { TokenType::Operator, std::string(1,c), line, col }; // or Unknown if disallowed
    }
    // unknown fallback
    char bad = current; advance();
    return { TokenType::Unknown, std::string(1,bad), line, col };
}

// --------------------------- Dispatcher ------------------------
Token Lexer::nextToken() {
    skipSpace();
    if (eof) return { TokenType::EndOfFile, "", line, col };

    if (current == '"') return scanString();                    // <-- NEW
    if (current == '.' && std::isdigit(static_cast<unsigned char>(peek())))
        return scanRealStartingWithDot();
    if (isLetter(current)) return scanIdentifierFSM();
    if (std::isdigit(static_cast<unsigned char>(current))) return scanNumberFSM();
    if (isSeparator(current) || isOperatorStart(current)) return scanOpOrSep();

    char bad = current; advance();
    return { TokenType::Unknown, std::string(1,bad), line, col };
}

