#include "symbol_table.h"
#include <sstream>

// ---------------------------------------------------------------------------
// TypeInfo
// ---------------------------------------------------------------------------
std::string TypeInfo::toString() const {
    if (category == Category::Array) {
        return "array[" + std::to_string(arrayLow) + ".." +
               std::to_string(arrayHigh) + "] of " + elementType;
    }
    return baseType;
}

TypeInfo TypeInfo::makeSimple(const std::string& base) {
    TypeInfo t;
    t.category  = Category::Simple;
    t.baseType  = base;
    return t;
}

TypeInfo TypeInfo::makeArray(int low, int high, const std::string& elemType) {
    TypeInfo t;
    t.category    = Category::Array;
    t.baseType    = "array";
    t.arrayLow    = low;
    t.arrayHigh   = high;
    t.elementType = elemType;
    return t;
}

TypeInfo TypeInfo::fromString(const std::string& typeStr) {
    // Format: "array[low..high] of elemType"  or  "integer" / "real" / ...
    if (typeStr.size() >= 5 && typeStr.substr(0, 5) == "array") {
        auto bracketStart = typeStr.find('[');
        auto bracketEnd   = typeStr.find(']');
        auto ofPos        = typeStr.find(" of ");
        if (bracketStart != std::string::npos &&
            bracketEnd   != std::string::npos &&
            ofPos        != std::string::npos)
        {
            std::string range    = typeStr.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
            std::string elemType = typeStr.substr(ofPos + 4);

            // Parse first dimension  "low..high"
            auto dotdot = range.find("..");
            if (dotdot != std::string::npos) {
                // Handle possible multi-dimension (take first dimension only)
                auto comma = range.find(',', dotdot);
                int low  = std::stoi(range.substr(0, dotdot));
                int high;
                if (comma != std::string::npos)
                    high = std::stoi(range.substr(dotdot + 2, comma - dotdot - 2));
                else
                    high = std::stoi(range.substr(dotdot + 2));
                return makeArray(low, high, elemType);
            }
        }
    }
    return makeSimple(typeStr);
}

// ---------------------------------------------------------------------------
// SymbolTable
// ---------------------------------------------------------------------------
void SymbolTable::enterScope() {
    scopes_.emplace_back();
}

void SymbolTable::exitScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool SymbolTable::declare(const Symbol& sym) {
    if (scopes_.empty()) return false;
    auto& current = scopes_.back();
    if (current.count(sym.name)) return false;   // duplicate
    Symbol s     = sym;
    s.scopeLevel = getCurrentLevel();
    current[s.name] = s;
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) return &it->second;
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrent(const std::string& name) {
    if (scopes_.empty()) return nullptr;
    auto it = scopes_.back().find(name);
    if (it != scopes_.back().end()) return &it->second;
    return nullptr;
}
