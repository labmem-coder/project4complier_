#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>

// A production: lhs -> rhs (list of symbols)
// Symbols: non-terminals are multi-char like "programstruct", terminals are keywords/operators
struct Production {
    std::string lhs;
    std::vector<std::string> rhs; // "ε" represented as empty vector
};

class Grammar {
public:
    std::vector<Production> productions;
    std::set<std::string> nonTerminals;
    std::set<std::string> terminals;
    std::string startSymbol;

    void addProduction(const std::string& lhs, const std::vector<std::string>& rhs);
    void classifySymbols();
    void eliminateLeftRecursion();
    void eliminateLeftFactoring();
    void transformToLL1();
    void print(std::ostream& os) const;

    // FIRST and FOLLOW
    std::map<std::string, std::set<std::string>> firstSets;
    std::map<std::string, std::set<std::string>> followSets;
    void computeFirstSets();
    void computeFollowSets();
    std::set<std::string> computeFirstOfSequence(const std::vector<std::string>& seq) const;

    // LL(1) parse table: [nonTerminal][terminal] -> production index
    std::map<std::string, std::map<std::string, int>> parseTable;
    bool buildParseTable();
    void printParseTable(std::ostream& os) const;

private:
    bool isNonTerminal(const std::string& s) const;
    std::string newNonTerminal(const std::string& base);
    int ntCounter = 0;
};

// Build the Pascal-S grammar from the specification
Grammar buildPascalSGrammar();

#endif
