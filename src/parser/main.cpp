#include "grammar.h"
#include "parser.h"
#include "token.h"
#include <iostream>
#include <fstream>
#include <string>

void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " <token_file>           Parse the token file\n"
              << "  " << prog << " --grammar              Print the LL(1) grammar\n"
              << "  " << prog << " --first-follow         Print FIRST and FOLLOW sets\n"
              << "  " << prog << " --table                Print LL(1) parse table\n"
              << "  " << prog << " --all <token_file>     Print everything + parse\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    // Build and transform grammar
    Grammar g = buildPascalSGrammar();

    // The grammar is already manually transformed in buildPascalSGrammar().
    // But we still run the automated transformation for any remaining issues.
    g.transformToLL1();

    // Compute FIRST and FOLLOW sets
    g.computeFirstSets();
    g.computeFollowSets();

    // Build parse table
    bool isLL1 = g.buildParseTable();

    if (mode == "--grammar") {
        std::cout << "=== Transformed LL(1) Grammar ===\n\n";
        g.print(std::cout);
        if (!isLL1) {
            std::cout << "\n[WARNING] Grammar has LL(1) conflicts.\n";
        }
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

        // Also output to file
        std::ofstream fout("parse_table.txt");
        if (fout.is_open()) {
            g.printParseTable(fout);
            fout.close();
            std::cout << "\nParse table also written to parse_table.txt\n";
        }
        return 0;
    }

    // Parse mode
    std::string tokenFile;
    bool printAll = false;

    if (mode == "--all") {
        if (argc < 3) {
            std::cerr << "Error: --all requires a token file argument.\n";
            return 1;
        }
        tokenFile = argv[2];
        printAll = true;
    } else {
        tokenFile = mode;
    }

    if (printAll) {
        // Output grammar to file
        {
            std::ofstream fout("ll1_grammar.txt");
            fout << "=== Transformed LL(1) Grammar ===\n\n";
            g.print(fout);
            fout.close();
            std::cout << "LL(1) grammar written to ll1_grammar.txt\n";
        }

        // Output FIRST/FOLLOW to file
        {
            std::ofstream fout("first_follow.txt");
            fout << "=== FIRST Sets ===\n\n";
            for (auto& nt : g.nonTerminals) {
                fout << "FIRST(" << nt << ") = { ";
                bool first = true;
                for (auto& f : g.firstSets[nt]) {
                    if (!first) fout << ", ";
                    fout << f;
                    first = false;
                }
                fout << " }\n";
            }
            fout << "\n=== FOLLOW Sets ===\n\n";
            for (auto& nt : g.nonTerminals) {
                fout << "FOLLOW(" << nt << ") = { ";
                bool first = true;
                for (auto& f : g.followSets[nt]) {
                    if (!first) fout << ", ";
                    fout << f;
                    first = false;
                }
                fout << " }\n";
            }
            fout.close();
            std::cout << "FIRST/FOLLOW sets written to first_follow.txt\n";
        }

        // Output parse table to file
        {
            std::ofstream fout("parse_table.txt");
            g.printParseTable(fout);
            fout.close();
            std::cout << "Parse table written to parse_table.txt\n";
        }
    }

    // Read tokens and parse
    std::vector<Token> tokens = readTokensFromFile(tokenFile);
    if (tokens.empty()) {
        std::cerr << "Error: no tokens read from " << tokenFile << "\n";
        return 1;
    }

    Parser parser(g, tokens);
    bool success = parser.parse();

    // Output parse process
    {
        std::ofstream fout("parse_process.txt");
        parser.printParseProcess(fout);
        fout.close();
        if (printAll) {
            std::cout << "Parse process written to parse_process.txt\n";
        }
    }

    // Print parse process to stdout too
    parser.printParseProcess(std::cout);

    if (success) {
        {
            std::ofstream astOut("ast.txt");
            printAST(parser.getASTRoot(), astOut);
        }

        std::cout << "\n*** PARSE SUCCESSFUL ***\n";
        std::cout << "\n=== AST ===\n";
        printAST(parser.getASTRoot(), std::cout);
        if (printAll) {
            std::cout << "AST written to ast.txt\n";
        }
    } else {
        std::cout << "\n*** PARSE FAILED with " << parser.getErrors().size() << " error(s): ***\n";
        for (auto& err : parser.getErrors()) {
            std::cout << "  [Line " << err.line << ", Col " << err.column << "] " << err.message << "\n";
        }
    }

    return success ? 0 : 1;
}
