%option noyywrap yylineno noinput nounput

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g-v1.tab.h"

#define YY_USER_ACTION do { \
    yylloc.first_line = yylineno; \
    yylloc.last_line = yylineno; \
    yylloc.first_column = 1; \
    yylloc.last_column = 1; \
} while (0);

static int comment_start_line = 1;
static int string_start_line = 1;

static char *xstrdup_local(const char *src);
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

[ \t\r]+                           { }
\n                                 { }

"/*"                               { comment_start_line = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"                      { BEGIN(INITIAL); }
<COMMENT>\n                        { }
<COMMENT>.                         { }
<COMMENT><<EOF>>                   { lexical_error_at("COMENTARIO NAO TERMINA", comment_start_line); }

\"                                 { string_start_line = yylineno; BEGIN(STRING); yymore(); }
<STRING>[^\\\n\"]+                 { yymore(); }
<STRING>{CAR_ESC}                  { yymore(); }
<STRING>\"                         { BEGIN(INITIAL); yylval.str = xstrdup_local(yytext); return CADEIACARACTERES; }
<STRING>\n                         { lexical_error_at("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", string_start_line); }
<STRING><<EOF>>                    { lexical_error_at("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", string_start_line); }

"principal"                        { return PRINCIPAL; }
"int"                              { return INT; }
"car"                              { return CAR; }
"leia"                             { return LEIA; }
"escreva"                          { return ESCREVA; }
"novalinha"                        { return NOVALINHA; }
"se"                               { return SE; }
"entao"                            { return ENTAO; }
"senao"                            { return SENAO; }
"fimse"                            { return FIMSE; }
"enquanto"                         { return ENQUANTO; }

"||"                               { return OU; }
"&"                                { return E; }
"=="                               { return IGUAL; }
"!="                               { return DIFERENTE; }
">="                               { return MAIORIGUAL; }
"<="                               { return MENORIGUAL; }

{CARACTERE}                        { yylval.str = xstrdup_local(yytext); return CARCONST; }
{INTLIT}                           { yylval.str = xstrdup_local(yytext); return INTCONST; }
{ID}                               { yylval.str = xstrdup_local(yytext); return IDENTIFICADOR; }

[{}();,:+\-*/<>=!]                 { return yytext[0]; }

<<EOF>>                            { return 0; }

.                                  { lexical_error_at("CARACTERE INVALIDO", yylineno); }

%%

static char *xstrdup_local(const char *src) {
    size_t len = strlen(src) + 1;
    char *dst = (char *)malloc(len);
    if (dst == NULL) {
        fprintf(stderr, "ERRO: falha de memoria\n");
        exit(EXIT_FAILURE);
    }
    memcpy(dst, src, len);
    return dst;
}

static void lexical_error_at(const char *msg, int line) {
    printf("ERRO: %s %d\n", msg, line);
    exit(EXIT_FAILURE);
}
