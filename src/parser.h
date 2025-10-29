// Parser.h
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include "Lexer.h"
#include "Token.h"

enum class StartSymbol {
    Program,     // <Rat25F>
    Statement,   // <Statement>
    Expression   // <Expression>
};

enum class Rule {
    // Top-level
    Rat25F, OptFuncDefs, FuncDefs, FuncDefsPrime, Function,
    OptParamList, ParamList, ParamListPrime, Parameter, Qualifier,
    Body, OptDeclList, DeclList, DeclListPrime, Declaration, IDs, IDsPrime,
    // Statements
    StatementList, StatementListPrime, Statement, Compound, Assign, If, OptElse,
    Return, Print, Scan, While,
    // Expressions
    Condition, Relop, Expression, ExpressionPrime, Term, TermPrime, Factor, Primary, PrimaryPrime
};

struct TraceConfig {
    bool master = true;          // Print on/off
    bool hideEpsilon = true;     // hide "ε"
    bool hideOpt = true;         // hide "Opt ..."
    bool hideScaffolding = true; // hide Rat25F/Statement List

    // Rule based print on/off
    std::unordered_set<Rule> enabled = {
        Rule::Statement, Rule::Assign, Rule::If, Rule::Return, Rule::Print, Rule::Scan, Rule::While
    };
};

struct ParserPolicy {
    bool echoTokens = true;         // print token eco
    bool lenientKeywords = true;    // allow identifier keyword
    bool allowStringPrimary = true; // Primary allow string
};

struct ProductionSink {
    virtual ~ProductionSink() = default;
    virtual void emit(std::string_view line) = 0;
};

struct ConsoleSink : ProductionSink {
    void emit(std::string_view line) override;
};

// pretty print for token type
const char* prettyTokenKind(TokenType t);

class Parser {
public:
    Parser(Lexer& lex,
           TraceConfig trace = {},
           ParserPolicy policy = {},
           std::shared_ptr<ProductionSink> sink = std::make_shared<ConsoleSink>());

    // select starting symbol
    void parse(StartSymbol start = StartSymbol::Program);

private:
    void parseRat25F();

    // Function defs & declarations
    void parseOptFunctionDefinitions();
    void parseFunctionDefinitions();
    void parseFunctionDefinitionsPrime();
    void parseFunction();
    void parseOptParameterList();
    void parseParameterList();
    void parseParameterListPrime();
    void parseParameter();
    void parseQualifier();
    void parseBody();
    void parseOptDeclarationList();
    void parseDeclarationList();
    void parseDeclarationListPrime();
    void parseDeclaration();
    void parseIDs();
    void parseIDsPrime();

    // Statements
    void parseStatementList();
    void parseStatementListPrime();
    void parseOptStatementList();
    void parseStatement();
    void parseCompound();
    void parseAssign();
    void parseIf();
    void parseOptElse();
    void parseReturn();
    void parsePrint();
    void parseScan();
    void parseWhile();

    // Expressions
    void parseCondition();
    void parseRelop();
    void parseExpression();
    void parseExpressionPrime();
    void parseTerm();
    void parseTermPrime();
    void parseFactor();
    void parsePrimary();
    void parsePrimaryPrime();

    // helpers
    void advance();
    bool isKw(const char* s) const;
    bool isOp(const char* s) const;
    bool isSep(const char* s) const;

    [[noreturn]] void errorHere(const std::string& msg) const;
    void echoToken();

    // production print
    void prod(Rule r, const char* text);

    // expect
    void expectIdentifier();
    void expectKw(const char* s);
    void expectOp(const char* s);
    void expectSep(const char* s);

    //handling comments
    void skipDocStrings();
    void skipBannerStrings();


private:
    Lexer& lex_;
    Token tok_{};
    TraceConfig trace_;
    ParserPolicy policy_;
    std::shared_ptr<ProductionSink> sink_;
};
