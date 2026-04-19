#ifndef GV1_AST_H
#define GV1_AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_EMPTY_STMT,
    AST_ASSIGN,
    AST_READ,
    AST_WRITE,
    AST_NEWLINE,
    AST_IF,
    AST_WHILE,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IDENT,
    AST_INT_CONST,
    AST_CHAR_CONST,
    AST_STRING_LITERAL
} ASTKind;

typedef enum {
    AST_TYPE_INVALID = 0,
    AST_TYPE_INT,
    AST_TYPE_CAR
} ASTDataType;

typedef struct ASTNode {
    ASTKind kind;
    int line;
    ASTDataType data_type;
    char *lexeme;
    struct ASTNode *child1;
    struct ASTNode *child2;
    struct ASTNode *child3;
    struct ASTNode *next;
} ASTNode;

ASTNode *ast_make_program(ASTNode *block, int line);
ASTNode *ast_make_block(ASTNode *decls, ASTNode *commands, int line);
ASTNode *ast_make_decl(int line, const char *name, ASTDataType type);
ASTNode *ast_make_ident(const char *name, int line);
ASTNode *ast_make_int_const(const char *text, int line);
ASTNode *ast_make_char_const(const char *text, int line);
ASTNode *ast_make_string_literal(const char *text, int line);
ASTNode *ast_make_empty_stmt(int line);
ASTNode *ast_make_assign(ASTNode *lhs, ASTNode *rhs, int line);
ASTNode *ast_make_read(ASTNode *ident, int line);
ASTNode *ast_make_write(ASTNode *value, int line);
ASTNode *ast_make_newline(int line);
ASTNode *ast_make_if(ASTNode *cond, ASTNode *then_cmd, ASTNode *else_cmd, int line);
ASTNode *ast_make_while(ASTNode *cond, ASTNode *body, int line);
ASTNode *ast_make_binary_op(const char *op, ASTNode *lhs, ASTNode *rhs, int line);
ASTNode *ast_make_unary_op(const char *op, ASTNode *expr, int line);

ASTNode *ast_append(ASTNode *list, ASTNode *node);
ASTNode *ast_build_decl_list(ASTNode *id_list, ASTDataType type);
const char *ast_type_name(ASTDataType type);

void ast_print(FILE *out, const ASTNode *node);
void ast_free(ASTNode *node);

#endif
