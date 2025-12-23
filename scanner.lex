%{
/* This part is copied exactly like it is to the C file that flex creates. */
#include <memory>        
#include "nodes.hpp"     
#include "parser.tab.h"  
#include "output.hpp"
#include <string>

using namespace output;

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}
static std::string processStringLiteral(const char* text, int len) {
    std::string result;
    if (len < 2) return result;  // shouldn't happen for valid strings

    // Skip the leading and trailing quotes: text[0] == '"', text[len-1] == '"'
    int i = 1;
    int end = len - 1;

    while (i < end) {
        char c = text[i];

        if (c != '\\') {
            // Regular character inside the string
            result.push_back(c);
            ++i;
            continue;
        }

        // We saw a backslash – must be an escape sequence
        if (i + 1 >= end) {
            // Backslash at the very end before closing quote (or malformed)
            std::string seq(text + i + 1, end - i);
            errorLex(yylineno);
            return "";
        }

        char next = text[i + 1];

        // Hex escape: \xDD
        if (next == 'x') {
            // Need exactly two hex digits after \x
            if (i + 3 >= end) {
                // Incomplete hex escape
                std::string seq(text + i + 1, end - i - 1);
                errorLex(yylineno);
                return "";
            }

            char h1 = text[i + 2];
            char h2 = text[i + 3];
            int v1 = hexValue(h1);
            int v2 = hexValue(h2);

            if (v1 < 0 || v2 < 0) {
                // Non-hex digit inside \x??
                std::string seq(text + i + 1, 3); // "\x??"
                errorLex(yylineno);
                return "";
            }

            int value = (v1 << 4) | v2;

            // According to the HW, the resulting char must be printable,
            // otherwise it is considered an undefined escape.
            if (value < 0x20 || value > 0x7E) {
                std::string seq(text + i + 1, 3); // "\x??"
                errorLex(yylineno);
                return "";
            }

            result.push_back(static_cast<char>(value));
            i += 4;  // consumed \ x h1 h2
            continue;
        }

        // Simple escapes: \n, \r, \t, \", \\, \0
        char out;
        switch (next) {
            case 'n': out = '\n'; break;
            case 'r': out = '\r'; break;
            case 't': out = '\t'; break;
            case '"': out = '"';  break;
            case '\\': out = '\\'; break;
            case '0': out = '\0'; break;
            default: {
                // Should not happen if the lexer patterns are correct,
                // but we handle it defensively anyway.
                std::string seq(text + i + 1, 1); // "\?"
                errorLex(yylineno);
                return "";
            }
        }

        result.push_back(out);
        i += 2;  // consumed backslash + one escape char
    }

    return result;
}

%}

%option yylineno
%option noyywrap

digit           [0-9]
nonzero_digit   [1-9]
letter          [A-Za-z]
string          [ !#-\[\]-~]
hex             \\x[0-9A-Fa-f]{2}
escape          [nrt"\\0]
comment         [^\r\n]
whitespace      [\t\r\n ]

%%

void                    { return VOID; }
int                     { return INT; }
byte                    { return BYTE; }
bool                    { return BOOL; }
and                     { return AND; }
or                      { return OR; }
not                     { return NOT; }
true                    { return TRUE; }
false                   { return FALSE; }
return                  { return RETURN; }
if                      { return IF; }
else                    { return ELSE; }
while                   { return WHILE; }
break                   { return BREAK; }
continue                { return CONTINUE; }
";"                     { return SC; }
","                     { return COMMA; }
"("                     { return LPAREN; }
")"                     { return RPAREN; }
"{"                     { return LBRACE; }
"}"                     { return RBRACE; }
"["                     { return LBRACK; }
"]"                     { return RBRACK; }
"="                     { return ASSIGN; }
"=="                    { return EQ; }
"!="                    { return NE; }
"<"                     { return LT; }
">"                     { return GT; }
"<="                    { return LE; }
">="                    { return GE; }
"+"                     { return ADD; }
"-"                     { return SUB; }
"*"                     { return MUL; }
"/"                     { return DIV; }


\/\/{comment}*          {  }


{letter}({letter}|{digit})* {
    yylval = std::make_shared<ast::ID>(yytext);
    return ID;
}

({nonzero_digit}{digit}*|0) {
    yylval = std::make_shared<ast::Num>(yytext);
    return NUM;
}

({nonzero_digit}{digit}*|0)b {
    yylval = std::make_shared<ast::NumB>(yytext);
    return NUM_B;
}

\"([^\"\n\\]|\\.)*\" {
    yylval = std::make_shared<ast::String>(yytext);
    return STRING;
}

\"([^\"\n\\]|\\.)*   {
    errorLex(yylineno);
}

{whitespace}+ {}

. {
    errorLex(yylineno);
    exit(0);
}

%%
