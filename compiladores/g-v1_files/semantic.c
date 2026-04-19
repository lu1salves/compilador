#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"

typedef struct SemanticContext {
    SymbolTableStack symtab;
    int error_reported;
} SemanticContext;

static void semantic_error(SemanticContext *ctx, int line, const char *message, const char *detail) {
    if (ctx->error_reported) {
        return;
    }

    if (detail != NULL && detail[0] != '\0') {
        printf("ERRO: %s %s %d\n", message, detail, line);
    } else {
        printf("ERRO: %s %d\n", message, line);
    }
    ctx->error_reported = 1;
}

static int is_valid_type(ASTDataType type) {
    return type == AST_TYPE_INT || type == AST_TYPE_CAR;
}

static int is_int_type(ASTDataType type) {
    return type == AST_TYPE_INT;
}

static ASTDataType analyze_expression(SemanticContext *ctx, ASTNode *expr);
static void analyze_command_list(SemanticContext *ctx, ASTNode *cmd);
static void analyze_block(SemanticContext *ctx, ASTNode *block);

static ASTDataType analyze_identifier_use(SemanticContext *ctx, ASTNode *node) {
    SymbolEntry *entry;

    if (node == NULL || node->lexeme == NULL) {
        return AST_TYPE_INVALID;
    }

    entry = symtab_lookup(&ctx->symtab, node->lexeme);
    if (entry == NULL) {
        semantic_error(ctx, node->line, "IDENTIFICADOR NAO DECLARADO", node->lexeme);
        node->data_type = AST_TYPE_INVALID;
        return AST_TYPE_INVALID;
    }

    node->data_type = entry->type;
    return entry->type;
}

static ASTDataType analyze_binary_op(SemanticContext *ctx, ASTNode *expr) {
    ASTDataType left_type;
    ASTDataType right_type;
    const char *op;

    left_type = analyze_expression(ctx, expr->child1);
    right_type = analyze_expression(ctx, expr->child2);
    op = expr->lexeme != NULL ? expr->lexeme : "";

    if (ctx->error_reported) {
        expr->data_type = AST_TYPE_INVALID;
        return AST_TYPE_INVALID;
    }

    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        if (!is_int_type(left_type) || !is_int_type(right_type)) {
            semantic_error(ctx, expr->line, "OPERACAO ARITMETICA REQUER OPERANDOS INT", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
        }
        expr->data_type = AST_TYPE_INT;
        return AST_TYPE_INT;
    }

    if (strcmp(op, "||") == 0 || strcmp(op, "&") == 0) {
        if (!is_int_type(left_type) || !is_int_type(right_type)) {
            semantic_error(ctx, expr->line, "OPERACAO LOGICA REQUER OPERANDOS INT", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
        }
        expr->data_type = AST_TYPE_INT;
        return AST_TYPE_INT;
    }

    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
        if (!is_valid_type(left_type) || !is_valid_type(right_type) || left_type != right_type) {
            semantic_error(ctx, expr->line, "OPERACAO RELACIONAL REQUER OPERANDOS DO MESMO TIPO", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
        }
        expr->data_type = AST_TYPE_INT;
        return AST_TYPE_INT;
    }

    semantic_error(ctx, expr->line, "OPERADOR DESCONHECIDO", op);
    expr->data_type = AST_TYPE_INVALID;
    return AST_TYPE_INVALID;
}

static ASTDataType analyze_unary_op(SemanticContext *ctx, ASTNode *expr) {
    ASTDataType inner_type;
    const char *op;

    inner_type = analyze_expression(ctx, expr->child1);
    op = expr->lexeme != NULL ? expr->lexeme : "";

    if (ctx->error_reported) {
        expr->data_type = AST_TYPE_INVALID;
        return AST_TYPE_INVALID;
    }

    if (strcmp(op, "-") == 0) {
        if (!is_int_type(inner_type)) {
            semantic_error(ctx, expr->line, "NEGACAO ARITMETICA REQUER OPERANDO INT", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
        }
        expr->data_type = AST_TYPE_INT;
        return AST_TYPE_INT;
    }

    if (strcmp(op, "!") == 0) {
        if (!is_int_type(inner_type)) {
            semantic_error(ctx, expr->line, "NEGACAO LOGICA REQUER OPERANDO INT", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
        }
        expr->data_type = AST_TYPE_INT;
        return AST_TYPE_INT;
    }

    semantic_error(ctx, expr->line, "OPERADOR UNARIO DESCONHECIDO", op);
    expr->data_type = AST_TYPE_INVALID;
    return AST_TYPE_INVALID;
}

static ASTDataType analyze_expression(SemanticContext *ctx, ASTNode *expr) {
    ASTDataType left_type;
    ASTDataType right_type;

    if (expr == NULL) {
        return AST_TYPE_INVALID;
    }

    switch (expr->kind) {
        case AST_IDENT:
            return analyze_identifier_use(ctx, expr);

        case AST_INT_CONST:
            expr->data_type = AST_TYPE_INT;
            return AST_TYPE_INT;

        case AST_CHAR_CONST:
            expr->data_type = AST_TYPE_CAR;
            return AST_TYPE_CAR;

        case AST_STRING_LITERAL:
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;

        case AST_ASSIGN:
            left_type = analyze_expression(ctx, expr->child1);
            right_type = analyze_expression(ctx, expr->child2);
            if (ctx->error_reported) {
                expr->data_type = AST_TYPE_INVALID;
                return AST_TYPE_INVALID;
            }
            if (expr->child1 == NULL || expr->child1->kind != AST_IDENT) {
                semantic_error(ctx, expr->line, "LADO ESQUERDO DA ATRIBUICAO INVALIDO", NULL);
                expr->data_type = AST_TYPE_INVALID;
                return AST_TYPE_INVALID;
            }
            if (!is_valid_type(left_type) || !is_valid_type(right_type) || left_type != right_type) {
                semantic_error(ctx, expr->line, "ATRIBUICAO COM TIPOS INCOMPATIVEIS", NULL);
                expr->data_type = AST_TYPE_INVALID;
                return AST_TYPE_INVALID;
            }
            expr->data_type = left_type;
            return left_type;

        case AST_BINARY_OP:
            return analyze_binary_op(ctx, expr);

        case AST_UNARY_OP:
            return analyze_unary_op(ctx, expr);

        default:
            semantic_error(ctx, expr->line, "EXPRESSAO INVALIDA", NULL);
            expr->data_type = AST_TYPE_INVALID;
            return AST_TYPE_INVALID;
    }
}

static void analyze_declarations(SemanticContext *ctx, ASTNode *decl) {
    while (decl != NULL && !ctx->error_reported) {
        if (decl->kind != AST_VAR_DECL) {
            semantic_error(ctx, decl->line, "DECLARACAO INVALIDA", NULL);
            return;
        }

        if (!is_valid_type(decl->data_type)) {
            semantic_error(ctx, decl->line, "TIPO DE DECLARACAO INVALIDO", NULL);
            return;
        }

        if (symtab_lookup_current(&ctx->symtab, decl->lexeme) != NULL) {
            semantic_error(ctx, decl->line, "IDENTIFICADOR JA DECLARADO NO ESCOPO", decl->lexeme);
            return;
        }

        if (symtab_insert(&ctx->symtab, decl->lexeme, decl->data_type, decl->line) == NULL) {
            semantic_error(ctx, decl->line, "FALHA AO INSERIR IDENTIFICADOR", decl->lexeme);
            return;
        }

        decl = decl->next;
    }
}

static void analyze_command(SemanticContext *ctx, ASTNode *cmd) {
    ASTDataType cond_type;

    if (cmd == NULL || ctx->error_reported) {
        return;
    }

    switch (cmd->kind) {
        case AST_EMPTY_STMT:
        case AST_NEWLINE:
            return;

        case AST_ASSIGN:
            (void)analyze_expression(ctx, cmd);
            return;

        case AST_READ:
            if (cmd->child1 == NULL || cmd->child1->kind != AST_IDENT) {
                semantic_error(ctx, cmd->line, "COMANDO LEIA INVALIDO", NULL);
                return;
            }
            (void)analyze_identifier_use(ctx, cmd->child1);
            return;

        case AST_WRITE:
            if (cmd->child1 == NULL) {
                semantic_error(ctx, cmd->line, "COMANDO ESCREVA INVALIDO", NULL);
                return;
            }
            if (cmd->child1->kind != AST_STRING_LITERAL) {
                (void)analyze_expression(ctx, cmd->child1);
            }
            return;

        case AST_IF:
            cond_type = analyze_expression(ctx, cmd->child1);
            if (!ctx->error_reported && !is_valid_type(cond_type)) {
                semantic_error(ctx, cmd->line, "CONDICAO INVALIDA NO SE", NULL);
                return;
            }
            analyze_command(ctx, cmd->child2);
            analyze_command(ctx, cmd->child3);
            return;

        case AST_WHILE:
            cond_type = analyze_expression(ctx, cmd->child1);
            if (!ctx->error_reported && !is_valid_type(cond_type)) {
                semantic_error(ctx, cmd->line, "CONDICAO INVALIDA NO ENQUANTO", NULL);
                return;
            }
            analyze_command(ctx, cmd->child2);
            return;

        case AST_BLOCK:
            analyze_block(ctx, cmd);
            return;

        default:
            semantic_error(ctx, cmd->line, "COMANDO INVALIDO", NULL);
            return;
    }
}

static void analyze_command_list(SemanticContext *ctx, ASTNode *cmd) {
    while (cmd != NULL && !ctx->error_reported) {
        analyze_command(ctx, cmd);
        cmd = cmd->next;
    }
}

static void analyze_block(SemanticContext *ctx, ASTNode *block) {
    if (block == NULL || ctx->error_reported) {
        return;
    }

    if (block->kind != AST_BLOCK) {
        semantic_error(ctx, block->line, "BLOCO INVALIDO", NULL);
        return;
    }

    symtab_push_scope(&ctx->symtab);
    analyze_declarations(ctx, block->child1);
    if (!ctx->error_reported) {
        analyze_command_list(ctx, block->child2);
    }
    symtab_pop_scope(&ctx->symtab);
}

int semantic_check(ASTNode *root) {
    SemanticContext ctx;

    ctx.error_reported = 0;
    symtab_init(&ctx.symtab);

    if (root == NULL) {
        semantic_error(&ctx, 0, "ARVORE SINTATICA AUSENTE", NULL);
    } else if (root->kind != AST_PROGRAM || root->child1 == NULL) {
        semantic_error(&ctx, root->line, "PROGRAMA INVALIDO", NULL);
    } else {
        analyze_block(&ctx, root->child1);
    }

    symtab_destroy(&ctx.symtab);
    return ctx.error_reported ? 0 : 1;
}
