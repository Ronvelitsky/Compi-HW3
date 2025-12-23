#ifndef SEMANTICPARSER_HPP
#define SEMANTICPARSER_HPP
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include "visitor.hpp"
#include "nodes.hpp"


struct SymbolTableEntry {
    std::string name;
    int offset;
    ast::BuiltInType type;
    bool is_function;
};

class SymbolTable {
private:
    std::unordered_map<std::string, Entry> entries;
    int current_offset;

public:
    SymbolTable() : current_offset(0) {}

    void insert(const std::string& name, ast::BuiltInType type, bool is_function = false) {
        SymbolTableEntry entry;
        entry.name = name;
        entry.offset = current_offset++;
        entry.type = type;
        entry.is_function = is_function;
        entries[name] = entry;
    }

    void remove(const std::string& name) {
        entries.erase(name);
    }

    SymbolTableEntry* lookup(const std::string& name) {
        auto it = entries.find(name);
        if (it != entries.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    bool exists(const std::string& name) const {
        return entries.find(name) != entries.end();
    }
};

class SemanticParser : Visitor{
private:
    SymbolTable symbolTable;

public:
    SemanticParser();

    void visit(ast::Num &node) override;

    void visit(ast::NumB &node) override;

    void visit(ast::String &node) override;

    void visit(ast::Bool &node) override;

    void visit(ast::ID &node) override;

    void visit(ast::BinOp &node) override;

    void visit(ast::RelOp &node) override;

    void visit(ast::Not &node) override;

    void visit(ast::And &node) override;

    void visit(ast::Or &node) override;

    void visit(ast::Type &node) override;

    void visit(ast::Cast &node) override;

    void visit(ast::ExpList &node) override;

    void visit(ast::Call &node) override;

    void visit(ast::Statements &node) override;

    void visit(ast::Break &node) override;

    void visit(ast::Continue &node) override;

    void visit(ast::Return &node) override;

    void visit(ast::If &node) override;

    void visit(ast::While &node) override;

    void visit(ast::VarDecl &node) override;

    void visit(ast::Assign &node) override;

    void visit(ast::Formal &node) override;

    void visit(ast::Formals &node) override;

    void visit(ast::FuncDecl &node) override;

    void visit(ast::Funcs &node) override;
};


#endif
