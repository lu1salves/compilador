#ifndef TABELA_H
#define TABELA_H

/*
 * tabela.h — Tabela de símbolos com suporte a escopos aninhados
 *
 * A linguagem G-V1 tem blocos que se aninham, e cada bloco pode declarar
 * variáveis locais. Para modelar isso usamos uma PILHA de tabelas:
 *
 *   topo → [ escopo atual ] → [ escopo pai ] → [ escopo avô ] → NULL
 *
 * Ao entrar num bloco com declarações: tabela_push().
 * Ao sair do bloco:                    tabela_pop().
 *
 * Cada tabela é uma lista encadeada de EntradaTabela.
 *
 * O campo 'offset' é reservado para a geração de código, que precisará
 * saber a posição de cada variável na memória (ex.: deslocamento em
 * relação ao frame pointer no MIPS).
 */

#include "ast.h"

/* ===== Uma entrada na tabela ===== */
typedef struct EntradaTabela {
    char   *nome;
    TipoVar tipo;
    int     offset;              /* calculado na geração de código */
    struct EntradaTabela *prox;
} EntradaTabela;

/* ===== Um nível de escopo ===== */
typedef struct TabelaSimbolos {
    EntradaTabela       *entradas;   /* lista de variáveis deste escopo */
    struct TabelaSimbolos *anterior; /* escopo envolvente               */
} TabelaSimbolos;

/* ===== Topo da pilha (escopo atual) ===== */
extern TabelaSimbolos *tabela_topo;

/* ===== Operações ===== */

/* Cria e empurra um novo escopo vazio na pilha. */
void tabela_push(void);

/* Remove e libera o escopo do topo da pilha. */
void tabela_pop(void);

/*
 * Insere uma variável no escopo atual.
 * Retorna  0 se inserida com sucesso.
 * Retorna -1 se já existe uma variável com o mesmo nome NESTE escopo
 *            (redeclaração — erro semântico a ser tratado pelo chamador).
 */
int tabela_insere(const char *nome, TipoVar tipo);

/*
 * Busca APENAS no escopo atual.
 * Útil para detectar redeclaração no mesmo bloco.
 */
EntradaTabela *tabela_busca_local(const char *nome);

/*
 * Busca em TODOS os escopos empilhados, do mais interno para fora.
 * Útil para verificar se uma variável foi declarada antes do uso.
 * Retorna a entrada do escopo mais próximo (shadowing).
 */
EntradaTabela *tabela_busca(const char *nome);

/* Imprime toda a pilha de tabelas (para depuração). */
void tabela_print(void);

#endif /* TABELA_H */
