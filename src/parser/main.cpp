#include "grammar.h"
#include "parser.h"
#include "token.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool endsWith(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

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
        << "  " << prog << " <source.pas|tokens.tok>   Lex (if .pas) + Parse\n"
        << "  " << prog << " --lex <source.pas>         Lex only (print tokens)\n"
        << "  " << prog << " --grammar                  Print LL(1) grammar\n"
        << "  " << prog << " --first-follow             Print FIRST/FOLLOW sets\n"
        << "  " << prog << " --table                    Print LL(1) parse table\n"
        << "  " << prog << " --all <source.pas|.tok>    Print everything + parse\n";
}

static void printTokens(const std::vector<Token>& tokens, std::ostream& os) {
    for (const auto& tok : tokens) {
        os << tokenTypeToString(tok.type) << " "
           << tok.lexeme << " "
           << tok.line << " " << tok.column << "\n";
    }
}

// ---------------------------------------------------------------------------
// Legacy .tok file reader (kept for backward compatibility only)
// ---------------------------------------------------------------------------
static std::vector<Token> readTokensFromFile(const std::string& filename) {
    std::vector<Token> tokens;
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open token file: " << filename << "\n";
        return tokens;
    }
    auto nameMap = buildTokenNameMap();
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string typeName, lexeme;
        int ln = 0, col = 0;
        if (!(iss >> typeName >> lexeme >> ln >> col)) continue;
        auto it = nameMap.find(typeName);
        if (it == nameMap.end()) continue;
        tokens.push_back({it->second, lexeme, ln, col});
    }
    return tokens;
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
// Run the full pipeline: source/tokens → Parser → AST
//   For .pas files: Parser lexes internally via Parser(grammar, source)
//   For .tok files: read tokens, then Parser(grammar, tokens)
// ---------------------------------------------------------------------------
static bool runPipeline(Grammar& g, const std::string& inputFile, bool verbose) {
    Parser* parserPtr = nullptr;
    std::unique_ptr<Parser> parser;

    if (endsWith(inputFile, ".tok")) {
        // Legacy path: pre-tokenized file
        if (verbose) std::cout << "[pipeline] Reading pre-tokenized file\n";
        auto tokens = readTokensFromFile(inputFile);
        if (tokens.empty()) {
            std::cerr << "Error: no tokens read from " << inputFile << "\n";
            return false;
        }
        parser = std::make_unique<Parser>(g, tokens);
    } else {
        // Primary path: lexer integrated in parser
        if (verbose) std::cout << "[pipeline] Lexing + parsing source file\n";
        std::string source = readFile(inputFile);
        if (source.empty()) {
            std::cerr << "Error: cannot open or empty source file: " << inputFile << "\n";
            return false;
        }
        parser = std::make_unique<Parser>(g, source);

        // Report lexer errors
        if (parser->hasLexerErrors()) {
            std::cerr << "*** LEXER ERRORS ***\n";
            for (const auto& err : parser->getLexerErrors()) {
                std::cerr << "  [Line " << err.line << ", Col " << err.column << "] "
                          << err.message << "\n";
            }
        }
    }
    parserPtr = parser.get();

    // Dump token list if verbose
    if (verbose) {
        std::ofstream fout("tokens.txt");
        printTokens(parserPtr->getTokens(), fout);
        std::cout << "[pipeline] Token list written to tokens.txt\n";
    }

    // Stage 2: Parse → AST
    bool success = parserPtr->parse();

    if (verbose) {
        std::ofstream fout("parse_process.txt");
        parserPtr->printParseProcess(fout);
        std::cout << "[pipeline] Parse process written to parse_process.txt\n";
    }

    if (success) {
        std::cout << "\n*** PARSE SUCCESSFUL ***\n\n";
        std::cout << "=== AST ===\n";
        printAST(parserPtr->getASTRoot(), std::cout);

        if (verbose) {
            std::ofstream fout("ast.txt");
            printAST(parserPtr->getASTRoot(), fout);
            std::cout << "\n[pipeline] AST written to ast.txt\n";
        }

        // --- Future pipeline stages ---
        // Stage 3: Semantic analysis  (TODO)
        // Stage 4: C code generation  (TODO)
    } else {
        std::cerr << "\n*** PARSE FAILED with "
                  << parserPtr->getErrors().size() << " error(s): ***\n";
        for (auto& err : parserPtr->getErrors()) {
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
