#include "symtab.h"

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

static char *xstrdup_sym(const char *src) {
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

static void free_symbol_list(SymbolEntry *entry) {
    while (entry != NULL) {
        SymbolEntry *next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
}

void symtab_init(SymbolTableStack *stack) {
    stack->top = NULL;
    stack->size = 0;
    stack->next_scope_id = 0;
}

void symtab_destroy(SymbolTableStack *stack) {
    while (symtab_pop_scope(stack)) {
    }
}

ScopeFrame *symtab_push_scope(SymbolTableStack *stack) {
    ScopeFrame *scope = (ScopeFrame *)xmalloc(sizeof(*scope));

    scope->scope_id = stack->next_scope_id++;
    scope->level = stack->size;
    scope->symbols = NULL;
    scope->next = stack->top;

    stack->top = scope;
    stack->size += 1;
    return scope;
}

int symtab_pop_scope(SymbolTableStack *stack) {
    ScopeFrame *scope;

    if (stack->top == NULL) {
        return 0;
    }

    scope = stack->top;
    stack->top = scope->next;
    stack->size -= 1;

    free_symbol_list(scope->symbols);
    free(scope);
    return 1;
}

SymbolEntry *symtab_lookup_current(const SymbolTableStack *stack, const char *name) {
    SymbolEntry *entry;

    if (stack->top == NULL) {
        return NULL;
    }

    entry = stack->top->symbols;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

SymbolEntry *symtab_lookup(const SymbolTableStack *stack, const char *name) {
    ScopeFrame *scope = stack->top;

    while (scope != NULL) {
        SymbolEntry *entry = scope->symbols;
        while (entry != NULL) {
            if (strcmp(entry->name, name) == 0) {
                return entry;
            }
            entry = entry->next;
        }
        scope = scope->next;
    }

    return NULL;
}

SymbolEntry *symtab_insert(SymbolTableStack *stack, const char *name, ASTDataType type, int decl_line) {
    SymbolEntry *entry;
    SymbolEntry *tail;

    if (stack->top == NULL) {
        return NULL;
    }

    if (symtab_lookup_current(stack, name) != NULL) {
        return NULL;
    }

    entry = (SymbolEntry *)xmalloc(sizeof(*entry));
    entry->name = xstrdup_sym(name);
    entry->type = type;
    entry->decl_line = decl_line;
    entry->scope_level = stack->top->level;
    entry->scope_id = stack->top->scope_id;
    entry->next = NULL;

    if (stack->top->symbols == NULL) {
        stack->top->symbols = entry;
    } else {
        tail = stack->top->symbols;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = entry;
    }

    return entry;
}

void symtab_print_stack(FILE *out, const SymbolTableStack *stack) {
    const ScopeFrame *scope = stack->top;

    while (scope != NULL) {
        const SymbolEntry *entry;

        fprintf(out, "ESCOPO %d (nivel %d)\n", scope->scope_id, scope->level);
        entry = scope->symbols;
        if (entry == NULL) {
            fprintf(out, "  <vazio>\n");
        }
        while (entry != NULL) {
            fprintf(out, "  %s : %s (linha %d)\n",
                    entry->name,
                    ast_type_name(entry->type),
                    entry->decl_line);
            entry = entry->next;
        }

        scope = scope->next;
    }
}

static void dump_block(FILE *out, SymbolTableStack *stack, const ASTNode *block);

static void dump_node(FILE *out, SymbolTableStack *stack, const ASTNode *node) {
    const ASTNode *curr = node;

    while (curr != NULL) {
        switch (curr->kind) {
            case AST_BLOCK:
                dump_block(out, stack, curr);
                break;

            case AST_IF:
                dump_node(out, stack, curr->child2);
                dump_node(out, stack, curr->child3);
                break;

            case AST_WHILE:
                dump_node(out, stack, curr->child2);
                break;

            default:
                break;
        }
        curr = curr->next;
    }
}

static void dump_block(FILE *out, SymbolTableStack *stack, const ASTNode *block) {
    const ASTNode *decl;
    ScopeFrame *scope = symtab_push_scope(stack);

    decl = block->child1;
    while (decl != NULL) {
        if (decl->kind == AST_VAR_DECL) {
            symtab_insert(stack, decl->lexeme, decl->data_type, decl->line);
        }
        decl = decl->next;
    }

    fprintf(out, "ESCOPO %d (nivel %d)\n", scope->scope_id, scope->level);
    if (scope->symbols == NULL) {
        fprintf(out, "  <vazio>\n");
    } else {
        SymbolEntry *entry = scope->symbols;
        while (entry != NULL) {
            fprintf(out, "  %s : %s (linha %d)\n",
                    entry->name,
                    ast_type_name(entry->type),
                    entry->decl_line);
            entry = entry->next;
        }
    }

    dump_node(out, stack, block->child2);
    symtab_pop_scope(stack);
}

void symtab_dump_from_ast(FILE *out, const ASTNode *root) {
    SymbolTableStack stack;

    symtab_init(&stack);
    if (root != NULL && root->kind == AST_PROGRAM) {
        dump_block(out, &stack, root->child1);
    }
    symtab_destroy(&stack);
}
