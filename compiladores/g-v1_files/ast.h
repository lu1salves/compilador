#ifndef AST_H
#define AST_H

/*
 * ast.h — Definição da Árvore Sintática Abstrata (AST)
 *
 * Todos os nós compartilham a mesma struct No. Isso simplifica o código e
 * evita a necessidade de casts. Campos não utilizados em determinado tipo
 * de nó simplesmente ficam NULL/0.
 *
 * Convenção de filhos:
 *   filho1, filho2, filho3 — sub-árvores do nó atual
 *   prox                   — próximo item numa lista (lista de comandos,
 *                            lista de declarações, lista de ids)
 */

/* ===== Tipo das variáveis/expressões ===== */
typedef enum {
    TIPO_INT,
    TIPO_CAR,
    TIPO_INDET   /* ainda não determinado — preenchido pelo analisador semântico */
} TipoVar;

/* ===== Tipos de nós ===== */
typedef enum {
    /* Estrutura do programa */
    NO_PROGRAMA,      /* filho1=bloco                                          */
    NO_BLOCO,         /* filho1=lista_decl (NULL se sem vars), filho2=lista_cmd */

    /* Declarações de variáveis */
    NO_DECL_VAR,      /* filho1=lista de NO_ID (via prox); val.ival=TipoVar    */
                      /* prox = próxima linha de declaração no mesmo bloco     */

    /* Comandos */
    NO_CMD_VAZIO,     /* ;                                                     */
    NO_CMD_EXPR,      /* filho1=expr                                           */
    NO_CMD_LEIA,      /* filho1=NO_ID                                          */
    NO_CMD_ESCREVA,   /* filho1=expr ou NO_CADEIA                              */
    NO_CMD_NOVALINHA,
    NO_CMD_SE,        /* filho1=cond, filho2=entao, filho3=senao (NULL se n/a) */
    NO_CMD_ENQUANTO,  /* filho1=cond, filho2=corpo                             */

    /* Expressões binárias */
    NO_ATRIB,         /* val.sval=nome da variável destino; filho1=expr dir.   */
    NO_OU,            /* filho1=esq, filho2=dir  (||)                          */
    NO_E,             /* filho1=esq, filho2=dir  (&)                           */
    NO_IGUAL,         /* ==  */
    NO_DIFERENTE,     /* !=  */
    NO_MENOR,         /* <   */
    NO_MAIOR,         /* >   */
    NO_MENORIGUAL,    /* <=  */
    NO_MAIORIGUAL,    /* >=  */
    NO_SOMA,          /* +   */
    NO_SUB,           /* -   */
    NO_MUL,           /* *   */
    NO_DIV,           /* /   */

    /* Expressões unárias */
    NO_NEG,           /* -expr   filho1=operando */
    NO_NOT,           /* !expr   filho1=operando */

    /* Folhas */
    NO_ID,            /* val.sval = nome do identificador */
    NO_INTCONST,      /* val.ival = valor inteiro         */
    NO_CARCONST,      /* val.sval = literal (ex: 'a')     */
    NO_CADEIA,        /* val.sval = literal (ex: "oi")    */
} TipoNo;

/* ===== Estrutura do nó ===== */
typedef struct No {
    TipoNo  tipo;
    int     id;          /* identificador único (para eventos de visualização) */
    int     linha;
    TipoVar tipo_expr;   /* preenchido pela análise semântica */
    union {
        char *sval;      /* nomes, strings, char literals     */
        int   ival;      /* inteiros; TipoVar em NO_DECL_VAR  */
    } val;
    struct No *filho1;
    struct No *filho2;
    struct No *filho3;
    struct No *prox;     /* próximo elemento em listas        */
} No;

/* ===== Raiz global da AST ===== */
extern No *ast_raiz;

/* ===== Stream de eventos para o visualizador (NULL = desativado) ===== */
extern FILE *ast_eventos_fp;

/*
 * ast_liga_prox — liga dois nós via campo prox e emite evento P.
 * Use no lugar de "a->prox = b" quando quiser rastrear listas.
 */
void ast_liga_prox(No *a, No *b);

/* ===== Funções construtoras ===== */
No *ast_programa    (No *bloco, int linha);
No *ast_bloco       (No *decls, No *cmds, int linha);
No *ast_decl_var    (No *ids, int tipo, int linha);
No *ast_id          (char *nome, int linha);
No *ast_cmd_vazio   (int linha);
No *ast_cmd_expr    (No *expr, int linha);
No *ast_cmd_leia    (No *id, int linha);
No *ast_cmd_escreva (No *expr, int linha);
No *ast_cmd_novalinha(int linha);
No *ast_cmd_se      (No *cond, No *entao, No *senao, int linha);
No *ast_cmd_enquanto(No *cond, No *corpo, int linha);
No *ast_binop       (TipoNo op, No *esq, No *dir, int linha);
No *ast_unop        (TipoNo op, No *filho, int linha);
No *ast_atrib       (char *nome, No *expr, int linha);
No *ast_intconst    (int val, int linha);
No *ast_carconst    (char *val, int linha);
No *ast_cadeia      (char *val, int linha);

/* ===== Utilitários ===== */
const char *nome_tipo_no(TipoNo t);
void ast_print(No *no, int indent);

#endif /* AST_H */
