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
    if (category == Category::Record) {
        std::string result = "record{";
        for (size_t i = 0; i < recordFields.size(); ++i) {
            if (i > 0) result += ";";
            result += recordFields[i].first + ":" + recordFields[i].second.toString();
        }
        result += "}";
        return result;
    }
    return baseType;
}

const TypeInfo* TypeInfo::findField(const std::string& fieldName) const {
    for (const auto& field : recordFields) {
        if (field.first == fieldName) return &field.second;
    }
    return nullptr;
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

TypeInfo TypeInfo::makeRecord(const std::vector<std::pair<std::string, TypeInfo>>& fields) {
    TypeInfo t;
    t.category = Category::Record;
    t.baseType = "record";
    t.recordFields = fields;
    return t;
}

TypeInfo TypeInfo::fromString(const std::string& typeStr) {
    auto trim = [](const std::string& text) {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string{};
        const auto last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    };

    auto splitTopLevel = [](const std::string& text, char delimiter) {
        std::vector<std::string> parts;
        int bracketDepth = 0;
        int braceDepth = 0;
        size_t start = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '[') ++bracketDepth;
            else if (text[i] == ']') --bracketDepth;
            else if (text[i] == '{') ++braceDepth;
            else if (text[i] == '}') --braceDepth;
            else if (text[i] == delimiter && bracketDepth == 0 && braceDepth == 0) {
                parts.push_back(text.substr(start, i - start));
                start = i + 1;
            }
        }
        parts.push_back(text.substr(start));
        return parts;
    };

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

            // Parse dimensions recursively so array[a..b, c..d] of T becomes
            // array[a..b] of array[c..d] of T.
            auto dotdot = range.find("..");
            if (dotdot != std::string::npos) {
                auto comma = range.find(',', dotdot);
                int low  = std::stoi(range.substr(0, dotdot));
                int high = 0;
                std::string nestedElementType = trim(elemType);
                if (comma != std::string::npos) {
                    high = std::stoi(range.substr(dotdot + 2, comma - dotdot - 2));
                    std::string remainingRange = trim(range.substr(comma + 1));
                    nestedElementType = "array[" + remainingRange + "] of " + nestedElementType;
                } else {
                    high = std::stoi(range.substr(dotdot + 2));
                }
                return makeArray(low, high, nestedElementType);
            }
        }
    }
    if (typeStr.size() >= 8 && typeStr.substr(0, 7) == "record{" && typeStr.back() == '}') {
        std::string body = typeStr.substr(7, typeStr.size() - 8);
        std::vector<std::pair<std::string, TypeInfo>> fields;
        for (const auto& rawField : splitTopLevel(body, ';')) {
            auto fieldDecl = trim(rawField);
            if (fieldDecl.empty()) continue;
            auto colonPos = fieldDecl.find(':');
            if (colonPos == std::string::npos) continue;
            std::string fieldName = trim(fieldDecl.substr(0, colonPos));
            std::string fieldType = trim(fieldDecl.substr(colonPos + 1));
            fields.push_back({fieldName, fromString(fieldType)});
        }
        return makeRecord(fields);
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

Symbol* SymbolTable::lookupCallable(const std::string& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end() &&
            (it->second.kind == SymbolKind::Function || it->second.kind == SymbolKind::Procedure)) {
            return &it->second;
        }
    }
    return nullptr;
}

Symbol* SymbolTable::lookupFunction(const std::string& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end() && it->second.kind == SymbolKind::Function) {
            return &it->second;
        }
    }
    return nullptr;
}
