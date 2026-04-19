#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela.h"

TabelaSimbolos *tabela_topo = NULL;

/* ------------------------------------------------------------------
 * tabela_push
 *
 * Cria um novo nível de escopo e coloca no topo da pilha.
 * Chamado ao entrar num bloco que tem declarações de variáveis.
 * ------------------------------------------------------------------ */
void tabela_push(void) {
    TabelaSimbolos *t = calloc(1, sizeof(TabelaSimbolos));
    if (!t) { perror("calloc"); exit(EXIT_FAILURE); }
    t->anterior = tabela_topo;   /* encadeia ao escopo pai */
    tabela_topo = t;
}

/* ------------------------------------------------------------------
 * tabela_pop
 *
 * Remove e libera o escopo do topo.
 * Chamado ao sair de um bloco que tinha declarações de variáveis.
 * ------------------------------------------------------------------ */
void tabela_pop(void) {
    if (!tabela_topo) return;

    /* Libera todas as entradas do escopo atual */
    EntradaTabela *e = tabela_topo->entradas;
    while (e) {
        EntradaTabela *prox = e->prox;
        free(e->nome);
        free(e);
        e = prox;
    }

    TabelaSimbolos *ant = tabela_topo->anterior;
    free(tabela_topo);
    tabela_topo = ant;
}

/* ------------------------------------------------------------------
 * tabela_insere
 *
 * Insere 'nome' com tipo 'tipo' no escopo atual.
 *
 * Antes de inserir, verifica redeclaração chamando tabela_busca_local.
 * Isso garante que duas variáveis com o mesmo nome no mesmo bloco
 * sejam detectadas como erro semântico.
 *
 * Inserção no início da lista: O(1) e suficiente para este contexto.
 * ------------------------------------------------------------------ */
int tabela_insere(const char *nome, TipoVar tipo) {
    if (!tabela_topo) return -1;

    if (tabela_busca_local(nome)) return -1;  /* já existe neste escopo */

    EntradaTabela *e = calloc(1, sizeof(EntradaTabela));
    if (!e) { perror("calloc"); exit(EXIT_FAILURE); }
    e->nome   = strdup(nome);
    e->tipo   = tipo;
    e->offset = 0;   /* será calculado na fase de geração de código */

    /* Insere no início (mais rápido, ordem de declaração não importa) */
    e->prox             = tabela_topo->entradas;
    tabela_topo->entradas = e;
    return 0;
}

/* ------------------------------------------------------------------
 * tabela_busca_local
 *
 * Percorre apenas as entradas do escopo atual (topo da pilha).
 * Retorna o ponteiro para a entrada encontrada, ou NULL.
 * ------------------------------------------------------------------ */
EntradaTabela *tabela_busca_local(const char *nome) {
    if (!tabela_topo) return NULL;
    for (EntradaTabela *e = tabela_topo->entradas; e; e = e->prox)
        if (strcmp(e->nome, nome) == 0) return e;
    return NULL;
}

/* ------------------------------------------------------------------
 * tabela_busca
 *
 * Percorre a pilha do topo para a base, retornando a primeira
 * ocorrência encontrada. Isso implementa a semântica de shadowing:
 * uma variável local "esconde" uma do escopo externo com mesmo nome.
 * ------------------------------------------------------------------ */
EntradaTabela *tabela_busca(const char *nome) {
    for (TabelaSimbolos *t = tabela_topo; t; t = t->anterior)
        for (EntradaTabela *e = t->entradas; e; e = e->prox)
            if (strcmp(e->nome, nome) == 0) return e;
    return NULL;
}

/* ------------------------------------------------------------------
 * tabela_print  (depuração)
 *
 * Imprime todos os escopos empilhados, do mais interno para fora.
 * ------------------------------------------------------------------ */
void tabela_print(void) {
    int nivel = 0;
    for (TabelaSimbolos *t = tabela_topo; t; t = t->anterior) {
        printf("Escopo %d:\n", nivel++);
        for (EntradaTabela *e = t->entradas; e; e = e->prox)
            printf("  %-12s : %s  (offset=%d)\n",
                   e->nome,
                   e->tipo == TIPO_INT ? "int" : "car",
                   e->offset);
    }
}
