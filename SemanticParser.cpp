#include "SemanticParser.hpp"
#include <iostream>

using ast::BuiltInType;

void SemanticParser::print() const {
    std::cout << printer;
}

SemanticParser::SemanticParser() {
    // global scope exists as a map, but we don't printer.beginScope() for global.
    // operator<< already prints ---begin global scope--- and uses globalsBuffer for funcs.
    scopes.push_back({});
    insertFunc("print",  BuiltInType::VOID, {BuiltInType::STRING}, 0);
    insertFunc("printi", BuiltInType::VOID, {BuiltInType::INT},    0);
}

void SemanticParser::pushScope() {
    scopes.push_back({});
    printer.beginScope();
    scopeOffsetStack.push_back(nextLocalOffset);
}

void SemanticParser::popScope() {
    printer.endScope();
    scopes.pop_back();
    if (!scopeOffsetStack.empty()) {
        nextLocalOffset = scopeOffsetStack.back();
        scopeOffsetStack.pop_back();
    }
}

bool SemanticParser::existsInAnyScope(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) return true;
    }
    return false;
}

SymbolEntry* SemanticParser::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

bool SemanticParser::existsInCurrentScope(const std::string& name) const {
    return scopes.back().find(name) != scopes.back().end();
}

void SemanticParser::insertVar(const std::string& name, BuiltInType type, int offset, int lineno) {
    if (existsInAnyScope(name)) {
        output::errorDef(lineno, name);
    }
    SymbolEntry e;
    e.name = name;
    e.isFunc = false;
    e.type = type;
    e.offset = offset;

    scopes.back()[name] = e;
    printer.emitVar(name, type, offset);
}

void SemanticParser::insertFunc(const std::string& name, BuiltInType ret,
                               const std::vector<BuiltInType>& params,
                               int lineno) {
    if (scopes.front().find(name) != scopes.front().end()) {
        output::errorDef(lineno, name);
    }
    SymbolEntry e;
    e.name = name;
    e.isFunc = true;
    e.type = ret;
    e.paramTypes = params;

    scopes.front()[name] = e; // force into global
    printer.emitFunc(name, ret, params);
}

bool SemanticParser::isNumeric(BuiltInType t) {
    return t == BuiltInType::INT || t == BuiltInType::BYTE;
}
bool SemanticParser::isBool(BuiltInType t) {
    return t == BuiltInType::BOOL;
}
bool SemanticParser::isString(BuiltInType t) {
    return t == BuiltInType::STRING;
}

bool SemanticParser::canAssign(BuiltInType dst, BuiltInType src) {
    if (dst == src) return true;
    // implicit widening: byte -> int
    if (dst == BuiltInType::INT && src == BuiltInType::BYTE) return true;
    return false;
}

BuiltInType SemanticParser::widenNumeric(BuiltInType a, BuiltInType b) {
    if (a == BuiltInType::INT || b == BuiltInType::INT) return BuiltInType::INT;
    return BuiltInType::BYTE; // both byte
}

void SemanticParser::ensureMainExists(const ast::Funcs& root) {
    // must have: void main()  (no params)
    auto* e = lookup("main");
    if (!e || !e->isFunc) {
        output::errorMainMissing();
    }
    if (e->type != BuiltInType::VOID) {
        output::errorMainMissing();
    }
    if (!e->paramTypes.empty()) {
        output::errorMainMissing();
    }
}

// -------------------- Visitors --------------------

void SemanticParser::visit(ast::Funcs &node) {
    // PASS 1: declare prototypes in global scope
    for (auto &f : node.funcs) {
        auto fname = f->id->value;

        std::vector<BuiltInType> params;
        for (auto &p : f->formals->formals) {
            params.push_back(p->type->type);
        }
        BuiltInType ret = f->return_type->type;
        insertFunc(fname, ret, params, f->id->line);
    }

    // main check (after prototypes exist)
    ensureMainExists(node);

    // PASS 2: analyze each function body
    for (auto &f : node.funcs) {
        f->accept(*this);
    }
}

void SemanticParser::visit(ast::FuncDecl &node) {
    insideFunction = true;
    currentFuncReturn = node.return_type->type;

    // reset offsets per function
    nextLocalOffset = 0;
    nextParamOffset = -1;

    // Enter function scope
    pushScope();

    // Insert parameters into function scope with negative offsets
    for (auto &p : node.formals->formals) {
        const std::string& pname = p->id->value;
        BuiltInType ptype = p->type->type;
        insertVar(pname, ptype, nextParamOffset, p->line);
        nextParamOffset--;
    }

    // Visit body statements (already within function scope)
    // Prevent "extra scope" wrapping for this body Statements node:
    bool prev = statementsAlreadyScoped;
    statementsAlreadyScoped = true;
    node.body->accept(*this);
    statementsAlreadyScoped = prev;

    // Leave function scope
    popScope();

    insideFunction = false;
    currentFuncReturn = BuiltInType::VOID;
}

void SemanticParser::visit(ast::Formals &node) {
    // not used directly for semantics in this design (handled in FuncDecl)
    (void)node;
}

void SemanticParser::visit(ast::Formal &node) {
    // not used directly for semantics in this design (handled in FuncDecl)
    (void)node;
}

void SemanticParser::visit(ast::Statements &node) {
    // If this Statements is a "block statement" we normally want scope,
    // BUT for function body we already pushed scope; we avoid double-scope using flag.
    if (!statementsAlreadyScoped) {
        pushScope();
    }

    for (auto &st : node.statements) {
        visitStatementPossiblyBlock(st, false);
    }

    if (!statementsAlreadyScoped) {
        popScope();
    }
}

void SemanticParser::visitStatementPossiblyBlock(const std::shared_ptr<ast::Statement>& st,
                                                 bool forceScopeForSingleStmt) {
    auto asBlock = std::dynamic_pointer_cast<ast::Statements>(st);
    if (asBlock) {
        // A real block: it should introduce exactly ONE scope (handled here)
        pushScope();
        bool prev = statementsAlreadyScoped;
        statementsAlreadyScoped = true;
        asBlock->accept(*this);
        statementsAlreadyScoped = prev;
        popScope();
        return;
    }

    if (forceScopeForSingleStmt) {
        pushScope();
        st->accept(*this);
        popScope();
    } else {
        st->accept(*this);
    }
}


void SemanticParser::visit(ast::VarDecl &node) {
    BuiltInType t = node.type->type;
    const std::string& name = node.id->value;

    // insert first (so init can refer? depends on spec; usually init can refer to earlier vars, not itself)
    int off = nextLocalOffset++;
    insertVar(name, t, off, node.id->line);

    if (node.init_exp) {
        node.init_exp->accept(*this);
        BuiltInType initT = lastType;
        if (!canAssign(t, initT)) {
            output::errorMismatch(node.line);
        }
    }
}

void SemanticParser::visit(ast::Assign &node) {
    // left must be var
    auto* e = lookup(node.id->value);
    if (!e) {
        output::errorUndef(node.line, node.id->value);
    }
    if (e->isFunc) {
        output::errorDefAsFunc(node.line, node.id->value);
    }

    node.exp->accept(*this);
    BuiltInType rhs = lastType;

    if (!canAssign(e->type, rhs)) {
        output::errorMismatch(node.line);
    }
}

void SemanticParser::visit(ast::ID &node) {
    auto* e = lookup(node.value);
    if (!e) {
        output::errorUndef(node.line, node.value);
    }
    if (e->isFunc) {
        output::errorDefAsFunc(node.line, node.value);
    }
    lastType = e->type;
}

void SemanticParser::visit(ast::Call &node) {
    auto* e = lookup(node.func_id->value);
    if (!e) {
        output::errorUndefFunc(node.line, node.func_id->value);
    }
    if (!e->isFunc) {
        output::errorDefAsVar(node.line, node.func_id->value);
    }

    // collect actual arg types
    std::vector<BuiltInType> actuals;
    for (auto &a : node.args->exps) {
        a->accept(*this);
        actuals.push_back(lastType);
    }

    // arity check
    if (actuals.size() != e->paramTypes.size()) {
        // needs vector<string> in errorPrototypeMismatch
        std::vector<std::string> expectedStr;
        expectedStr.reserve(e->paramTypes.size());
        for (auto pt : e->paramTypes) {
            // convert using same mapping as output.cpp
            switch (pt) {
                case BuiltInType::INT: expectedStr.push_back("int"); break;
                case BuiltInType::BYTE: expectedStr.push_back("byte"); break;
                case BuiltInType::BOOL: expectedStr.push_back("bool"); break;
                case BuiltInType::STRING: expectedStr.push_back("string"); break;
                case BuiltInType::VOID: expectedStr.push_back("void"); break;
            }
        }
        output::errorPrototypeMismatch(node.line, e->name, expectedStr);
    }

    // type check per arg
    for (size_t i = 0; i < actuals.size(); ++i) {
        if (!canAssign(e->paramTypes[i], actuals[i])) {
            std::vector<std::string> expectedStr;
            expectedStr.reserve(e->paramTypes.size());
            for (auto pt : e->paramTypes) {
                switch (pt) {
                    case BuiltInType::INT: expectedStr.push_back("int"); break;
                    case BuiltInType::BYTE: expectedStr.push_back("byte"); break;
                    case BuiltInType::BOOL: expectedStr.push_back("bool"); break;
                    case BuiltInType::STRING: expectedStr.push_back("string"); break;
                    case BuiltInType::VOID: expectedStr.push_back("void"); break;
                }
            }
            output::errorPrototypeMismatch(node.line, e->name, expectedStr);
        }
    }

    // call expression type is function return type
    lastType = e->type;
}

void SemanticParser::visit(ast::Return &node) {
    if (!insideFunction) {
        // shouldn't happen in valid parse tree, but keep safe
        output::errorMismatch(node.line);
    }

    if (!node.exp) {
        if (currentFuncReturn != BuiltInType::VOID) {
            output::errorMismatch(node.line);
        }
        return;
    }

    node.exp->accept(*this);
    BuiltInType rt = lastType;

    if (!canAssign(currentFuncReturn, rt)) {
        output::errorMismatch(node.line);
    }
}

void SemanticParser::visit(ast::If &node) {
    node.condition->accept(*this);
    if (lastType != BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }

    pushScope();
    visitStatementPossiblyBlock(node.then, false);
    popScope();

    if (node.otherwise) {
        pushScope();
        visitStatementPossiblyBlock(node.otherwise, false);
        popScope();
    }
}

void SemanticParser::visit(ast::While &node) {
    node.condition->accept(*this);
    if (lastType != BuiltInType::BOOL) {
        output::errorMismatch(node.condition->line);
    }

    pushScope();
    whileDepth++;
    visitStatementPossiblyBlock(node.body, false);
    whileDepth--;
    popScope();
}

void SemanticParser::visit(ast::Break &node) {
    if (whileDepth <= 0) {
        output::errorUnexpectedBreak(node.line);
    }
}

void SemanticParser::visit(ast::Continue &node) {
    if (whileDepth <= 0) {
        output::errorUnexpectedContinue(node.line);
    }
}

// ----------------- Expressions -----------------

void SemanticParser::visit(ast::Num &node) {
    (void)node;
    lastType = BuiltInType::INT;
}

void SemanticParser::visit(ast::NumB &node) {
    if (node.value < 0 || node.value > 255) {
        output::errorByteTooLarge(node.line, node.value);
    }
    lastType = BuiltInType::BYTE;
}

void SemanticParser::visit(ast::String &node) {
    (void)node;
    lastType = BuiltInType::STRING;
}

void SemanticParser::visit(ast::Bool &node) {
    (void)node;
    lastType = BuiltInType::BOOL;
}

void SemanticParser::visit(ast::BinOp &node) {
    node.left->accept(*this);
    BuiltInType l = lastType;
    node.right->accept(*this);
    BuiltInType r = lastType;

    if (!isNumeric(l) || !isNumeric(r)) {
        output::errorMismatch(node.line);
    }

    // division usually forces INT (depending on spec); here we widen normally
    lastType = widenNumeric(l, r);
}

void SemanticParser::visit(ast::RelOp &node) {
    node.left->accept(*this);
    BuiltInType l = lastType;
    node.right->accept(*this);
    BuiltInType r = lastType;

    if (isNumeric(l) && isNumeric(r)) {
        lastType = BuiltInType::BOOL;
        return;
    }

    output::errorMismatch(node.line);
}

void SemanticParser::visit(ast::Not &node) {
    node.exp->accept(*this);
    if (lastType != BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    lastType = BuiltInType::BOOL;
}

void SemanticParser::visit(ast::And &node) {
    node.left->accept(*this);
    BuiltInType l = lastType;
    node.right->accept(*this);
    BuiltInType r = lastType;

    if (l != BuiltInType::BOOL || r != BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    lastType = BuiltInType::BOOL;
}

void SemanticParser::visit(ast::Or &node) {
    node.left->accept(*this);
    BuiltInType l = lastType;
    node.right->accept(*this);
    BuiltInType r = lastType;

    if (l != BuiltInType::BOOL || r != BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    lastType = BuiltInType::BOOL;
}

void SemanticParser::visit(ast::Type &node) {
    // Not an expression; usually no-op
    (void)node;
}

void SemanticParser::visit(ast::Cast &node) {
    node.exp->accept(*this);
    BuiltInType src = lastType;
    BuiltInType dst = node.target_type->type;

    // Typical rule: only numeric casts between byte/int allowed
    if (isNumeric(src) && isNumeric(dst)) {
        lastType = dst;
        return;
    }

    output::errorMismatch(node.line);
}

void SemanticParser::visit(ast::ExpList &node) {
    // Not used directly (Call iterates args)
    (void)node;
}
