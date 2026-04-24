#include "codegen.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

typedef struct CGVar {
    char *name;
    ASTDataType type;
    int offset;
    struct CGVar *next;
} CGVar;

typedef struct CGScope {
    int alloc_bytes;
    CGVar *vars;
    struct CGScope *next;
} CGScope;

typedef struct StringLiteralEntry {
    char *text;
    char *label;
    struct StringLiteralEntry *next;
} StringLiteralEntry;

typedef struct CodegenContext {
    FILE *out;
    CGScope *scope_top;
    int next_label_id;
    int next_string_id;
    StringLiteralEntry *strings;
    int emitted_anything;
    int frame_bytes;
} CodegenContext;

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "ERRO: falha de memoria\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static char *xstrdup_codegen(const char *src) {
    size_t len;
    char *dst;

    if (src == NULL) {
        return NULL;
    }

    len = strlen(src) + 1;
    dst = (char *)xmalloc(len);
    memcpy(dst, src, len);
    return dst;
}

static void emit_line(CodegenContext *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(ctx->out, fmt, ap);
    va_end(ap);
    fputc('\n', ctx->out);
    ctx->emitted_anything = 1;
}

static void push_eval(CodegenContext *ctx) {
    emit_line(ctx, "    addiu $sp, $sp, -4");
    emit_line(ctx, "    sw $v0, 0($sp)");
}

static void pop_eval_to(CodegenContext *ctx, const char *reg) {
    emit_line(ctx, "    lw %s, 0($sp)", reg);
    emit_line(ctx, "    addiu $sp, $sp, 4");
}

static char *make_label(CodegenContext *ctx, const char *prefix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", prefix, ctx->next_label_id++);
    return xstrdup_codegen(buf);
}

static CGScope *push_scope(CodegenContext *ctx) {
    CGScope *scope = (CGScope *)xmalloc(sizeof(*scope));
    scope->alloc_bytes = 0;
    scope->vars = NULL;
    scope->next = ctx->scope_top;
    ctx->scope_top = scope;
    return scope;
}

static void free_var_list(CGVar *var) {
    while (var != NULL) {
        CGVar *next = var->next;
        free(var->name);
        free(var);
        var = next;
    }
}

static void pop_scope(CodegenContext *ctx) {
    CGScope *scope;

    if (ctx->scope_top == NULL) {
        return;
    }

    scope = ctx->scope_top;
    if (scope->alloc_bytes > 0) {
        emit_line(ctx, "    addiu $sp, $sp, %d", scope->alloc_bytes);
        ctx->frame_bytes -= scope->alloc_bytes;
    }
    ctx->scope_top = scope->next;
    free_var_list(scope->vars);
    free(scope);
}

static CGVar *lookup_var(const CodegenContext *ctx, const char *name) {
    const CGScope *scope = ctx->scope_top;

    while (scope != NULL) {
        const CGVar *var = scope->vars;
        while (var != NULL) {
            if (strcmp(var->name, name) == 0) {
                return (CGVar *)var;
            }
            var = var->next;
        }
        scope = scope->next;
    }

    return NULL;
}

static CGVar *declare_var(CodegenContext *ctx, const char *name, ASTDataType type) {
    CGVar *var;

    if (ctx->scope_top == NULL) {
        return NULL;
    }

    ctx->scope_top->alloc_bytes += 4;
    ctx->frame_bytes += 4;
    emit_line(ctx, "    addiu $sp, $sp, -4");
    emit_line(ctx, "    sw $zero, 0($sp)");

    var = (CGVar *)xmalloc(sizeof(*var));
    var->name = xstrdup_codegen(name);
    var->type = type;
    var->offset = -ctx->frame_bytes;
    var->next = ctx->scope_top->vars;
    ctx->scope_top->vars = var;
    return var;
}

static int parse_char_const(const char *lexeme) {
    const unsigned char *p;

    if (lexeme == NULL || lexeme[0] != '\'' ) {
        return 0;
    }

    p = (const unsigned char *)(lexeme + 1);
    if (*p == '\\') {
        ++p;
        switch (*p) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            case '0': return '\0';
            default: return *p;
        }
    }
    return *p;
}

static const char *intern_string_label(CodegenContext *ctx, const char *text) {
    StringLiteralEntry *entry = ctx->strings;

    while (entry != NULL) {
        if (strcmp(entry->text, text) == 0) {
            return entry->label;
        }
        entry = entry->next;
    }

    entry = (StringLiteralEntry *)xmalloc(sizeof(*entry));
    entry->text = xstrdup_codegen(text);
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "_str_%d", ctx->next_string_id++);
        entry->label = xstrdup_codegen(buf);
    }
    entry->next = ctx->strings;
    ctx->strings = entry;
    return entry->label;
}

static void collect_strings(CodegenContext *ctx, const ASTNode *node) {
    const ASTNode *curr = node;

    while (curr != NULL) {
        if (curr->kind == AST_STRING_LITERAL && curr->lexeme != NULL) {
            (void)intern_string_label(ctx, curr->lexeme);
        }
        collect_strings(ctx, curr->child1);
        collect_strings(ctx, curr->child2);
        collect_strings(ctx, curr->child3);
        curr = curr->next;
    }
}

static void emit_data_section(CodegenContext *ctx) {
    StringLiteralEntry *entry;

    if (ctx->strings == NULL) {
        return;
    }

    emit_line(ctx, ".data");
    entry = ctx->strings;
    while (entry != NULL) {
        emit_line(ctx, "%s: .asciiz %s", entry->label, entry->text);
        entry = entry->next;
    }
    emit_line(ctx, "");
}

static void gen_expr(CodegenContext *ctx, const ASTNode *expr);
static void gen_command(CodegenContext *ctx, const ASTNode *cmd);
static void gen_block(CodegenContext *ctx, const ASTNode *block);

static void gen_binary_truthy(CodegenContext *ctx, const char *src, const char *dst) {
    emit_line(ctx, "    sltu %s, $zero, %s", dst, src);
}

static void gen_expr(CodegenContext *ctx, const ASTNode *expr) {
    CGVar *var;

    if (expr == NULL) {
        emit_line(ctx, "    move $v0, $zero");
        return;
    }

    switch (expr->kind) {
        case AST_IDENT:
            var = lookup_var(ctx, expr->lexeme);
            emit_line(ctx, "    lw $v0, %d($fp)", var->offset);
            return;

        case AST_INT_CONST:
            emit_line(ctx, "    li $v0, %s", expr->lexeme != NULL ? expr->lexeme : "0");
            return;

        case AST_CHAR_CONST:
            emit_line(ctx, "    li $v0, %d", parse_char_const(expr->lexeme));
            return;

        case AST_ASSIGN:
            gen_expr(ctx, expr->child2);
            var = lookup_var(ctx, expr->child1->lexeme);
            emit_line(ctx, "    sw $v0, %d($fp)", var->offset);
            return;

        case AST_UNARY_OP:
            gen_expr(ctx, expr->child1);
            if (strcmp(expr->lexeme, "-") == 0) {
                emit_line(ctx, "    subu $v0, $zero, $v0");
            } else if (strcmp(expr->lexeme, "!") == 0) {
                emit_line(ctx, "    sltiu $v0, $v0, 1");
            }
            return;

        case AST_BINARY_OP:
            gen_expr(ctx, expr->child1);
            push_eval(ctx);
            gen_expr(ctx, expr->child2);
            pop_eval_to(ctx, "$t0");

            if (strcmp(expr->lexeme, "+") == 0) {
                emit_line(ctx, "    addu $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, "-") == 0) {
                emit_line(ctx, "    subu $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, "*") == 0) {
                emit_line(ctx, "    mul $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, "/") == 0) {
                emit_line(ctx, "    div $t0, $v0");
                emit_line(ctx, "    mflo $v0");
            } else if (strcmp(expr->lexeme, "||") == 0) {
                gen_binary_truthy(ctx, "$t0", "$t0");
                gen_binary_truthy(ctx, "$v0", "$v0");
                emit_line(ctx, "    or $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, "&") == 0) {
                gen_binary_truthy(ctx, "$t0", "$t0");
                gen_binary_truthy(ctx, "$v0", "$v0");
                emit_line(ctx, "    and $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, "==") == 0) {
                emit_line(ctx, "    xor $v0, $t0, $v0");
                emit_line(ctx, "    sltiu $v0, $v0, 1");
            } else if (strcmp(expr->lexeme, "!=") == 0) {
                emit_line(ctx, "    xor $v0, $t0, $v0");
                emit_line(ctx, "    sltu $v0, $zero, $v0");
            } else if (strcmp(expr->lexeme, "<") == 0) {
                emit_line(ctx, "    slt $v0, $t0, $v0");
            } else if (strcmp(expr->lexeme, ">") == 0) {
                emit_line(ctx, "    slt $v0, $v0, $t0");
            } else if (strcmp(expr->lexeme, "<=") == 0) {
                emit_line(ctx, "    slt $v0, $v0, $t0");
                emit_line(ctx, "    xori $v0, $v0, 1");
            } else if (strcmp(expr->lexeme, ">=") == 0) {
                emit_line(ctx, "    slt $v0, $t0, $v0");
                emit_line(ctx, "    xori $v0, $v0, 1");
            }
            return;

        default:
            emit_line(ctx, "    move $v0, $zero");
            return;
    }
}

static void gen_command_list(CodegenContext *ctx, const ASTNode *cmd) {
    while (cmd != NULL) {
        gen_command(ctx, cmd);
        cmd = cmd->next;
    }
}

static void gen_command(CodegenContext *ctx, const ASTNode *cmd) {
    char *label_else;
    char *label_end;
    char *label_begin;
    char *label_exit;
    CGVar *var;
    const char *str_label;

    if (cmd == NULL) {
        return;
    }

    switch (cmd->kind) {
        case AST_EMPTY_STMT:
            return;

        case AST_ASSIGN:
            gen_expr(ctx, cmd);
            return;

        case AST_READ:
            var = lookup_var(ctx, cmd->child1->lexeme);
            if (var->type == AST_TYPE_INT) {
                emit_line(ctx, "    li $v0, 5");
                emit_line(ctx, "    syscall");
            } else {
                emit_line(ctx, "    li $v0, 12");
                emit_line(ctx, "    syscall");
            }
            emit_line(ctx, "    sw $v0, %d($fp)", var->offset);
            return;

        case AST_WRITE:
            if (cmd->child1 != NULL && cmd->child1->kind == AST_STRING_LITERAL) {
                str_label = intern_string_label(ctx, cmd->child1->lexeme);
                emit_line(ctx, "    la $a0, %s", str_label);
                emit_line(ctx, "    li $v0, 4");
                emit_line(ctx, "    syscall");
            } else {
                gen_expr(ctx, cmd->child1);
                emit_line(ctx, "    move $a0, $v0");
                if (cmd->child1 != NULL && cmd->child1->data_type == AST_TYPE_CAR) {
                    emit_line(ctx, "    li $v0, 11");
                } else {
                    emit_line(ctx, "    li $v0, 1");
                }
                emit_line(ctx, "    syscall");
            }
            return;

        case AST_NEWLINE:
            emit_line(ctx, "    li $a0, 10");
            emit_line(ctx, "    li $v0, 11");
            emit_line(ctx, "    syscall");
            return;

        case AST_IF:
            label_else = make_label(ctx, "else");
            label_end = make_label(ctx, "endif");
            gen_expr(ctx, cmd->child1);
            emit_line(ctx, "    beq $v0, $zero, %s", cmd->child3 != NULL ? label_else : label_end);
            emit_line(ctx, "    nop");
            gen_command(ctx, cmd->child2);
            if (cmd->child3 != NULL) {
                emit_line(ctx, "    j %s", label_end);
                emit_line(ctx, "    nop");
                emit_line(ctx, "%s:", label_else);
                gen_command(ctx, cmd->child3);
            }
            emit_line(ctx, "%s:", label_end);
            free(label_else);
            free(label_end);
            return;

        case AST_WHILE:
            label_begin = make_label(ctx, "while_begin");
            label_exit = make_label(ctx, "while_end");
            emit_line(ctx, "%s:", label_begin);
            gen_expr(ctx, cmd->child1);
            emit_line(ctx, "    beq $v0, $zero, %s", label_exit);
            emit_line(ctx, "    nop");
            gen_command(ctx, cmd->child2);
            emit_line(ctx, "    j %s", label_begin);
            emit_line(ctx, "    nop");
            emit_line(ctx, "%s:", label_exit);
            free(label_begin);
            free(label_exit);
            return;

        case AST_BLOCK:
            gen_block(ctx, cmd);
            return;

        default:
            gen_expr(ctx, cmd);
            return;
    }
}

static void gen_declarations(CodegenContext *ctx, const ASTNode *decl) {
    while (decl != NULL) {
        if (decl->kind == AST_VAR_DECL) {
            (void)declare_var(ctx, decl->lexeme, decl->data_type);
        }
        decl = decl->next;
    }
}

static void gen_block(CodegenContext *ctx, const ASTNode *block) {
    push_scope(ctx);
    gen_declarations(ctx, block->child1);
    gen_command_list(ctx, block->child2);
    pop_scope(ctx);
}

static void free_string_table(StringLiteralEntry *entry) {
    while (entry != NULL) {
        StringLiteralEntry *next = entry->next;
        free(entry->text);
        free(entry->label);
        free(entry);
        entry = next;
    }
}

int codegen_emit(FILE *out, const ASTNode *root) {
    CodegenContext ctx;

    if (out == NULL || root == NULL || root->kind != AST_PROGRAM || root->child1 == NULL) {
        return 0;
    }

    ctx.out = out;
    ctx.scope_top = NULL;
    ctx.next_label_id = 0;
    ctx.next_string_id = 0;
    ctx.strings = NULL;
    ctx.emitted_anything = 0;
    ctx.frame_bytes = 0;

    collect_strings(&ctx, root);
    emit_data_section(&ctx);
    emit_line(&ctx, ".text");
    emit_line(&ctx, ".globl main");
    emit_line(&ctx, "main:");
    emit_line(&ctx, "    move $fp, $sp");
    gen_block(&ctx, root->child1);
    emit_line(&ctx, "    li $v0, 10");
    emit_line(&ctx, "    syscall");

    while (ctx.scope_top != NULL) {
        pop_scope(&ctx);
    }
    free_string_table(ctx.strings);
    return ctx.emitted_anything;
}
