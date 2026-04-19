%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tabela.h"

extern int yylineno;
extern char *yytext;
extern int yylex(void);
extern FILE *yyin;

static void yyerror(const char *s);
%}

%union {
    char *sval;
    int   ival;
    No   *no;
}

%token <sval> IDENTIFICADOR CADEIACARACTERES CARCONST
%token <ival> INTCONST

%token PRINCIPAL INT CAR LEIA ESCREVA NOVALINHA
%token SE ENTAO SENAO FIMSE ENQUANTO
%token OU E IGUAL DIFERENTE MAIORIGUAL MENORIGUAL

%type <no>   Programa DeclPrograma Bloco VarSection
%type <no>   ListaDeclVar DeclVar
%type <no>   ListaComando Comando
%type <no>   Expr OrExpr AndExpr EqExpr DesigExpr AddExpr MulExpr UnExpr PrimExpr
%type <ival> Tipo

%start Programa

%%

Programa
    : DeclPrograma { ast_raiz = $1; $$ = $1; }
    ;

DeclPrograma
    : PRINCIPAL Bloco { $$ = ast_programa($2, yylineno); }
    ;

Bloco
    : '{' ListaComando '}'
        { $$ = ast_bloco(NULL, $2, yylineno); }
    | VarSection '{' ListaComando '}'
        { $$ = ast_bloco($1, $3, yylineno); }
    ;

VarSection
    : '{' ListaDeclVar '}'
        { $$ = $2; }
    ;

ListaDeclVar
    : IDENTIFICADOR DeclVar ':' Tipo ';'
        {
            No *id  = ast_id($1, yylineno);
            ast_liga_prox(id, $2);
            $$ = ast_decl_var(id, $4, yylineno);
        }
    | IDENTIFICADOR DeclVar ':' Tipo ';' ListaDeclVar
        {
            No *id   = ast_id($1, yylineno);
            ast_liga_prox(id, $2);
            No *decl = ast_decl_var(id, $4, yylineno);
            ast_liga_prox(decl, $6);
            $$ = decl;
        }
    ;

DeclVar
    : /* vazio */
        { $$ = NULL; }
    | ',' IDENTIFICADOR DeclVar
        {
            No *id = ast_id($2, yylineno);
            ast_liga_prox(id, $3);
            $$ = id;
        }
    ;

Tipo
    : INT { $$ = TIPO_INT; }
    | CAR { $$ = TIPO_CAR; }
    ;

ListaComando
    : Comando
        { $$ = $1; }
    | Comando ListaComando
        {
            ast_liga_prox($1, $2);
            $$ = $1;
        }
    ;

Comando
    : ';'
        { $$ = ast_cmd_vazio(yylineno); }
    | Expr ';'
        { $$ = ast_cmd_expr($1, yylineno); }
    | LEIA IDENTIFICADOR ';'
        { $$ = ast_cmd_leia(ast_id($2, yylineno), yylineno); }
    | ESCREVA Expr ';'
        { $$ = ast_cmd_escreva($2, yylineno); }
    | ESCREVA CADEIACARACTERES ';'
        { $$ = ast_cmd_escreva(ast_cadeia($2, yylineno), yylineno); }
    | NOVALINHA ';'
        { $$ = ast_cmd_novalinha(yylineno); }
    | SE '(' Expr ')' ENTAO Comando FIMSE
        { $$ = ast_cmd_se($3, $6, NULL, yylineno); }
    | SE '(' Expr ')' ENTAO Comando SENAO Comando FIMSE
        { $$ = ast_cmd_se($3, $6, $8, yylineno); }
    | ENQUANTO '(' Expr ')' Comando
        { $$ = ast_cmd_enquanto($3, $5, yylineno); }
    | Bloco
        { $$ = $1; }
    ;

Expr
    : OrExpr
        { $$ = $1; }
    | IDENTIFICADOR '=' Expr
        { $$ = ast_atrib($1, $3, yylineno); }
    ;

OrExpr
    : AndExpr
        { $$ = $1; }
    | OrExpr OU AndExpr
        { $$ = ast_binop(NO_OU, $1, $3, yylineno); }
    ;

AndExpr
    : EqExpr
        { $$ = $1; }
    | AndExpr E EqExpr
        { $$ = ast_binop(NO_E, $1, $3, yylineno); }
    ;

EqExpr
    : DesigExpr
        { $$ = $1; }
    | EqExpr IGUAL DesigExpr
        { $$ = ast_binop(NO_IGUAL, $1, $3, yylineno); }
    | EqExpr DIFERENTE DesigExpr
        { $$ = ast_binop(NO_DIFERENTE, $1, $3, yylineno); }
    ;

DesigExpr
    : AddExpr
        { $$ = $1; }
    | DesigExpr '<' AddExpr
        { $$ = ast_binop(NO_MENOR, $1, $3, yylineno); }
    | DesigExpr '>' AddExpr
        { $$ = ast_binop(NO_MAIOR, $1, $3, yylineno); }
    | DesigExpr MAIORIGUAL AddExpr
        { $$ = ast_binop(NO_MAIORIGUAL, $1, $3, yylineno); }
    | DesigExpr MENORIGUAL AddExpr
        { $$ = ast_binop(NO_MENORIGUAL, $1, $3, yylineno); }
    ;

AddExpr
    : MulExpr
        { $$ = $1; }
    | AddExpr '+' MulExpr
        { $$ = ast_binop(NO_SOMA, $1, $3, yylineno); }
    | AddExpr '-' MulExpr
        { $$ = ast_binop(NO_SUB, $1, $3, yylineno); }
    ;

MulExpr
    : UnExpr
        { $$ = $1; }
    | MulExpr '*' UnExpr
        { $$ = ast_binop(NO_MUL, $1, $3, yylineno); }
    | MulExpr '/' UnExpr
        { $$ = ast_binop(NO_DIV, $1, $3, yylineno); }
    ;

UnExpr
    : PrimExpr
        { $$ = $1; }
    | '-' PrimExpr
        { $$ = ast_unop(NO_NEG, $2, yylineno); }
    | '!' PrimExpr
        { $$ = ast_unop(NO_NOT, $2, yylineno); }
    ;

PrimExpr
    : IDENTIFICADOR
        { $$ = ast_id($1, yylineno); }
    | CARCONST
        { $$ = ast_carconst($1, yylineno); }
    | INTCONST
        { $$ = ast_intconst($1, yylineno); }
    | '(' Expr ')'
        { $$ = $2; }
    ;

%%

static void yyerror(const char *s) {
    (void)s;
    fprintf(stderr, "ERRO: ERRO SINTATICO %d\n", yylineno);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int file_arg = 1;

    if (argc == 3 && strcmp(argv[1], "--step") == 0) {
        /* Modo passo a passo: eventos em stdout, erros em stderr */
        ast_eventos_fp = stdout;
        file_arg = 2;
    } else if (argc != 2) {
        fprintf(stderr, "Uso: %s [--step] <arquivo.g>\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[file_arg], "r");
    if (!yyin) { perror(argv[file_arg]); return EXIT_FAILURE; }

    int status = yyparse();
    fclose(yyin);

    if (status == 0) {
#ifdef DEBUG
        fprintf(stderr, "=== AST ===\n");
        ast_print(ast_raiz, 0);
#endif
        /* TODO: análise semântica */
        /* TODO: geração de código */
    }

    return status;
}
