#include "grammar.h"
#include "parser.h"
#include "lexer.h"
#include "token.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string readFile(const std::string& path) {
    std::ifstream fin(path);
    if (!fin.is_open()) return "";
    std::ostringstream oss;
    oss << fin.rdbuf();
    return oss.str();
}

static void printUsage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " <source.pas>               Lex + Parse (on-demand)\n"
        << "  " << prog << " --lex <source.pas>         Lex only (print tokens)\n"
        << "  " << prog << " --grammar                  Print LL(1) grammar\n"
        << "  " << prog << " --first-follow             Print FIRST/FOLLOW sets\n"
        << "  " << prog << " --table                    Print LL(1) parse table\n"
        << "  " << prog << " --all <source.pas>         Print everything + parse\n";
}

static void printTokens(const std::vector<Token>& tokens, std::ostream& os) {
    for (const auto& tok : tokens) {
        os << tokenTypeToString(tok.type) << " "
           << tok.lexeme << " "
           << tok.line << " " << tok.column << "\n";
    }
}

// ---------------------------------------------------------------------------
// Build grammar (shared by all modes that need parsing)
// ---------------------------------------------------------------------------
static Grammar buildGrammar() {
    Grammar g = buildPascalSGrammar();
    g.transformToLL1();
    g.computeFirstSets();
    g.computeFollowSets();
    g.buildParseTable();
    return g;
}

// ---------------------------------------------------------------------------
// Run the full pipeline: source → Lexer(on-demand) → Parser → AST
// ---------------------------------------------------------------------------
static bool runPipeline(Grammar& g, const std::string& inputFile, bool verbose) {
    if (verbose) std::cout << "[pipeline] Lexing + parsing source file (on-demand)\n";
    std::string source = readFile(inputFile);
    if (source.empty()) {
        std::cerr << "Error: cannot open or empty source file: " << inputFile << "\n";
        return false;
    }
    Lexer lexer(source);
    Parser parser(g, lexer);

    // Stage 2: Parse → AST
    bool success = parser.parse();

    // Report lexer errors (collected during on-demand tokenization)
    if (parser.hasLexerErrors()) {
        std::cerr << "*** LEXER ERRORS ***\n";
        for (const auto& err : parser.getLexerErrors()) {
            std::cerr << "  [Line " << err.line << ", Col " << err.column << "] "
                      << err.message << "\n";
        }
    }

    // Dump consumed token list if verbose
    if (verbose) {
        std::ofstream fout("tokens.txt");
        printTokens(parser.getConsumedTokens(), fout);
        std::cout << "[pipeline] Token list written to tokens.txt\n";
    }

    if (verbose) {
        std::ofstream fout("parse_process.txt");
        parser.printParseProcess(fout);
        std::cout << "[pipeline] Parse process written to parse_process.txt\n";
    }

    if (success) {
        std::cout << "\n*** PARSE SUCCESSFUL ***\n\n";
        std::cout << "=== AST ===\n";
        printAST(parser.getASTRoot(), std::cout);

        if (verbose) {
            std::ofstream fout("ast.txt");
            printAST(parser.getASTRoot(), fout);
            std::cout << "\n[pipeline] AST written to ast.txt\n";
        }

        // --- Future pipeline stages ---
        // Stage 3: Semantic analysis  (TODO)
        // Stage 4: C code generation  (TODO)
    } else {
        std::cerr << "\n*** PARSE FAILED with "
                  << parser.getErrors().size() << " error(s): ***\n";
        for (auto& err : parser.getErrors()) {
            std::cerr << "  [Line " << err.line << ", Col " << err.column << "] "
                      << err.message << "\n";
        }
    }

    return success;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    // ------ Lex-only mode (uses Lexer directly, no Parser) ------
    if (mode == "--lex") {
        if (argc < 3) {
            std::cerr << "Error: --lex requires a source file argument.\n";
            return 1;
        }
        std::string source = readFile(argv[2]);
        if (source.empty()) {
            std::cerr << "Error: cannot open source file: " << argv[2] << "\n";
            return 1;
        }
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (lexer.hasErrors()) {
            std::cerr << "*** LEXER ERRORS ***\n";
            for (const auto& err : lexer.getErrors()) {
                std::cerr << "  [Line " << err.line << ", Col " << err.column << "] "
                          << err.message << "\n";
            }
        }
        printTokens(tokens, std::cout);
        return lexer.hasErrors() ? 1 : 0;
    }

    // ------ Grammar-info modes ------
    Grammar g = buildGrammar();

    if (mode == "--grammar") {
        std::cout << "=== Transformed LL(1) Grammar ===\n\n";
        g.print(std::cout);
        return 0;
    }

    if (mode == "--first-follow") {
        std::cout << "=== FIRST Sets ===\n\n";
        for (auto& nt : g.nonTerminals) {
            std::cout << "FIRST(" << nt << ") = { ";
            bool first = true;
            for (auto& f : g.firstSets[nt]) {
                if (!first) std::cout << ", ";
                std::cout << f;
                first = false;
            }
            std::cout << " }\n";
        }
        std::cout << "\n=== FOLLOW Sets ===\n\n";
        for (auto& nt : g.nonTerminals) {
            std::cout << "FOLLOW(" << nt << ") = { ";
            bool first = true;
            for (auto& f : g.followSets[nt]) {
                if (!first) std::cout << ", ";
                std::cout << f;
                first = false;
            }
            std::cout << " }\n";
        }
        return 0;
    }

    if (mode == "--table") {
        g.printParseTable(std::cout);
        return 0;
    }

    // ------ Parse mode (default or --all) ------
    std::string inputFile;
    bool verbose = false;

    if (mode == "--all") {
        if (argc < 3) {
            std::cerr << "Error: --all requires an input file argument.\n";
            return 1;
        }
        inputFile = argv[2];
        verbose = true;
    } else {
        inputFile = mode;
    }

    bool success = runPipeline(g, inputFile, verbose);
    return success ? 0 : 1;
}
