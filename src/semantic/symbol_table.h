#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// TypeInfo — describes a Pascal-S type (simple or array)
// ---------------------------------------------------------------------------
struct TypeInfo {
    enum class Category { Simple, Array, Record };
    Category category = Category::Simple;
    std::string baseType;      // "integer", "real", "boolean", "char"
    // Array-specific
    int arrayLow  = 0;
    int arrayHigh = 0;
    std::string elementType;   // element base type for arrays
    std::vector<std::pair<std::string, TypeInfo>> recordFields;

    bool isNumeric()  const { return baseType == "integer" || baseType == "real"; }
    bool isInteger()  const { return category == Category::Simple && baseType == "integer"; }
    bool isReal()     const { return category == Category::Simple && baseType == "real"; }
    bool isBoolean()  const { return category == Category::Simple && baseType == "boolean"; }
    bool isChar()     const { return category == Category::Simple && baseType == "char"; }
    bool isArray()    const { return category == Category::Array; }
    bool isRecord()   const { return category == Category::Record; }

    std::string toString() const;
    const TypeInfo* findField(const std::string& fieldName) const;

    // Parse a type string produced by the parser, e.g. "integer" or "array[0..5] of integer"
    static TypeInfo fromString(const std::string& typeStr);
    static TypeInfo makeSimple(const std::string& base);
    static TypeInfo makeArray(int low, int high, const std::string& elemType);
    static TypeInfo makeRecord(const std::vector<std::pair<std::string, TypeInfo>>& fields);
};

// ---------------------------------------------------------------------------
// Symbol — a single entry in the symbol table
// ---------------------------------------------------------------------------
enum class SymbolKind { Constant, Variable, Parameter, Function, Procedure, Program };

struct ParamInfo {
    std::string name;
    TypeInfo type;
    bool byReference = false;
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    TypeInfo type;
    int scopeLevel = 0;
    bool byReference = false;          // meaningful for Parameter symbols

    // Function / Procedure specific
    std::vector<ParamInfo> params;
    std::string returnType;            // non-empty for functions

    // Constant specific
    std::string constValue;
};

// ---------------------------------------------------------------------------
// SymbolTable — stack-of-scopes implementation
// ---------------------------------------------------------------------------
class SymbolTable {
public:
    void enterScope();
    void exitScope();
    bool declare(const Symbol& sym);
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrent(const std::string& name);
    Symbol* lookupCallable(const std::string& name);
    Symbol* lookupFunction(const std::string& name);
    int getCurrentLevel() const { return static_cast<int>(scopes_.size()) - 1; }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};

#endif
