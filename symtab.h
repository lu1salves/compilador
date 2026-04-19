#ifndef GV1_SYMTAB_H
#define GV1_SYMTAB_H

#include <stdio.h>

#include "ast.h"

typedef struct SymbolEntry {
    char *name;
    ASTDataType type;
    int decl_line;
    int scope_level;
    int scope_id;
    struct SymbolEntry *next;
} SymbolEntry;

typedef struct ScopeFrame {
    int scope_id;
    int level;
    SymbolEntry *symbols;
    struct ScopeFrame *next;
} ScopeFrame;

typedef struct SymbolTableStack {
    ScopeFrame *top;
    int size;
    int next_scope_id;
} SymbolTableStack;

void symtab_init(SymbolTableStack *stack);
void symtab_destroy(SymbolTableStack *stack);

ScopeFrame *symtab_push_scope(SymbolTableStack *stack);
int symtab_pop_scope(SymbolTableStack *stack);

SymbolEntry *symtab_insert(SymbolTableStack *stack, const char *name, ASTDataType type, int decl_line);
SymbolEntry *symtab_lookup_current(const SymbolTableStack *stack, const char *name);
SymbolEntry *symtab_lookup(const SymbolTableStack *stack, const char *name);

void symtab_print_stack(FILE *out, const SymbolTableStack *stack);
void symtab_dump_from_ast(FILE *out, const ASTNode *root);

#endif
