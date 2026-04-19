#include "ast.h"

#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "ERRO: falha de memoria\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static char *xstrdup_ast(const char *src) {
    size_t len;
    char *dst;

    if (src == NULL) {
        return NULL;
    }

    len = strlen(src) + 1;
    dst = xmalloc(len);
    memcpy(dst, src, len);
    return dst;
}

static ASTNode *ast_new(ASTKind kind, int line, const char *lexeme,
                        ASTNode *child1, ASTNode *child2, ASTNode *child3) {
    ASTNode *node = (ASTNode *)xmalloc(sizeof(*node));
    node->kind = kind;
    node->line = line;
    node->data_type = AST_TYPE_INVALID;
    node->lexeme = xstrdup_ast(lexeme);
    node->child1 = child1;
    node->child2 = child2;
    node->child3 = child3;
    node->next = NULL;
    return node;
}

ASTNode *ast_make_program(ASTNode *block, int line) {
    return ast_new(AST_PROGRAM, line, "principal", block, NULL, NULL);
}

ASTNode *ast_make_block(ASTNode *decls, ASTNode *commands, int line) {
    return ast_new(AST_BLOCK, line, NULL, decls, commands, NULL);
}

ASTNode *ast_make_decl(int line, const char *name, ASTDataType type) {
    ASTNode *node = ast_new(AST_VAR_DECL, line, name, NULL, NULL, NULL);
    node->data_type = type;
    return node;
}

ASTNode *ast_make_ident(const char *name, int line) {
    return ast_new(AST_IDENT, line, name, NULL, NULL, NULL);
}

ASTNode *ast_make_int_const(const char *text, int line) {
    return ast_new(AST_INT_CONST, line, text, NULL, NULL, NULL);
}

ASTNode *ast_make_char_const(const char *text, int line) {
    return ast_new(AST_CHAR_CONST, line, text, NULL, NULL, NULL);
}

ASTNode *ast_make_string_literal(const char *text, int line) {
    return ast_new(AST_STRING_LITERAL, line, text, NULL, NULL, NULL);
}

ASTNode *ast_make_empty_stmt(int line) {
    return ast_new(AST_EMPTY_STMT, line, NULL, NULL, NULL, NULL);
}

ASTNode *ast_make_assign(ASTNode *lhs, ASTNode *rhs, int line) {
    return ast_new(AST_ASSIGN, line, "=", lhs, rhs, NULL);
}

ASTNode *ast_make_read(ASTNode *ident, int line) {
    return ast_new(AST_READ, line, "leia", ident, NULL, NULL);
}

ASTNode *ast_make_write(ASTNode *value, int line) {
    return ast_new(AST_WRITE, line, "escreva", value, NULL, NULL);
}

ASTNode *ast_make_newline(int line) {
    return ast_new(AST_NEWLINE, line, "novalinha", NULL, NULL, NULL);
}

ASTNode *ast_make_if(ASTNode *cond, ASTNode *then_cmd, ASTNode *else_cmd, int line) {
    return ast_new(AST_IF, line, "se", cond, then_cmd, else_cmd);
}

ASTNode *ast_make_while(ASTNode *cond, ASTNode *body, int line) {
    return ast_new(AST_WHILE, line, "enquanto", cond, body, NULL);
}

ASTNode *ast_make_binary_op(const char *op, ASTNode *lhs, ASTNode *rhs, int line) {
    return ast_new(AST_BINARY_OP, line, op, lhs, rhs, NULL);
}

ASTNode *ast_make_unary_op(const char *op, ASTNode *expr, int line) {
    return ast_new(AST_UNARY_OP, line, op, expr, NULL, NULL);
}

ASTNode *ast_append(ASTNode *list, ASTNode *node) {
    ASTNode *tail;

    if (list == NULL) {
        return node;
    }
    if (node == NULL) {
        return list;
    }

    tail = list;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = node;
    return list;
}

ASTNode *ast_build_decl_list(ASTNode *id_list, ASTDataType type) {
    ASTNode *decls = NULL;
    ASTNode *curr = id_list;

    while (curr != NULL) {
        ASTNode *next = curr->next;
        ASTNode *decl = ast_make_decl(curr->line, curr->lexeme, type);
        decls = ast_append(decls, decl);

        curr->next = NULL;
        ast_free(curr);
        curr = next;
    }

    return decls;
}

const char *ast_type_name(ASTDataType type) {
    switch (type) {
        case AST_TYPE_INT:
            return "int";
        case AST_TYPE_CAR:
            return "car";
        default:
            return "<tipo-invalido>";
    }
}

static void print_indent(FILE *out, int indent) {
    int i;
    for (i = 0; i < indent; ++i) {
        fputs("  ", out);
    }
}

static void ast_print_node(FILE *out, const ASTNode *node, int indent);

static void ast_print_list(FILE *out, const char *label, const ASTNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    print_indent(out, indent);
    fprintf(out, "%s:\n", label);

    while (node != NULL) {
        ast_print_node(out, node, indent + 1);
        node = node->next;
    }
}

static void ast_print_node(FILE *out, const ASTNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    print_indent(out, indent);

    switch (node->kind) {
        case AST_PROGRAM:
            fprintf(out, "PROGRAM line=%d\n", node->line);
            ast_print_node(out, node->child1, indent + 1);
            break;

        case AST_BLOCK:
            fprintf(out, "BLOCK line=%d\n", node->line);
            ast_print_list(out, "decls", node->child1, indent + 1);
            ast_print_list(out, "commands", node->child2, indent + 1);
            break;

        case AST_VAR_DECL:
            fprintf(out, "VAR_DECL line=%d name=%s type=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "<anon>",
                    ast_type_name(node->data_type));
            break;

        case AST_EMPTY_STMT:
            fprintf(out, "EMPTY_STMT line=%d\n", node->line);
            break;

        case AST_ASSIGN:
            fprintf(out, "ASSIGN line=%d\n", node->line);
            ast_print_node(out, node->child1, indent + 1);
            ast_print_node(out, node->child2, indent + 1);
            break;

        case AST_READ:
            fprintf(out, "READ line=%d\n", node->line);
            ast_print_node(out, node->child1, indent + 1);
            break;

        case AST_WRITE:
            fprintf(out, "WRITE line=%d\n", node->line);
            ast_print_node(out, node->child1, indent + 1);
            break;

        case AST_NEWLINE:
            fprintf(out, "NEWLINE line=%d\n", node->line);
            break;

        case AST_IF:
            fprintf(out, "IF line=%d\n", node->line);
            print_indent(out, indent + 1);
            fprintf(out, "cond:\n");
            ast_print_node(out, node->child1, indent + 2);
            print_indent(out, indent + 1);
            fprintf(out, "then:\n");
            ast_print_node(out, node->child2, indent + 2);
            if (node->child3 != NULL) {
                print_indent(out, indent + 1);
                fprintf(out, "else:\n");
                ast_print_node(out, node->child3, indent + 2);
            }
            break;

        case AST_WHILE:
            fprintf(out, "WHILE line=%d\n", node->line);
            print_indent(out, indent + 1);
            fprintf(out, "cond:\n");
            ast_print_node(out, node->child1, indent + 2);
            print_indent(out, indent + 1);
            fprintf(out, "body:\n");
            ast_print_node(out, node->child2, indent + 2);
            break;

        case AST_BINARY_OP:
            fprintf(out, "BINARY_OP line=%d op=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "<op>");
            ast_print_node(out, node->child1, indent + 1);
            ast_print_node(out, node->child2, indent + 1);
            break;

        case AST_UNARY_OP:
            fprintf(out, "UNARY_OP line=%d op=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "<op>");
            ast_print_node(out, node->child1, indent + 1);
            break;

        case AST_IDENT:
            fprintf(out, "IDENT line=%d name=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "<id>");
            break;

        case AST_INT_CONST:
            fprintf(out, "INT_CONST line=%d value=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "0");
            break;

        case AST_CHAR_CONST:
            fprintf(out, "CHAR_CONST line=%d value=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "''");
            break;

        case AST_STRING_LITERAL:
            fprintf(out, "STRING_LITERAL line=%d value=%s\n",
                    node->line,
                    node->lexeme ? node->lexeme : "\"\"");
            break;
    }
}

void ast_print(FILE *out, const ASTNode *node) {
    ast_print_node(out, node, 0);
}

void ast_free(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    ast_free(node->child1);
    ast_free(node->child2);
    ast_free(node->child3);
    ast_free(node->next);
    free(node->lexeme);
    free(node);
}
