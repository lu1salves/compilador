%{
#include <stdio.h>
#include <stdlib.h>

extern int yylineno;
extern char *yytext;
extern int yylex(void);
extern FILE *yyin;

static void yyerror(const char *s);
%}

%token PRINCIPAL IDENTIFICADOR INT CAR LEIA ESCREVA NOVALINHA
%token SE ENTAO SENAO FIMSE ENQUANTO
%token CADEIACARACTERES OU E IGUAL DIFERENTE MAIORIGUAL MENORIGUAL
%token CARCONST INTCONST

%start Programa

%%

Programa
    : DeclPrograma
    ;

DeclPrograma
    : PRINCIPAL Bloco
    ;

Bloco
    : '{' ListaComando '}'
    | VarSection '{' ListaComando '}'
    ;

VarSection
    : '{' ListaDeclVar '}'
    ;

ListaDeclVar
    : IDENTIFICADOR DeclVar ':' Tipo ';'
    | IDENTIFICADOR DeclVar ':' Tipo ';' ListaDeclVar
    ;

DeclVar
    : /* vazio */
    | ',' IDENTIFICADOR DeclVar
    ;

Tipo
    : INT
    | CAR
    ;

ListaComando
    : Comando
    | Comando ListaComando
    ;

Comando
    : ';'
    | Expr ';'
    | LEIA IDENTIFICADOR ';'
    | ESCREVA Expr ';'
    | ESCREVA CADEIACARACTERES ';'
    | NOVALINHA ';'
    | SE '(' Expr ')' ENTAO Comando FIMSE
    | SE '(' Expr ')' ENTAO Comando SENAO Comando FIMSE
    | ENQUANTO '(' Expr ')' Comando
    | Bloco
    ;

Expr
    : OrExpr
    | IDENTIFICADOR '=' Expr
    ;

OrExpr
    : AndExpr
    | OrExpr OU AndExpr
    ;

AndExpr
    : EqExpr
    | AndExpr E EqExpr
    ;

EqExpr
    : DesigExpr
    | EqExpr IGUAL DesigExpr
    | EqExpr DIFERENTE DesigExpr
    ;

DesigExpr
    : AddExpr
    | DesigExpr '<' AddExpr
    | DesigExpr '>' AddExpr
    | DesigExpr MAIORIGUAL AddExpr
    | DesigExpr MENORIGUAL AddExpr
    ;

AddExpr
    : MulExpr
    | AddExpr '+' MulExpr
    | AddExpr '-' MulExpr
    ;

MulExpr
    : UnExpr
    | MulExpr '*' UnExpr
    | MulExpr '/' UnExpr
    ;

UnExpr
    : PrimExpr
    | '-' PrimExpr
    | '!' PrimExpr
    ;

PrimExpr
    : IDENTIFICADOR
    | CARCONST
    | INTCONST
    | '(' Expr ')'
    ;

%%

static void yyerror(const char *s) {
    (void)s;
    printf("ERRO: ERRO SINTATICO %d\n", yylineno);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.g>\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    int status = yyparse();
    fclose(yyin);
    return status;
}
