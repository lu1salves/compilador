%option noyywrap yylineno noinput nounput

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "g-v1.tab.h"

static int comment_start_line = 1;
static int string_start_line = 1;

static void lexical_error_at(const char *msg, int line);
%}

%x COMMENT STRING

LETRA      [A-Za-z_]
DIGITO     [0-9]
ID         {LETRA}({LETRA}|{DIGITO})*
INTLIT     {DIGITO}+
CAR_ESC    \\.
CAR_BODY   ([^\\\n']|{CAR_ESC})
CARACTERE  \'{CAR_BODY}\'

%%

[ \t\r]+                      ;
\n                            ;

"/*"                          { comment_start_line = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"                 { BEGIN(INITIAL); }
<COMMENT>\n                   ;
<COMMENT>.                    ;
<COMMENT><<EOF>>              { lexical_error_at("COMENTARIO NAO TERMINA", comment_start_line); }

"\""                          { string_start_line = yylineno; BEGIN(STRING); yymore(); }
<STRING>[^\\\n\"]+          { yymore(); }
<STRING>{CAR_ESC}             { yymore(); }
<STRING>"\""                  { BEGIN(INITIAL); yylval.sval = strdup(yytext); return CADEIACARACTERES; }
<STRING>\n                    { lexical_error_at("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", string_start_line); }
<STRING><<EOF>>               { lexical_error_at("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", string_start_line); }

"principal"                   { return PRINCIPAL; }
"int"                         { return INT; }
"car"                         { return CAR; }
"leia"                        { return LEIA; }
"escreva"                     { return ESCREVA; }
"novalinha"                   { return NOVALINHA; }
"se"                          { return SE; }
"entao"                       { return ENTAO; }
"senao"                       { return SENAO; }
"fimse"                       { return FIMSE; }
"enquanto"                    { return ENQUANTO; }

"||"                          { return OU; }
"&"                           { return E; }
"=="                          { return IGUAL; }
"!="                          { return DIFERENTE; }
">="                          { return MAIORIGUAL; }
"<="                          { return MENORIGUAL; }

{CARACTERE}                   { yylval.sval = strdup(yytext); return CARCONST; }
{INTLIT}                      { yylval.ival = atoi(yytext);   return INTCONST; }
{ID}                          { yylval.sval = strdup(yytext); return IDENTIFICADOR; }

[{}();,:+\-*/<>=!]            { return yytext[0]; }

<<EOF>>                       { return 0; }

.                             { lexical_error_at("CARACTERE INVALIDO", yylineno); }

%%

static void lexical_error_at(const char *msg, int line) {
    fprintf(stderr, "ERRO: %s %d\n", msg, line);
    exit(EXIT_FAILURE);
}
