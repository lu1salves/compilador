#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"

int main(void) {
    SymbolTableStack stack;
    SymbolEntry *e;

    symtab_init(&stack);
    if (stack.top != NULL || stack.size != 0) {
        fprintf(stderr, "symtab_init falhou\n");
        return 1;
    }

    symtab_push_scope(&stack);
    if (symtab_insert(&stack, "x", AST_TYPE_INT, 1) == NULL) {
        fprintf(stderr, "insert x falhou\n");
        return 2;
    }
    if (symtab_insert(&stack, "x", AST_TYPE_CAR, 2) != NULL) {
        fprintf(stderr, "redecl no mesmo escopo passou\n");
        return 3;
    }

    symtab_push_scope(&stack);
    if (symtab_insert(&stack, "x", AST_TYPE_CAR, 3) == NULL) {
        fprintf(stderr, "shadowing falhou\n");
        return 4;
    }

    e = symtab_lookup(&stack, "x");
    if (e == NULL || e->type != AST_TYPE_CAR || e->decl_line != 3) {
        fprintf(stderr, "lookup topo falhou\n");
        return 5;
    }

    symtab_pop_scope(&stack);
    e = symtab_lookup(&stack, "x");
    if (e == NULL || e->type != AST_TYPE_INT || e->decl_line != 1) {
        fprintf(stderr, "lookup apos pop falhou\n");
        return 6;
    }

    symtab_destroy(&stack);
    return 0;
}
