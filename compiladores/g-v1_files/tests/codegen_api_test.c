#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "semantic.h"
#include "codegen.h"

int main(void) {
    ASTNode *decls = NULL;
    ASTNode *cmds = NULL;
    ASTNode *root;
    FILE *fp;

    decls = ast_append(decls, ast_make_decl(1, "x", AST_TYPE_INT));
    decls = ast_append(decls, ast_make_decl(1, "c", AST_TYPE_CAR));
    cmds = ast_append(cmds, ast_make_assign(ast_make_ident("x", 2), ast_make_int_const("5", 2), 2));
    cmds = ast_append(cmds, ast_make_assign(ast_make_ident("c", 3), ast_make_char_const("'A'", 3), 3));
    cmds = ast_append(cmds, ast_make_write(ast_make_ident("x", 4), 4));
    cmds = ast_append(cmds, ast_make_newline(4));
    cmds = ast_append(cmds, ast_make_if(
        ast_make_binary_op("<", ast_make_ident("x", 5), ast_make_int_const("10", 5), 5),
        ast_make_write(ast_make_string_literal("\"ok\\n\"", 5), 5),
        NULL,
        5));

    root = ast_make_program(ast_make_block(decls, cmds, 1), 1);

    if (!semantic_check(root)) {
        fprintf(stderr, "semantic_check falhou\n");
        ast_free(root);
        return 1;
    }

    fp = tmpfile();
    if (fp == NULL) {
        perror("tmpfile");
        ast_free(root);
        return 2;
    }

    if (!codegen_emit(fp, root)) {
        fprintf(stderr, "codegen_emit falhou\n");
        fclose(fp);
        ast_free(root);
        return 3;
    }

    fclose(fp);
    ast_free(root);
    return 0;
}
