#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

No  *ast_raiz      = NULL;
FILE *ast_eventos_fp = NULL;   /* NULL = sem eventos; stdout em modo --step */

static int _id_ctr = 0;        /* contador global de IDs de nós */

/* ------------------------------------------------------------------
 * Helpers de emissão de eventos
 *
 * Formato de linha:
 *   N <id> <tipo> <linha> <val>    — novo nó
 *   L <id_pai> <slot> <id_filho>   — ligação filho1/2/3
 *   P <id_no> <id_prox>            — ligação prox (lista)
 *
 * O campo <val> é sempre o último, então pode conter espaços
 * (Python usa split(None, 4) para extrair os 5 campos).
 * ------------------------------------------------------------------ */

static void emit_node(const No *n) {
    if (!ast_eventos_fp) return;
    char val[512] = "-";
    switch (n->tipo) {
        case NO_ID: case NO_ATRIB: case NO_CARCONST: case NO_CADEIA:
            if (n->val.sval)
                snprintf(val, sizeof(val), "%s", n->val.sval);
            break;
        case NO_INTCONST:
            snprintf(val, sizeof(val), "%d", n->val.ival);
            break;
        case NO_DECL_VAR:
            snprintf(val, sizeof(val), "%s",
                     n->val.ival == TIPO_INT ? "int" : "car");
            break;
        default: break;
    }
    fprintf(ast_eventos_fp, "N %d %s %d %s\n",
            n->id, nome_tipo_no(n->tipo), n->linha, val);
    fflush(ast_eventos_fp);
}

static void emit_link(const No *pai, const char *slot, const No *filho) {
    if (!ast_eventos_fp || !filho) return;
    fprintf(ast_eventos_fp, "L %d %s %d\n", pai->id, slot, filho->id);
    fflush(ast_eventos_fp);
}

/* ------------------------------------------------------------------
 * ast_liga_prox — substitui "a->prox = b" nos arquivos .y
 * Emite evento P para que o visualizador possa desenhar a ligação.
 * ------------------------------------------------------------------ */
void ast_liga_prox(No *a, No *b) {
    if (!a) return;
    a->prox = b;
    if (ast_eventos_fp && b) {
        fprintf(ast_eventos_fp, "P %d %d\n", a->id, b->id);
        fflush(ast_eventos_fp);
    }
}

/* ------------------------------------------------------------------
 * nome_tipo_no — string legível para cada tipo de nó
 * (declarada aqui para ser usada tanto em emit_node quanto em ast_print)
 * ------------------------------------------------------------------ */
const char *nome_tipo_no(TipoNo t) {
    switch (t) {
        case NO_PROGRAMA:      return "NO_PROGRAMA";
        case NO_BLOCO:         return "NO_BLOCO";
        case NO_DECL_VAR:      return "NO_DECL_VAR";
        case NO_CMD_VAZIO:     return "NO_CMD_VAZIO";
        case NO_CMD_EXPR:      return "NO_CMD_EXPR";
        case NO_CMD_LEIA:      return "NO_CMD_LEIA";
        case NO_CMD_ESCREVA:   return "NO_CMD_ESCREVA";
        case NO_CMD_NOVALINHA: return "NO_CMD_NOVALINHA";
        case NO_CMD_SE:        return "NO_CMD_SE";
        case NO_CMD_ENQUANTO:  return "NO_CMD_ENQUANTO";
        case NO_ATRIB:         return "NO_ATRIB";
        case NO_OU:            return "NO_OU";
        case NO_E:             return "NO_E";
        case NO_IGUAL:         return "NO_IGUAL";
        case NO_DIFERENTE:     return "NO_DIFERENTE";
        case NO_MENOR:         return "NO_MENOR";
        case NO_MAIOR:         return "NO_MAIOR";
        case NO_MENORIGUAL:    return "NO_MENORIGUAL";
        case NO_MAIORIGUAL:    return "NO_MAIORIGUAL";
        case NO_SOMA:          return "NO_SOMA";
        case NO_SUB:           return "NO_SUB";
        case NO_MUL:           return "NO_MUL";
        case NO_DIV:           return "NO_DIV";
        case NO_NEG:           return "NO_NEG";
        case NO_NOT:           return "NO_NOT";
        case NO_ID:            return "NO_ID";
        case NO_INTCONST:      return "NO_INTCONST";
        case NO_CARCONST:      return "NO_CARCONST";
        case NO_CADEIA:        return "NO_CADEIA";
        default:               return "?";
    }
}

/* ------------------------------------------------------------------
 * Alocador genérico
 * ------------------------------------------------------------------ */
static No *novo_no(TipoNo tipo, int linha) {
    No *n = calloc(1, sizeof(No));
    if (!n) { perror("calloc"); exit(EXIT_FAILURE); }
    n->tipo      = tipo;
    n->id        = ++_id_ctr;
    n->linha     = linha;
    n->tipo_expr = TIPO_INDET;
    return n;
}

/* ===== Construtores ===== */

No *ast_programa(No *bloco, int linha) {
    No *n = novo_no(NO_PROGRAMA, linha);
    n->filho1 = bloco;
    emit_node(n);
    emit_link(n, "f1", bloco);
    return n;
}

No *ast_bloco(No *decls, No *cmds, int linha) {
    No *n = novo_no(NO_BLOCO, linha);
    n->filho1 = decls;
    n->filho2 = cmds;
    emit_node(n);
    emit_link(n, "f1", decls);
    emit_link(n, "f2", cmds);
    return n;
}

No *ast_decl_var(No *ids, int tipo, int linha) {
    No *n       = novo_no(NO_DECL_VAR, linha);
    n->filho1   = ids;
    n->val.ival = tipo;
    emit_node(n);
    emit_link(n, "f1", ids);
    return n;
}

No *ast_id(char *nome, int linha) {
    No *n       = novo_no(NO_ID, linha);
    n->val.sval = nome;
    emit_node(n);
    return n;
}

No *ast_cmd_vazio(int linha) {
    No *n = novo_no(NO_CMD_VAZIO, linha);
    emit_node(n);
    return n;
}

No *ast_cmd_expr(No *expr, int linha) {
    No *n     = novo_no(NO_CMD_EXPR, linha);
    n->filho1 = expr;
    emit_node(n);
    emit_link(n, "f1", expr);
    return n;
}

No *ast_cmd_leia(No *id, int linha) {
    No *n     = novo_no(NO_CMD_LEIA, linha);
    n->filho1 = id;
    emit_node(n);
    emit_link(n, "f1", id);
    return n;
}

No *ast_cmd_escreva(No *expr, int linha) {
    No *n     = novo_no(NO_CMD_ESCREVA, linha);
    n->filho1 = expr;
    emit_node(n);
    emit_link(n, "f1", expr);
    return n;
}

No *ast_cmd_novalinha(int linha) {
    No *n = novo_no(NO_CMD_NOVALINHA, linha);
    emit_node(n);
    return n;
}

No *ast_cmd_se(No *cond, No *entao, No *senao, int linha) {
    No *n     = novo_no(NO_CMD_SE, linha);
    n->filho1 = cond;
    n->filho2 = entao;
    n->filho3 = senao;
    emit_node(n);
    emit_link(n, "f1", cond);
    emit_link(n, "f2", entao);
    emit_link(n, "f3", senao);  /* emit_link ignora NULL */
    return n;
}

No *ast_cmd_enquanto(No *cond, No *corpo, int linha) {
    No *n     = novo_no(NO_CMD_ENQUANTO, linha);
    n->filho1 = cond;
    n->filho2 = corpo;
    emit_node(n);
    emit_link(n, "f1", cond);
    emit_link(n, "f2", corpo);
    return n;
}

No *ast_binop(TipoNo op, No *esq, No *dir, int linha) {
    No *n     = novo_no(op, linha);
    n->filho1 = esq;
    n->filho2 = dir;
    emit_node(n);
    emit_link(n, "f1", esq);
    emit_link(n, "f2", dir);
    return n;
}

No *ast_unop(TipoNo op, No *filho, int linha) {
    No *n     = novo_no(op, linha);
    n->filho1 = filho;
    emit_node(n);
    emit_link(n, "f1", filho);
    return n;
}

No *ast_atrib(char *nome, No *expr, int linha) {
    No *n       = novo_no(NO_ATRIB, linha);
    n->val.sval = nome;
    n->filho1   = expr;
    emit_node(n);
    emit_link(n, "f1", expr);
    return n;
}

No *ast_intconst(int val, int linha) {
    No *n       = novo_no(NO_INTCONST, linha);
    n->val.ival = val;
    emit_node(n);
    return n;
}

No *ast_carconst(char *val, int linha) {
    No *n       = novo_no(NO_CARCONST, linha);
    n->val.sval = val;
    emit_node(n);
    return n;
}

No *ast_cadeia(char *val, int linha) {
    No *n       = novo_no(NO_CADEIA, linha);
    n->val.sval = val;
    emit_node(n);
    return n;
}

/* ===== Impressão da AST (debug textual) ===== */

void ast_print(No *no, int indent) {
    if (!no) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("%s", nome_tipo_no(no->tipo));
    switch (no->tipo) {
        case NO_ID: case NO_ATRIB: case NO_CARCONST: case NO_CADEIA:
            printf("(%s)", no->val.sval); break;
        case NO_INTCONST:
            printf("(%d)", no->val.ival); break;
        case NO_DECL_VAR:
            printf("(%s)", no->val.ival == TIPO_INT ? "int" : "car"); break;
        default: break;
    }
    printf(" [L%d]\n", no->linha);
    ast_print(no->filho1, indent + 1);
    ast_print(no->filho2, indent + 1);
    ast_print(no->filho3, indent + 1);
    ast_print(no->prox,   indent);
}
