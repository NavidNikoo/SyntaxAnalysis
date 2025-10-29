// Parser.cpp
#include "Parser.h"
#include <iostream>
#include <stdexcept>

// ------------ pretty token name ------------
const char* prettyTokenKind(TokenType t) {
    switch (t) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::Keyword:    return "Keyword";
        case TokenType::Integer:    return "Integer";
        case TokenType::Real:       return "Real";
        case TokenType::Operator:   return "Operator";
        case TokenType::Separator:  return "Separator";
        case TokenType::String:     return "String";
        case TokenType::Unknown:    return "Unknown";
        case TokenType::EndOfFile:  return "EOF";
    }
    return "Unknown";
}

void ConsoleSink::emit(std::string_view line) {
    std::cout << line << "\n";
}

struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; };

// ------------ small utils ------------
static inline bool contains(std::string_view s, std::string_view sub) {
    return s.find(sub) != std::string_view::npos;
}

// ------------ Parser impl ------------
Parser::Parser(Lexer& lex, TraceConfig trace, ParserPolicy policy,
               std::shared_ptr<ProductionSink> sink)
    : lex_(lex), trace_(std::move(trace)), policy_(std::move(policy)), sink_(std::move(sink)) {
    advance();
}

void Parser::parse(StartSymbol start) {
    switch (start) {
        case StartSymbol::Program:    parseRat25F();    break;
        case StartSymbol::Statement:  parseStatement(); break;
        case StartSymbol::Expression: parseExpression(); break;
    }
}

void Parser::advance() { tok_ = lex_.nextToken(); }

bool Parser::isKw(const char* s) const {
    if (!policy_.lenientKeywords) {
        return (tok_.type == TokenType::Keyword) && (tok_.lexeme == s);
    }
    // lenient: Keyword 또는 Identifier여도 lexeme가 같으면 허용
    if (tok_.type == TokenType::Keyword || tok_.type == TokenType::Identifier)
        return tok_.lexeme == s;
    return false;
}
bool Parser::isOp(const char* s) const { return tok_.type == TokenType::Operator  && tok_.lexeme == s; }
bool Parser::isSep(const char* s) const { return tok_.type == TokenType::Separator && tok_.lexeme == s; }

[[noreturn]] void Parser::errorHere(const std::string& msg) const {
    throw ParseError("Syntax error: " + msg +
                     " at line " + std::to_string(tok_.line) +
                     ", col " + std::to_string(tok_.col) +
                     " (near '" + tok_.lexeme + "')");
}

void Parser::echoToken() {
    if (!policy_.echoTokens) return;
    if (tok_.type == TokenType::EndOfFile) return;
    std::cout << "Token: " << prettyTokenKind(tok_.type)
              << " Lexeme: " << tok_.lexeme << "\n";
}

void Parser::prod(Rule r, const char* text) {
    if (!trace_.master) return;

    // 선택 규칙만 출력(비워두면 전부 허용)
    if (!trace_.enabled.empty() && trace_.enabled.count(r) == 0)
        return;

    // 자동 필터: ε / Opt / 상위 스캐폴딩
    if (trace_.hideEpsilon && contains(text, "ε")) return;
    if (trace_.hideOpt && contains(text, "Opt ")) return;
    if (trace_.hideScaffolding &&
        (contains(text, "Rat25F") || contains(text, "Statement List"))) return;

    sink_->emit(text);
}

// ------------ expect ------------
void Parser::expectIdentifier() {
    if (tok_.type != TokenType::Identifier) errorHere("identifier expected");
    echoToken(); advance();
}
void Parser::expectKw(const char* s) {
    if (!isKw(s)) errorHere(std::string("'") + s + "' expected");
    echoToken(); advance();
}
void Parser::expectOp(const char* s) {
    if (!isOp(s)) errorHere(std::string("operator '") + s + "' expected");
    echoToken(); advance();
}
void Parser::expectSep(const char* s) {
    if (!isSep(s)) errorHere(std::string("separator '") + s + "' expected");
    echoToken(); advance();
}

// =================== Grammar ===================

// <Rat25F> -> <Opt Function Definitions> <Opt Declaration List> <Statement List>
void Parser::parseRat25F() {
    prod(Rule::Rat25F, "<Rat25F> -> <Opt Function Definitions> <Opt Declaration List> <Statement List>");
    skipBannerStrings();
    parseOptFunctionDefinitions();
    skipBannerStrings();
    parseOptDeclarationList();
    skipBannerStrings();
    parseStatementList();
}


// ----- Function defs -----
void Parser::parseOptFunctionDefinitions() {
    skipBannerStrings();  // <== NEW (handles banners before the first function)
    if (isKw("function")) {
        prod(Rule::OptFuncDefs, "<Opt Function Definitions> -> <Function Definitions>");
        parseFunctionDefinitions();
    } else {
        prod(Rule::OptFuncDefs, "<Opt Function Definitions> -> ε");
    }
}
void Parser::parseFunctionDefinitions() {
    prod(Rule::FuncDefs, "<Function Definitions> -> <Function> <Function Definitions Prime>");
    parseFunction();
    parseFunctionDefinitionsPrime();
}
void Parser::parseFunctionDefinitionsPrime() {
    skipBannerStrings();  // <== NEW (handles banners *between* functions)
    if (isKw("function")) {
        prod(Rule::FuncDefsPrime, "<Function Definitions Prime> -> <Function> <Function Definitions Prime>");
        parseFunction();
        parseFunctionDefinitionsPrime();
    } else {
        prod(Rule::FuncDefsPrime, "<Function Definitions Prime> -> ε");
    }
}
void Parser::parseFunction() {
    prod(Rule::Function, "<Function> -> function <Identifier> ( <Opt Parameter List> ) <Opt Declaration List> <Body>");
    expectKw("function");
    expectIdentifier();
    expectSep("(");
    parseOptParameterList();
    expectSep(")");
    parseOptDeclarationList();
    parseBody();
}
void Parser::parseOptParameterList() {
    // parameters start with an identifier, not the qualifier
    if (tok_.type == TokenType::Identifier) {
        prod(Rule::OptParamList, "<Opt Parameter List> -> <Parameter List>");
        parseParameterList();
    } else {
        prod(Rule::OptParamList, "<Opt Parameter List> -> ε");
    }
}
void Parser::parseParameterList() {
    prod(Rule::ParamList, "<Parameter List> -> <Parameter> <Parameter List Prime>");
    parseParameter();
    parseParameterListPrime();
}
void Parser::parseParameterListPrime() {
    if (isSep(",")) {
        prod(Rule::ParamListPrime, "<Parameter List Prime> -> , <Parameter> <Parameter List Prime>");
        expectSep(",");
        parseParameter();
        parseParameterListPrime();
    } else {
        prod(Rule::ParamListPrime, "<Parameter List Prime> -> ε");
    }
}
void Parser::parseParameter() {
    // <Parameter> -> <IDs> <Qualifier>
    prod(Rule::Parameter, "<Parameter> -> <IDs> <Qualifier>");
    parseIDs();
    parseQualifier();
}
void Parser::parseQualifier() {
    if (isKw("integer") || isKw("boolean") || isKw("real")) {
        prod(Rule::Qualifier, "<Qualifier> -> integer | boolean | real");
        echoToken(); advance();
    } else {
        errorHere("qualifier (integer|boolean|real) expected");
    }
}

void Parser::parseBody() {
    prod(Rule::Body, "<Body> -> { <Opt Statement List> }");
    expectSep("{");
    parseOptStatementList(); // instead of parseStatementList()
    expectSep("}");
}

// ----- Declarations -----
void Parser::parseOptDeclarationList() {
    if (isKw("integer") || isKw("boolean") || isKw("real")) {
        prod(Rule::OptDeclList, "<Opt Declaration List> -> <Declaration List>");
        parseDeclarationList();
    } else {
        prod(Rule::OptDeclList, "<Opt Declaration List> -> ε");
    }
}
void Parser::parseDeclarationList() {
    prod(Rule::DeclList, "<Declaration List> -> <Declaration> ; <Declaration List Prime>");
    parseDeclaration();
    expectSep(";");
    parseDeclarationListPrime();
}
void Parser::parseDeclarationListPrime() {
    if (isKw("integer") || isKw("boolean") || isKw("real")) {
        prod(Rule::DeclListPrime, "<Declaration List Prime> -> <Declaration> ; <Declaration List Prime>");
        parseDeclaration();
        expectSep(";");
        parseDeclarationListPrime();
    } else {
        prod(Rule::DeclListPrime, "<Declaration List Prime> -> ε");
    }
}
void Parser::parseDeclaration() {
    prod(Rule::Declaration, "<Declaration> -> <Qualifier> <IDs>");
    parseQualifier();
    parseIDs();
}
void Parser::parseIDs() {
    prod(Rule::IDs, "<IDs> -> <Identifier> <IDs Prime>");
    expectIdentifier();
    parseIDsPrime();
}
void Parser::parseIDsPrime() {
    if (isSep(",")) {
        prod(Rule::IDsPrime, "<IDs Prime> -> , <IDs>");
        expectSep(",");
        parseIDs();
    } else {
        prod(Rule::IDsPrime, "<IDs Prime> -> ε");
    }
}

// ----- Statements -----
void Parser::parseStatementList() {
    // Skip any stray banner strings before deciding if there are statements
    skipBannerStrings();                 // <<== NEW

    // Make Statement List *effectively optional* when there's nothing to parse.
    if (tok_.type == TokenType::EndOfFile || isSep("}")) {
        prod(Rule::StatementList, "<Statement List> -> ε"); // <<== NEW production line
        return;
    }

    // If a statement *can* start, parse it; otherwise epsilon.
    if (isSep("{") || tok_.type == TokenType::Identifier || isKw("if") || isKw("return")
        || isKw("put") || isKw("get") || isKw("while") ) {
        prod(Rule::StatementList, "<Statement List> -> <Statement> <Statement List Prime>");
        parseStatement();
        parseStatementListPrime();
        } else {
            prod(Rule::StatementList, "<Statement List> -> ε"); // <<== NEW safe fallback
        }
}

void Parser::parseStatementListPrime() {
    skipBannerStrings();                 // <<== NEW

    if (tok_.type == TokenType::EndOfFile || isSep("}")) {
        prod(Rule::StatementListPrime, "<Statement List Prime> -> ε");
        return;
    }
    if (isSep("{") || tok_.type == TokenType::Identifier || isKw("if") || isKw("return")
        || isKw("put") || isKw("get") || isKw("while")) {
        prod(Rule::StatementListPrime, "<Statement List Prime> -> <Statement> <Statement List Prime>");
        parseStatement();
        parseStatementListPrime();
        } else {
            prod(Rule::StatementListPrime, "<Statement List Prime> -> ε");
        }
}

void Parser::parseOptStatementList() {
    if (tok_.type == TokenType::EndOfFile || isSep("}")) {
        prod(Rule::StatementList, "<Statement List> -> ε");
        return;
    }
    parseStatementList();
}


void Parser::parseStatement() {

    // Consume banner strings that appear as standalone "statements"
    if (tok_.type == TokenType::String) { // banner/comment line
        advance();                         // no echo
        // After consuming, let the caller continue its loop.
        // We return to let StatementList/Prime handle sequencing.
        prod(Rule::Statement, "<Statement> -> ε"); // optional: mark we consumed nothing
        return;
    }

    // 선택된 대안만 출력 (메뉴판 제거)
    if (isSep("{")) {
        prod(Rule::Statement, "<Statement> -> <Compound>");
        parseCompound();
    } else if (tok_.type == TokenType::Identifier) {
        prod(Rule::Statement, "<Statement> -> <Assign>");
        parseAssign();
    } else if (isKw("if")) {
        prod(Rule::Statement, "<Statement> -> <If>");
        parseIf();
    } else if (isKw("return")) {
        prod(Rule::Statement, "<Statement> -> <Return>");
        parseReturn();
    } else if (isKw("put")) {
        prod(Rule::Statement, "<Statement> -> <Print>");
        parsePrint();
    } else if (isKw("get")) {
        prod(Rule::Statement, "<Statement> -> <Scan>");
        parseScan();
    } else if (isKw("while")) {
        prod(Rule::Statement, "<Statement> -> <While>");
        parseWhile();
    } else {
        errorHere("statement expected");
    }
}
void Parser::parseCompound() {
    prod(Rule::Compound, "<Compound> -> { <Statement List> }");
    expectSep("{");
    parseStatementList();
    expectSep("}");
}
void Parser::parseAssign() {
    prod(Rule::Assign, "<Assign> -> <Identifier> = <Expression> ;");
    expectIdentifier();
    expectOp("=");
    parseExpression();
    expectSep(";");
}
void Parser::parseIf() {
    prod(Rule::If, "<If> -> if ( <Condition> ) <Statement> <OptElse> fi");
    expectKw("if");
    expectSep("(");
    parseCondition();
    expectSep(")");
    parseStatement();
    parseOptElse();
    expectKw("fi");
}
void Parser::parseOptElse() {
    if (isKw("else")) {
        prod(Rule::OptElse, "<OptElse> -> else <Statement>");
        expectKw("else");
        parseStatement();
    } else {
        prod(Rule::OptElse, "<OptElse> -> ε");
    }
}
void Parser::parseReturn() {
    prod(Rule::Return, "<Return> -> return ; | return <Expression> ;");
    expectKw("return");
    if (isSep(";")) {
        expectSep(";");
    } else {
        parseExpression();
        expectSep(";");
    }
}
void Parser::parsePrint() {
    prod(Rule::Print, "<Print> -> put ( <Expression> ) ;");
    expectKw("put");
    expectSep("(");
    parseExpression();
    expectSep(")");
    expectSep(";");
}
void Parser::parseScan() {
    prod(Rule::Scan, "<Scan> -> get ( <IDs> ) ;");
    expectKw("get");
    expectSep("(");
    parseIDs();
    expectSep(")");
    expectSep(";");
}
void Parser::parseWhile() {
    prod(Rule::While, "<While> -> while ( <Condition> ) <Statement>");
    expectKw("while");
    expectSep("(");
    parseCondition();
    expectSep(")");
    parseStatement();
}

// ----- Expressions -----
void Parser::parseCondition() {
    prod(Rule::Condition, "<Condition> -> <Expression> <Relop> <Expression>");
    parseExpression();
    parseRelop();
    parseExpression();
}

void Parser::parseRelop() {
    if (isOp("==") || isOp("!=") || isOp(">") || isOp("<") || isOp("<=") || isOp(">=")) {
        if (trace_.master && trace_.enabled.count(Rule::Relop)) {
            sink_->emit(std::string("<Relop> -> ") + tok_.lexeme);
        }
        echoToken();
        advance();
    } else {
        errorHere("relational operator expected");
    }
}

void Parser::parseExpression() {
    prod(Rule::Expression, "<Expression> -> <Term> <Expression Prime>");
    parseTerm();
    parseExpressionPrime();
}
void Parser::parseExpressionPrime() {
    if (isOp("+")) {
        prod(Rule::ExpressionPrime, "<Expression Prime> -> + <Term> <Expression Prime>");
        expectOp("+");
        parseTerm();
        parseExpressionPrime();
    } else if (isOp("-")) {
        prod(Rule::ExpressionPrime, "<Expression Prime> -> - <Term> <Expression Prime>");
        expectOp("-");
        parseTerm();
        parseExpressionPrime();
    } else {
        prod(Rule::ExpressionPrime, "<Expression Prime> -> ε");
    }
}
void Parser::parseTerm() {
    prod(Rule::Term, "<Term> -> <Factor> <Term Prime>");
    parseFactor();
    parseTermPrime();
}
void Parser::parseTermPrime() {
    if (isOp("*")) {
        prod(Rule::TermPrime, "<Term Prime> -> * <Factor> <Term Prime>");
        expectOp("*");
        parseFactor();
        parseTermPrime();
    } else if (isOp("/")) {
        prod(Rule::TermPrime, "<Term Prime> -> / <Factor> <Term Prime>");
        expectOp("/");
        parseFactor();
        parseTermPrime();
    } else {
        prod(Rule::TermPrime, "<Term Prime> -> ε");
    }
}
void Parser::parseFactor() {
    if (isOp("-")) {
        prod(Rule::Factor, "<Factor> -> - <Primary>");
        expectOp("-");
        parsePrimary();
    } else {
        prod(Rule::Factor, "<Factor> -> <Primary>");
        parsePrimary();
    }
}
void Parser::parsePrimary() {
    if (tok_.type == TokenType::Identifier) {
        prod(Rule::Primary, "<Primary> -> <Identifier> <Primary Prime>");
        expectIdentifier();
        parsePrimaryPrime();
    } else if (tok_.type == TokenType::Integer) {
        prod(Rule::Primary, "<Primary> -> <Integer>");
        echoToken(); advance();
    } else if (tok_.type == TokenType::Real) {
        prod(Rule::Primary, "<Primary> -> <Real>");
        echoToken(); advance();
    } else if (isSep("(")) {
        prod(Rule::Primary, "<Primary> -> ( <Expression> )");
        expectSep("(");
        parseExpression();
        expectSep(")");
    } else if ((tok_.lexeme == "true" || tok_.lexeme == "false")
               && (tok_.type == TokenType::Identifier || tok_.type == TokenType::Keyword)) {
        prod(Rule::Primary, "<Primary> -> true | false");
        echoToken(); advance();
    } else if (policy_.allowStringPrimary && tok_.type == TokenType::String) {
        prod(Rule::Primary, "<Primary> -> <String>");
        echoToken(); advance();
    } else {
        errorHere("primary expected");
    }
}
void Parser::parsePrimaryPrime() {
    if (isSep("(")) {
        prod(Rule::PrimaryPrime, "<Primary Prime> -> ( <IDs> )");
        expectSep("(");
        parseIDs();
        expectSep(")");
    } else {
        prod(Rule::PrimaryPrime, "<Primary Prime> -> ε");
    }
}

//Comment handling
void Parser::skipDocStrings() {
    while (tok_.type == TokenType::String) {
        echoToken();
        advance();
    }
}

// Consume any top-level/bare string tokens (banner comments).
void Parser::skipBannerStrings() {
    while (tok_.type == TokenType::String) {
        // do NOT echo; banners should be invisible in output
        advance();
    }
}
