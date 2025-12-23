#ifndef SEMANTICPARSER_HPP
#define SEMANTICPARSER_HPP
#include <vector>
#include <string>
#include <sstream>
#include "visitor.hpp"
#include "nodes.hpp"


class SemanticParser : Visitor{
private:
    std::vector<std::string> indents;
    std::vector<std::string> prefixes;

    /* Helper function to print a string with the current indentation */
    void print_indented(const std::string &str);

    /* Functions to manage the indentation level */
    void enter_child();

    void enter_last_child();

    void leave_child();

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
