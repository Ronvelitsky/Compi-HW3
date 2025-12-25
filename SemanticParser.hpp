#ifndef SEMANTICPARSER_HPP
#define SEMANTICPARSER_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

#include "visitor.hpp"
#include "nodes.hpp"
#include "output.hpp"

struct SymbolEntry {
    std::string name;
    bool isFunc = false;

    // For vars: var type
    // For funcs: return type
    ast::BuiltInType type = ast::BuiltInType::VOID;

    // Only for funcs
    std::vector<ast::BuiltInType> paramTypes;

    // Only for vars/params
    int offset = 0;
};

class SemanticParser : public Visitor {
public:
    SemanticParser();

    // Let main() print scopes
    const output::ScopePrinter& getPrinter() const { return printer; }
    void print() const;
    // Visitor overrides
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

private:
    // ----- Scopes -----
    std::vector<std::unordered_map<std::string, SymbolEntry>> scopes;

    // Printing
    output::ScopePrinter printer;

    // Current function context
    ast::BuiltInType currentFuncReturn = ast::BuiltInType::VOID;
    bool insideFunction = false;

    // break/continue validity
    int whileDepth = 0;

    // offsets for current function
    int nextLocalOffset = 0;   // 0,1,2,...
    int nextParamOffset = -1;  // -1,-2,-3,...

    // Expression type "return channel"
    ast::BuiltInType lastType = ast::BuiltInType::VOID;

    // A flag to avoid double-scoping the same Statements node
    bool statementsAlreadyScoped = false;

private:
    // Scope helpers
    void pushScope();
    void popScope();

    bool existsInAnyScope(const std::string& name) const;
    bool existsInCurrentScope(const std::string& name) const;
    SymbolEntry* lookup(const std::string& name);

    void insertVar(const std::string& name, ast::BuiltInType type, int offset, int lineno);
    void insertFunc(const std::string& name, ast::BuiltInType ret,
                    const std::vector<ast::BuiltInType>& params,
                    int lineno);

    // Type helpers
    static bool isNumeric(ast::BuiltInType t);
    static bool isBool(ast::BuiltInType t);
    static bool isString(ast::BuiltInType t);

    static bool canAssign(ast::BuiltInType dst, ast::BuiltInType src); // allow byte->int
    static ast::BuiltInType widenNumeric(ast::BuiltInType a, ast::BuiltInType b); // byte+int => int

    // Visit a statement that might be a block (Statements-as-Statement) and should open scope
    void visitStatementPossiblyBlock(const std::shared_ptr<ast::Statement>& st, bool forceScopeForSingleStmt = false);

    // Final checks
    void ensureMainExists(const ast::Funcs& root);
};

#endif
