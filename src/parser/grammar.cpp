#include "grammar.h"
#include <algorithm>
#include <sstream>
#include <cassert>
#include <queue>

// ==================== Grammar basic operations ====================

void Grammar::addProduction(const std::string& lhs, const std::vector<std::string>& rhs) {
    productions.push_back({lhs, rhs});
    nonTerminals.insert(lhs);
}

bool Grammar::isNonTerminal(const std::string& s) const {
    return nonTerminals.count(s) > 0;
}

std::string Grammar::newNonTerminal(const std::string& base) {
    std::string name = base + "'";
    while (nonTerminals.count(name)) {
        name += "'";
    }
    nonTerminals.insert(name);
    return name;
}

void Grammar::classifySymbols() {
    terminals.clear();
    for (auto& p : productions) {
        for (auto& s : p.rhs) {
            if (!isNonTerminal(s) && s != "ε") {
                terminals.insert(s);
            }
        }
    }
    terminals.insert("$"); // end marker
}

void Grammar::print(std::ostream& os) const {
    for (auto& p : productions) {
        os << p.lhs << " -> ";
        if (p.rhs.empty()) {
            os << "ε";
        } else {
            for (size_t i = 0; i < p.rhs.size(); i++) {
                if (i) os << " ";
                os << p.rhs[i];
            }
        }
        os << "\n";
    }
}

// ==================== Left Recursion Elimination ====================

void Grammar::eliminateLeftRecursion() {
    // Collect non-terminals in order of first appearance
    std::vector<std::string> ntOrder;
    std::set<std::string> seen;
    for (auto& p : productions) {
        if (!seen.count(p.lhs)) {
            ntOrder.push_back(p.lhs);
            seen.insert(p.lhs);
        }
    }

    for (size_t i = 0; i < ntOrder.size(); i++) {
        std::string Ai = ntOrder[i];

        // Substitute: for j < i, replace Ai -> Aj gamma with Ai -> delta1 gamma | delta2 gamma ...
        for (size_t j = 0; j < i; j++) {
            std::string Aj = ntOrder[j];
            std::vector<Production> newProds;
            std::vector<int> toRemove;

            for (size_t k = 0; k < productions.size(); k++) {
                if (productions[k].lhs == Ai && !productions[k].rhs.empty() &&
                    productions[k].rhs[0] == Aj) {
                    toRemove.push_back((int)k);
                    // Find all Aj productions
                    for (auto& pj : productions) {
                        if (pj.lhs == Aj) {
                            std::vector<std::string> newRhs;
                            if (!pj.rhs.empty()) {
                                newRhs = pj.rhs;
                            }
                            for (size_t m = 1; m < productions[k].rhs.size(); m++) {
                                newRhs.push_back(productions[k].rhs[m]);
                            }
                            newProds.push_back({Ai, newRhs});
                        }
                    }
                }
            }

            // Remove old, add new
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (int idx : toRemove) {
                productions.erase(productions.begin() + idx);
            }
            for (auto& np : newProds) {
                productions.push_back(np);
            }
        }

        // Now eliminate direct left recursion for Ai
        std::vector<Production> alpha; // Ai -> Ai alpha (left-recursive)
        std::vector<Production> beta;  // Ai -> beta (non-left-recursive)

        for (auto& p : productions) {
            if (p.lhs == Ai) {
                if (!p.rhs.empty() && p.rhs[0] == Ai) {
                    alpha.push_back(p);
                } else {
                    beta.push_back(p);
                }
            }
        }

        if (alpha.empty()) continue;

        std::string AiPrime = newNonTerminal(Ai);
        ntOrder.push_back(AiPrime); // add to the list for future iterations

        // Remove all Ai productions
        productions.erase(
            std::remove_if(productions.begin(), productions.end(),
                [&](const Production& p) { return p.lhs == Ai; }),
            productions.end());

        // Ai -> beta AiPrime
        for (auto& b : beta) {
            std::vector<std::string> newRhs = b.rhs;
            newRhs.push_back(AiPrime);
            productions.push_back({Ai, newRhs});
        }

        // AiPrime -> alpha AiPrime | ε
        for (auto& a : alpha) {
            std::vector<std::string> newRhs(a.rhs.begin() + 1, a.rhs.end());
            newRhs.push_back(AiPrime);
            productions.push_back({AiPrime, newRhs});
        }
        productions.push_back({AiPrime, {}}); // epsilon production
    }
}

// ==================== Left Factoring ====================

void Grammar::eliminateLeftFactoring() {
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::string> ntList;
        std::set<std::string> seen2;
        for (auto& p : productions) {
            if (!seen2.count(p.lhs)) {
                ntList.push_back(p.lhs);
                seen2.insert(p.lhs);
            }
        }

        for (auto& A : ntList) {
            // Gather all productions for A
            std::vector<std::vector<std::string>> alts;
            std::vector<int> indices;
            for (int i = 0; i < (int)productions.size(); i++) {
                if (productions[i].lhs == A) {
                    alts.push_back(productions[i].rhs);
                    indices.push_back(i);
                }
            }
            if (alts.size() <= 1) continue;

            // Find longest common prefix among any group
            std::map<std::string, std::vector<int>> groups;
            for (int i = 0; i < (int)alts.size(); i++) {
                std::string key = alts[i].empty() ? "ε" : alts[i][0];
                groups[key].push_back(i);
            }

            for (auto git = groups.begin(); git != groups.end(); ++git) {
                const std::string& prefix = git->first;
                const std::vector<int>& group = git->second;
                if (group.size() <= 1 || prefix == "ε") continue;

                // Find longest common prefix
                size_t lcpLen = 1;
                bool extending = true;
                while (extending) {
                    if (lcpLen >= alts[group[0]].size()) break;
                    std::string next = alts[group[0]][lcpLen];
                    for (size_t gi = 0; gi < group.size(); gi++) {
                        int idx2 = group[gi];
                        if (lcpLen >= alts[idx2].size() || alts[idx2][lcpLen] != next) {
                            extending = false;
                            break;
                        }
                    }
                    if (extending) lcpLen++;
                }

                std::vector<std::string> commonPrefix(alts[group[0]].begin(),
                                                       alts[group[0]].begin() + lcpLen);

                std::string APrime = newNonTerminal(A);
                changed = true;

                // Remove old productions (sorted descending to avoid index shift)
                std::vector<int> removeIdx;
                for (size_t gi = 0; gi < group.size(); gi++) {
                    removeIdx.push_back(indices[group[gi]]);
                }
                std::sort(removeIdx.rbegin(), removeIdx.rend());
                for (size_t ri = 0; ri < removeIdx.size(); ri++) {
                    productions.erase(productions.begin() + removeIdx[ri]);
                }

                // A -> commonPrefix APrime
                std::vector<std::string> newRhs = commonPrefix;
                newRhs.push_back(APrime);
                productions.push_back({A, newRhs});

                // APrime -> suffix1 | suffix2 | ... | ε
                for (size_t gi = 0; gi < group.size(); gi++) {
                    int idx2 = group[gi];
                    std::vector<std::string> suffix(alts[idx2].begin() + lcpLen, alts[idx2].end());
                    if (suffix.empty()) {
                        productions.push_back({APrime, {}}); // epsilon
                    } else {
                        productions.push_back({APrime, suffix});
                    }
                }

                break; // restart since productions changed
            }
            if (changed) break;
        }
    }
}

// ==================== Transform to LL(1) ====================

void Grammar::transformToLL1() {
    // Repeatedly apply left factoring and left recursion elimination
    // until the grammar no longer changes
    for (int iter = 0; iter < 100; iter++) {
        std::vector<Production> before = productions;

        eliminateLeftFactoring();
        eliminateLeftRecursion();
        eliminateLeftFactoring(); // one more pass after recursion elimination

        if (productions.size() == before.size()) {
            bool same = true;
            for (size_t i = 0; i < productions.size(); i++) {
                if (productions[i].lhs != before[i].lhs ||
                    productions[i].rhs != before[i].rhs) {
                    same = false;
                    break;
                }
            }
            if (same) break;
        }
    }
    classifySymbols();
}

// ==================== FIRST Sets ====================

void Grammar::computeFirstSets() {
    firstSets.clear();

    // Initialize: FIRST(terminal) = {terminal}
    for (auto& t : terminals) {
        firstSets[t].insert(t);
    }

    // Initialize FIRST(nonTerminal) = {}
    for (auto& nt : nonTerminals) {
        firstSets[nt] = {};
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : productions) {
            std::string A = p.lhs;
            if (p.rhs.empty()) {
                // A -> ε
                if (!firstSets[A].count("ε")) {
                    firstSets[A].insert("ε");
                    changed = true;
                }
                continue;
            }

            // A -> X1 X2 ... Xn
            bool allHaveEps = true;
            for (size_t i = 0; i < p.rhs.size(); i++) {
                std::string Xi = p.rhs[i];
                // Add FIRST(Xi) - {ε} to FIRST(A)
                for (auto& f : firstSets[Xi]) {
                    if (f != "ε" && !firstSets[A].count(f)) {
                        firstSets[A].insert(f);
                        changed = true;
                    }
                }
                if (!firstSets[Xi].count("ε")) {
                    allHaveEps = false;
                    break;
                }
            }
            if (allHaveEps) {
                if (!firstSets[A].count("ε")) {
                    firstSets[A].insert("ε");
                    changed = true;
                }
            }
        }
    }
}

std::set<std::string> Grammar::computeFirstOfSequence(const std::vector<std::string>& seq) const {
    std::set<std::string> result;
    if (seq.empty()) {
        result.insert("ε");
        return result;
    }
    bool allEps = true;
    for (size_t i = 0; i < seq.size(); i++) {
        auto it = firstSets.find(seq[i]);
        if (it == firstSets.end()) {
            // terminal not in firstSets map directly
            result.insert(seq[i]);
            allEps = false;
            break;
        }
        for (auto& f : it->second) {
            if (f != "ε") result.insert(f);
        }
        if (!it->second.count("ε")) {
            allEps = false;
            break;
        }
    }
    if (allEps) result.insert("ε");
    return result;
}

// ==================== FOLLOW Sets ====================

void Grammar::computeFollowSets() {
    followSets.clear();
    for (auto& nt : nonTerminals) {
        followSets[nt] = {};
    }
    followSets[startSymbol].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : productions) {
            for (size_t i = 0; i < p.rhs.size(); i++) {
                std::string B = p.rhs[i];
                if (!isNonTerminal(B)) continue;

                // beta = rhs[i+1..end]
                std::vector<std::string> beta(p.rhs.begin() + i + 1, p.rhs.end());
                std::set<std::string> firstBeta = computeFirstOfSequence(beta);

                for (auto& f : firstBeta) {
                    if (f != "ε" && !followSets[B].count(f)) {
                        followSets[B].insert(f);
                        changed = true;
                    }
                }

                if (firstBeta.count("ε")) {
                    for (auto& f : followSets[p.lhs]) {
                        if (!followSets[B].count(f)) {
                            followSets[B].insert(f);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

// ==================== LL(1) Parse Table ====================

bool Grammar::buildParseTable() {
    parseTable.clear();
    bool isLL1 = true;

    auto preferLongerForKnownTail = [&](const Production& existingProd, const Production& newProd) {
        const bool knownTail =
            (existingProd.lhs == "statement_id_tail" || existingProd.lhs == "factor_id_tail");
        if (!knownTail) return false;
        if (existingProd.rhs.empty() != newProd.rhs.empty()) return false;
        return newProd.rhs.size() > existingProd.rhs.size();
    };

    auto preferExistingKnownTail = [&](const Production& existingProd, const Production& newProd) {
        const bool knownTail =
            (existingProd.lhs == "statement_id_tail" || existingProd.lhs == "factor_id_tail");
        if (!knownTail) return false;
        if (existingProd.rhs.empty() || newProd.rhs.empty()) return true;
        return existingProd.rhs == newProd.rhs;
    };

    // Track which entries come from epsilon productions (for conflict resolution)
    std::map<std::string, std::map<std::string, bool>> isEpsilonEntry;

    for (int i = 0; i < (int)productions.size(); i++) {
        auto& p = productions[i];
        std::set<std::string> firstAlpha = computeFirstOfSequence(p.rhs);
        bool thisIsEpsilon = p.rhs.empty();

        for (auto& a : firstAlpha) {
            if (a == "ε") continue;
            bool shouldWriteEntry = true;
            if (parseTable[p.lhs].count(a)) {
                // Conflict resolution: prefer non-epsilon production (e.g., dangling else)
                bool existingIsEps = isEpsilonEntry[p.lhs][a];
                if (existingIsEps && !thisIsEpsilon) {
                    // Overwrite epsilon entry with non-epsilon (standard resolution)
                } else if (!existingIsEps && thisIsEpsilon) {
                    // Keep existing non-epsilon entry
                    shouldWriteEntry = false;
                } else if (preferLongerForKnownTail(productions[parseTable[p.lhs][a]], p)) {
                } else if (preferExistingKnownTail(productions[parseTable[p.lhs][a]], p)) {
                    shouldWriteEntry = false;
                } else {
                    std::cerr << "[WARNING] LL(1) conflict: " << p.lhs << " on terminal '" << a
                              << "' (productions " << parseTable[p.lhs][a] << " and " << i << ")\n";
                    isLL1 = false;
                    shouldWriteEntry = false;
                }
            }
            if (shouldWriteEntry) {
                parseTable[p.lhs][a] = i;
                isEpsilonEntry[p.lhs][a] = thisIsEpsilon;
            }
        }

        if (firstAlpha.count("ε")) {
            for (auto& b : followSets[p.lhs]) {
                bool shouldWriteEntry = true;
                if (parseTable[p.lhs].count(b)) {
                    bool existingIsEps = isEpsilonEntry[p.lhs][b];
                    if (!existingIsEps && thisIsEpsilon) {
                        // Keep existing non-epsilon entry (dangling else resolution)
                        shouldWriteEntry = false;
                    } else if (existingIsEps && !thisIsEpsilon) {
                    } else if (preferLongerForKnownTail(productions[parseTable[p.lhs][b]], p)) {
                    } else if (preferExistingKnownTail(productions[parseTable[p.lhs][b]], p)) {
                        shouldWriteEntry = false;
                    } else {
                        std::cerr << "[WARNING] LL(1) conflict: " << p.lhs << " on terminal '" << b
                                  << "' (productions " << parseTable[p.lhs][b] << " and " << i << ")\n";
                        isLL1 = false;
                        shouldWriteEntry = false;
                    }
                }
                if (shouldWriteEntry) {
                    parseTable[p.lhs][b] = i;
                    isEpsilonEntry[p.lhs][b] = thisIsEpsilon;
                }
            }
        }
    }

    return isLL1;
}

void Grammar::printParseTable(std::ostream& os) const {
    // Collect all terminals that appear
    std::set<std::string> usedTerminals;
    for (auto pit = parseTable.begin(); pit != parseTable.end(); ++pit) {
        for (auto rit = pit->second.begin(); rit != pit->second.end(); ++rit) {
            usedTerminals.insert(rit->first);
        }
    }
    std::vector<std::string> termList(usedTerminals.begin(), usedTerminals.end());
    std::sort(termList.begin(), termList.end());

    // Collect non-terminals in order
    std::vector<std::string> ntList;
    std::set<std::string> seen;
    for (auto& p : productions) {
        if (!seen.count(p.lhs)) {
            ntList.push_back(p.lhs);
            seen.insert(p.lhs);
        }
    }

    // Header
    os << "LL(1) Prediction Parse Table\n";
    os << "============================\n\n";

    for (auto& nt : ntList) {
        auto it = parseTable.find(nt);
        if (it == parseTable.end()) continue;
        os << "[" << nt << "]\n";
        for (auto& t : termList) {
            auto it2 = it->second.find(t);
            if (it2 != it->second.end()) {
                int idx = it2->second;
                os << "  on '" << t << "' -> " << productions[idx].lhs << " ::= ";
                if (productions[idx].rhs.empty()) {
                    os << "ε";
                } else {
                    for (size_t k = 0; k < productions[idx].rhs.size(); k++) {
                        if (k) os << " ";
                        os << productions[idx].rhs[k];
                    }
                }
                os << "\n";
            }
        }
        os << "\n";
    }
}

// ==================== Build Pascal-S Grammar ====================

Grammar buildPascalSGrammar() {
    Grammar g;
    g.startSymbol = "programstruct";

    // Helper: split a string by spaces into symbols
    auto split = [](const std::string& s) -> std::vector<std::string> {
        std::vector<std::string> result;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok) {
            result.push_back(tok);
        }
        return result;
    };

    // 1. Program structure
    g.addProduction("programstruct", split("program_head ; program_body ."));
    g.addProduction("program_head", split("program id ( idlist )"));
    g.addProduction("program_head", split("program id"));
    g.addProduction("program_body", split("const_declarations var_declarations subprogram_declarations compound_statement"));
    g.addProduction("idlist", split("id idlist'"));
    g.addProduction("idlist'", split(", id idlist'"));
    g.addProduction("idlist'", {}); // ε -- pre-transform to avoid left recursion here

    // const_declarations -> ε | const const_decl_list
    // const_decl_list -> id = const_value ; const_decl_list_tail
    // const_decl_list_tail -> id = const_value ; const_decl_list_tail | ε
    // This absorbs the trailing ';' into the list so decision is on 'id' vs other
    g.addProduction("const_declarations", {}); // ε
    g.addProduction("const_declarations", split("const const_decl_list"));
    g.addProduction("const_decl_list", split("id = const_value ; const_decl_list_tail"));
    g.addProduction("const_decl_list_tail", split("id = const_value ; const_decl_list_tail"));
    g.addProduction("const_decl_list_tail", {}); // ε

    // const_value
    g.addProduction("const_value", split("+ num"));
    g.addProduction("const_value", split("- num"));
    g.addProduction("const_value", split("num"));
    g.addProduction("const_value", split("letter"));
    g.addProduction("const_value", split("string"));
    g.addProduction("const_value", split("true"));
    g.addProduction("const_value", split("false"));

    // 2. Variables & subprograms
    // var_declarations -> ε | var var_decl_list
    // var_decl_list -> idlist : type ; var_decl_list_tail
    // var_decl_list_tail -> idlist : type ; var_decl_list_tail | ε
    g.addProduction("var_declarations", {}); // ε
    g.addProduction("var_declarations", split("var var_decl_list"));
    g.addProduction("var_decl_list", split("idlist : type ; var_decl_list_tail"));
    g.addProduction("var_decl_list_tail", split("idlist : type ; var_decl_list_tail"));
    g.addProduction("var_decl_list_tail", {}); // ε

    g.addProduction("type", split("basic_type"));
    g.addProduction("type", split("array [ period ] of basic_type"));
    g.addProduction("type", split("record record_field_decl_list end"));

    g.addProduction("basic_type", split("integer"));
    g.addProduction("basic_type", split("real"));
    g.addProduction("basic_type", split("boolean"));
    g.addProduction("basic_type", split("char"));
    g.addProduction("basic_type", split("string_kw"));

    g.addProduction("record_field_decl_list", split("idlist : type ; record_field_decl_list_tail"));
    g.addProduction("record_field_decl_list_tail", split("idlist : type ; record_field_decl_list_tail"));
    g.addProduction("record_field_decl_list_tail", {});

    // period -> digits .. digits period'
    // period' -> , digits .. digits period' | ε
    g.addProduction("period", split("num .. num period'"));
    g.addProduction("period'", split(", num .. num period'"));
    g.addProduction("period'", {}); // ε

    // subprogram_declarations -> subprogram_declarations subprogram ; => eliminate left rec
    // subprogram_declarations -> ε | subprogram ; subprogram_declarations'
    // subprogram_declarations' -> subprogram ; subprogram_declarations' | ε
    g.addProduction("subprogram_declarations", {}); // ε
    g.addProduction("subprogram_declarations", split("subprogram ; subprogram_declarations"));

    g.addProduction("subprogram", split("subprogram_head ; subprogram_body"));

    g.addProduction("subprogram_head", split("procedure id formal_parameter"));
    g.addProduction("subprogram_head", split("function id formal_parameter : basic_type"));

    g.addProduction("formal_parameter", {}); // ε
    g.addProduction("formal_parameter", split("( parameter_list )"));

    // parameter_list -> parameter parameter_list'
    // parameter_list' -> ; parameter parameter_list' | ε
    g.addProduction("parameter_list", {});
    g.addProduction("parameter_list", split("parameter parameter_list'"));
    g.addProduction("parameter_list'", split("; parameter parameter_list'"));
    g.addProduction("parameter_list'", {}); // ε

    // 3. Statements
    g.addProduction("parameter", split("var_parameter"));
    g.addProduction("parameter", split("value_parameter"));

    g.addProduction("var_parameter", split("var value_parameter"));
    g.addProduction("value_parameter", split("idlist : basic_type"));

    g.addProduction("subprogram_body", split("const_declarations var_declarations compound_statement"));
    g.addProduction("compound_statement", split("begin statement_list end"));

    // statement_list -> statement statement_list'
    // statement_list' -> ; statement statement_list' | ε
    g.addProduction("statement_list", split("statement statement_list'"));
    g.addProduction("statement_list'", split("; statement statement_list'"));
    g.addProduction("statement_list'", {}); // ε

    // 4. Statement -- this is the most complex one
    // statement -> ε | variable assignop expression | func_id assignop expression
    //            | procedure_call | compound_statement
    //            | if expression then statement else_part
    //            | for id assignop expression to expression do statement
    //            | read ( variable_list ) | write ( expression_list )
    //
    // Since variable, func_id, procedure_call all start with 'id', we need to factor:
    // statement -> ε
    //            | id statement_id_tail
    //            | begin statement_list end   (compound_statement)
    //            | if expression then statement else_part
    //            | for id assignop expression to expression do statement
    //            | read ( variable_list )
    //            | write ( expression_list )
    //
    // statement_id_tail -> id_varpart assignop expression    (variable assign)
    //                    | ( expression_list )                (procedure call with args)
    //                    | assignop expression                (func_id assign or simple proc call)
    //                    | ε                                  (procedure call: just id)

    g.addProduction("statement", {}); // ε
    g.addProduction("statement", split("id statement_id_tail"));
    g.addProduction("statement", split("begin statement_list end"));
    g.addProduction("statement", split("if expression then statement else_part"));
    g.addProduction("statement", split("for id assignop expression for_direction expression do statement"));
    g.addProduction("statement", split("while expression do statement"));
    g.addProduction("statement", split("case expression of case_branch_list end"));
    g.addProduction("statement", split("break"));
    g.addProduction("statement", split("continue"));
    g.addProduction("statement", split("read ( variable_list )"));
    g.addProduction("statement", split("write ( expression_list )"));

    g.addProduction("for_direction", split("to"));
    g.addProduction("for_direction", split("downto"));

    g.addProduction("case_branch_list", split("case_branch case_branch_list'"));
    g.addProduction("case_branch_list'", split("; case_branch case_branch_list'"));
    g.addProduction("case_branch_list'", split(";"));
    g.addProduction("case_branch_list'", {}); // 蔚
    g.addProduction("case_branch", split("case_label_list : statement"));
    g.addProduction("case_label_list", split("const_value case_label_list'"));
    g.addProduction("case_label_list'", split(", const_value case_label_list'"));
    g.addProduction("case_label_list'", {}); // 蔚

    // statement_id_tail handles the ambiguity of id starting variable/func_id/procedure_call
    g.addProduction("statement_id_tail", split("id_varpart assignop expression"));
    g.addProduction("statement_id_tail", split("( expression_list )"));          // procedure call with args
    // direct assignment is covered by id_varpart -> epsilon
    g.addProduction("statement_id_tail", {}); // ε -- bare procedure call (just id)

    // variable_list -> variable variable_list'
    // variable_list' -> , variable variable_list' | ε
    g.addProduction("variable_list", split("variable variable_list'"));
    g.addProduction("variable_list'", split(", variable variable_list'"));
    g.addProduction("variable_list'", {}); // ε

    g.addProduction("variable", split("id id_varpart"));
    g.addProduction("id_varpart", split("field_chain"));
    g.addProduction("id_varpart", {}); // ε
    g.addProduction("id_varpart", split("[ expression_list ] field_chain"));
    g.addProduction("field_chain", split(". id field_chain"));
    g.addProduction("field_chain", {});

    // 5. Expressions
    g.addProduction("else_part", {}); // ε
    g.addProduction("else_part", split("else statement"));

    // expression_list -> expression expression_list'
    // expression_list' -> , expression expression_list' | ε
    g.addProduction("expression_list", {}); // 蔚
    g.addProduction("expression_list", split("expression expression_list'"));
    g.addProduction("expression_list'", split(", expression expression_list'"));
    g.addProduction("expression_list'", {}); // ε

    // expression -> simple_expression expression'
    // expression' -> relop simple_expression | ε
    g.addProduction("expression", split("simple_expression expression_tail"));
    g.addProduction("expression_tail", split("relop simple_expression"));
    g.addProduction("expression_tail", {}); // ε

    // simple_expression -> term simple_expression'
    // simple_expression' -> addop term simple_expression' | ε
    g.addProduction("simple_expression", split("term simple_expression'"));
    g.addProduction("simple_expression'", split("addop term simple_expression'"));
    g.addProduction("simple_expression'", {}); // ε

    // term -> factor term'
    // term' -> mulop factor term' | ε
    g.addProduction("term", split("factor term'"));
    g.addProduction("term'", split("mulop factor term'"));
    g.addProduction("term'", {}); // ε

    // factor -> num | variable | ( expression ) | id ( expression_list ) | not factor | uminus factor
    // variable = id id_varpart, and id ( expression_list ) also starts with id
    // So factor with id prefix needs factoring:
    // factor -> num | id factor_id_tail | ( expression ) | not factor | - factor
    // factor_id_tail -> id_varpart | ( expression_list )
    // id_varpart -> ε | [ expression_list ]
    g.addProduction("factor", split("num"));
    g.addProduction("factor", split("string"));
    g.addProduction("factor", split("letter"));
    g.addProduction("factor", split("true"));
    g.addProduction("factor", split("false"));
    g.addProduction("factor", split("id factor_id_tail"));
    g.addProduction("factor", split("( expression )"));
    g.addProduction("factor", split("not factor"));
    g.addProduction("factor", split("+ factor"));
    g.addProduction("factor", split("- factor"));

    g.addProduction("factor_id_tail", split("id_varpart"));
    g.addProduction("factor_id_tail", split("( expression_list )"));
    g.addProduction("factor_id_tail", {}); // ε (just a variable = id)

    g.classifySymbols();
    return g;
}
