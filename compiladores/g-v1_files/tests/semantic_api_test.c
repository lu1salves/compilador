#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "semantic.h"

static ASTNode *build_valid(void) {
    ASTNode *decls = NULL;
    ASTNode *cmds = NULL;
    ASTNode *inner_decls = NULL;
    ASTNode *inner_cmds = NULL;
    ASTNode *inner;

    decls = ast_append(decls, ast_make_decl(1, "x", AST_TYPE_INT));
    decls = ast_append(decls, ast_make_decl(1, "c", AST_TYPE_CAR));
    cmds = ast_append(cmds, ast_make_assign(ast_make_ident("x", 2), ast_make_int_const("1", 2), 2));
    inner_decls = ast_append(inner_decls, ast_make_decl(3, "x", AST_TYPE_CAR));
    inner_cmds = ast_append(inner_cmds, ast_make_assign(ast_make_ident("x", 4), ast_make_char_const("'a'", 4), 4));
    inner = ast_make_block(inner_decls, inner_cmds, 3);
    cmds = ast_append(cmds, inner);
    cmds = ast_append(cmds, ast_make_if(ast_make_ident("x", 5), ast_make_empty_stmt(5), NULL, 5));

    return ast_make_program(ast_make_block(decls, cmds, 1), 1);
}

static ASTNode *build_invalid_undeclared(void) {
    ASTNode *cmds = NULL;
    cmds = ast_append(cmds, ast_make_assign(ast_make_ident("x", 2), ast_make_int_const("1", 2), 2));
    return ast_make_program(ast_make_block(NULL, cmds, 1), 1);
}

static ASTNode *build_invalid_types(void) {
    ASTNode *decls = NULL;
    ASTNode *cmds = NULL;
    decls = ast_append(decls, ast_make_decl(1, "x", AST_TYPE_INT));
    decls = ast_append(decls, ast_make_decl(1, "c", AST_TYPE_CAR));
    cmds = ast_append(cmds, ast_make_assign(ast_make_ident("x", 2), ast_make_ident("c", 2), 2));
    return ast_make_program(ast_make_block(decls, cmds, 1), 1);
}

int main(void) {
    ASTNode *root;

    root = build_valid();
    if (!semantic_check(root)) {
        fprintf(stderr, "valid falhou\n");
        return 1;
    }
    ast_free(root);

    root = build_invalid_undeclared();
    if (semantic_check(root)) {
        fprintf(stderr, "undeclared passou\n");
        return 2;
    }
    ast_free(root);

    root = build_invalid_types();
    if (semantic_check(root)) {
        fprintf(stderr, "types passou\n");
        return 3;
    }
    ast_free(root);

    return 0;
}
