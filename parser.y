%{

#include "nodes.hpp"
#include "output.hpp"

extern int yylineno;
extern int yylex();

void yyerror(const char*);

std::shared_ptr<ast::Node> program;

using namespace std;

// TODO: Place any additional declarations here
%}
%define api.value.type {std::shared_ptr<ast::Node>}
// TODO: Define tokens here

%token VOID INT BYTE BOOL AND OR NOT TRUE FALSE RETURN IF ELSE WHILE BREAK CONTINUE
%token SC COMMA LPAREN RPAREN LBRACE RBRACE LBRACK RBRACK ASSIGN COMMENT
%token ID NUM NUM_B STRING
%token ADD
%token SUB
%token MUL
%token DIV
%token EQ
%token NE
%token LT
%token GT
%token LE
%token GE

// TODO: Define precedence and associativity here

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%left OR
%left AND
%nonassoc EQ NE LT GT LE GE
%left ADD SUB
%left MUL DIV
%right NOT

%start Program

%%

Program: Funcs { program = $1; }
;

// TODO: Define grammar here

Funcs:
    { $$ = std::make_shared<ast::Funcs>(); }
  | FuncDecl Funcs
    {
        auto funcs = std::dynamic_pointer_cast<ast::Funcs>($2);
        auto f = std::dynamic_pointer_cast<ast::FuncDecl>($1);
        funcs->push_front(f);
        $$ = funcs;
    }
;


FuncDecl: RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    { $$ = std::make_shared<ast::FuncDecl>(
        std::dynamic_pointer_cast<ast::ID>($2),
        std::dynamic_pointer_cast<ast::Type>($1),
        std::dynamic_pointer_cast<ast::Formals>($4),
        std::dynamic_pointer_cast<ast::Statements>($7)
    ); }
;

RetType: Type { $$ = $1; }
       | VOID { $$ = std::make_shared<ast::Type>(ast::BuiltInType::VOID); }
;

Formals: { $$ = std::make_shared<ast::Formals>(); }
       | FormalsList { $$ = $1; }
;

FormalsList:
    FormalDecl
    {
        auto f = std::dynamic_pointer_cast<ast::Formal>($1);
        $$ = std::make_shared<ast::Formals>(f);
    }
  | FormalDecl COMMA FormalsList
    {
        auto f = std::dynamic_pointer_cast<ast::Formal>($1);
        auto list = std::dynamic_pointer_cast<ast::Formals>($3);
        list->push_front(f);
        $$ = list;
    }
;


FormalDecl: Type ID
    { $$ = std::make_shared<ast::Formal>(
        std::dynamic_pointer_cast<ast::ID>($2),
        std::dynamic_pointer_cast<ast::Type>($1)
    ); }
;

Statements:
      /* empty */
    {
        $$ = std::make_shared<ast::Statements>();
    }
    | Statements Statement
    {
        auto list = std::dynamic_pointer_cast<ast::Statements>($1);
        auto s = std::dynamic_pointer_cast<ast::Statement>($2);
        list->push_back(s);
        $$ = list;
    }
;


Statement: LBRACE Statements RBRACE
    { $$ = std::dynamic_pointer_cast<ast::Statements>($2); }
         | Type ID SC
    { auto t = std::dynamic_pointer_cast<ast::Type>($1);
      auto id = std::dynamic_pointer_cast<ast::ID>($2);
      $$ = std::make_shared<ast::VarDecl>(id, t, nullptr); }
         | Type ID ASSIGN Exp SC
    { auto t = std::dynamic_pointer_cast<ast::Type>($1);
      auto id = std::dynamic_pointer_cast<ast::ID>($2);
      auto exp = std::dynamic_pointer_cast<ast::Exp>($4);
      $$ = std::make_shared<ast::VarDecl>(id, t, exp); }
         | ID ASSIGN Exp SC
    { auto id = std::dynamic_pointer_cast<ast::ID>($1);
      auto exp = std::dynamic_pointer_cast<ast::Exp>($3);
      $$ = std::make_shared<ast::Assign>(id, exp); }
         | Call SC
    { $$ = std::dynamic_pointer_cast<ast::Statement>($1); }
         | RETURN SC
    { $$ = std::make_shared<ast::Return>(nullptr); }
         | RETURN Exp SC
    { auto exp = std::dynamic_pointer_cast<ast::Exp>($2);
      $$ = std::make_shared<ast::Return>(exp); }
         | IF LPAREN Exp RPAREN Statement %prec LOWER_THAN_ELSE
    { auto cond = std::dynamic_pointer_cast<ast::Exp>($3);
      auto then_s = std::dynamic_pointer_cast<ast::Statement>($5);
      $$ = std::make_shared<ast::If>(cond, then_s, nullptr); }
         | IF LPAREN Exp RPAREN Statement ELSE Statement
    { auto cond = std::dynamic_pointer_cast<ast::Exp>($3);
      auto then_s = std::dynamic_pointer_cast<ast::Statement>($5);
      auto else_s = std::dynamic_pointer_cast<ast::Statement>($7);
      $$ = std::make_shared<ast::If>(cond, then_s, else_s); }
         | WHILE LPAREN Exp RPAREN Statement
    { auto cond = std::dynamic_pointer_cast<ast::Exp>($3);
      auto body = std::dynamic_pointer_cast<ast::Statement>($5);
      $$ = std::make_shared<ast::While>(cond, body); }
         | BREAK SC
    { $$ = std::make_shared<ast::Break>(); }
         | CONTINUE SC
    { $$ = std::make_shared<ast::Continue>(); }
;

Call: ID LPAREN ExpList RPAREN
    { auto id = std::dynamic_pointer_cast<ast::ID>($1);
      auto args = std::dynamic_pointer_cast<ast::ExpList>($3);
      $$ = std::make_shared<ast::Call>(id, args); }
    | ID LPAREN RPAREN
    { auto id = std::dynamic_pointer_cast<ast::ID>($1);
      $$ = std::make_shared<ast::Call>(id); }
;

ExpList: Exp
    { auto e = std::dynamic_pointer_cast<ast::Exp>($1);
      $$ = std::make_shared<ast::ExpList>(e); }
       | Exp COMMA ExpList
    { auto e = std::dynamic_pointer_cast<ast::Exp>($1);
      auto list = std::dynamic_pointer_cast<ast::ExpList>($3);
      list->push_front(e);
      $$ = list; }
;

Type: INT
    { $$ = std::make_shared<ast::Type>(ast::BuiltInType::INT); }
    | BYTE
    { $$ = std::make_shared<ast::Type>(ast::BuiltInType::BYTE); }
    | BOOL
    { $$ = std::make_shared<ast::Type>(ast::BuiltInType::BOOL); }
;

Exp: LPAREN Exp RPAREN 
    { $$ = $2; }
   | Exp ADD Exp
    { $$ = std::make_shared<ast::BinOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::BinOpType::ADD
    ); }
   | Exp SUB Exp
    { $$ = std::make_shared<ast::BinOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::BinOpType::SUB
    ); }
   | Exp MUL Exp
    { $$ = std::make_shared<ast::BinOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::BinOpType::MUL
    ); }
   | Exp DIV Exp
    { $$ = std::make_shared<ast::BinOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::BinOpType::DIV
    ); }
   | ID
    { $$ = $1; }
   | Call
    { $$ = $1; }
   | NUM
    { $$ = $1; }
   | NUM_B
    { $$ = $1; }
   | STRING
    { $$ = $1; }
   | TRUE
    { $$ = std::make_shared<ast::Bool>(true); }
   | FALSE
    { $$ = std::make_shared<ast::Bool>(false); }
   | NOT Exp
    { $$ = std::make_shared<ast::Not>(
        std::dynamic_pointer_cast<ast::Exp>($2)
    ); }
   | Exp AND Exp
    { $$ = std::make_shared<ast::And>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3)
    ); }
   | Exp OR Exp
    { $$ = std::make_shared<ast::Or>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3)
    ); }
   | Exp EQ Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::EQ
    ); }
   | Exp NE Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::NE
    ); }
   | Exp LT Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::LT
    ); }
   | Exp GT Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::GT
    ); }
   | Exp LE Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::LE
    ); }
   | Exp GE Exp
    { $$ = std::make_shared<ast::RelOp>(
        std::dynamic_pointer_cast<ast::Exp>($1),
        std::dynamic_pointer_cast<ast::Exp>($3),
        ast::RelOpType::GE
    ); }
   | LPAREN Type RPAREN Exp %prec NOT
    { $$ = std::make_shared<ast::Cast>(
        std::dynamic_pointer_cast<ast::Exp>($4),
        std::dynamic_pointer_cast<ast::Type>($2)
    ); }
;

%%

// TODO: Place any additional code here
#include <cstdlib>

void yyerror(const char* msg) {
    output::errorSyn(yylineno);
    std::exit(0);
}
