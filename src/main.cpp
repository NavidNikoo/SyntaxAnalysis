#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Lexer.h"
#include "Parser.h"

static int run_one(const std::string& inPath, const std::string& outPath,
                   TraceConfig trace, ParserPolicy policy) {
    std::ifstream fin(inPath);
    if (!fin) { std::cerr << "Error: cannot open input file: " << inPath << "\n"; return 1; }

    std::ofstream fout(outPath);
    if (!fout) { std::cerr << "Error: cannot open output file: " << outPath << "\n"; return 1; }

    // redirect both cout and cerr to the output file
    std::streambuf* coutBuf = std::cout.rdbuf(fout.rdbuf());
    std::streambuf* cerrBuf = std::cerr.rdbuf(fout.rdbuf());

    int rc = 0;
    try {
        Lexer lex(fin);
        Parser parser(lex, trace, policy);
        parser.parse(StartSymbol::Program);
        std::cout << "Parsing finished successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        rc = 1;
    }

    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);
    return rc;
}

int main(int argc, char** argv) {
    // Trace/policy knobs (match prof’s sample)
    TraceConfig trace;
    trace.master = true;
    trace.hideEpsilon = false;     // show ε
    trace.hideOpt = true;
    trace.hideScaffolding = true;

    ParserPolicy policy;
    policy.echoTokens = true;          // print "Token: … Lexeme: …"
    policy.lenientKeywords = true;     // allow id/kw interop on textual match
    policy.allowStringPrimary = true;  // allow strings as primary

    // No-arg mode: run your 4 test cases automatically
    if (argc == 1) {
        std::vector<std::pair<std::string,std::string>> jobs = {
            {"tests/test0.rat25f", "tests/output0.txt"},
            {"tests/test1.rat25f", "tests/output1.txt"},
            {"tests/test2.rat25f", "tests/output2.txt"},
            {"tests/test3.rat25f", "tests/output3.txt"},
        };
        int rc = 0;
        for (auto& [inP, outP] : jobs) {
            std::cerr << "==> " << inP << " -> " << outP << "\n";
            rc |= run_one(inP, outP, trace, policy);
        }
        return rc;
    }

    // Pair mode: <in1> <out1> [<in2> <out2> ...]
    if ((argc < 3) || (argc % 2 == 0)) {
        std::cerr << "Usage: " << argv[0]
                  << " <input1> <output1> [<input2> <output2> ...]\n";
        std::cerr << "Or run with no args to process tests/test{0..3}.rat25f.\n";
        return 1;
    }

    int rc = 0;
    for (int i = 1; i + 1 < argc; i += 2) {
        rc |= run_one(argv[i], argv[i+1], trace, policy);
    }
    return rc;
}
