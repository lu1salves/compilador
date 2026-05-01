# Como esse compilador foi construído — raciocínio e passo a passo

Este documento explica **por que** cada decisão foi tomada e **como** o compilador foi montado peça por peça. A ideia não é repetir o que o código faz, mas explicar o raciocínio por trás de cada escolha.

---

## O problema que um compilador resolve

Antes de escrever qualquer linha, é preciso entender o que um compilador faz em essência: **traduzir texto** (um programa escrito pelo usuário) **em instruções que uma máquina executa**.

Esse processo tem etapas bem definidas, e cada etapa resolve um problema específico:

1. O texto bruto precisa ser partido em pedaços com significado → **léxico**
2. Esses pedaços precisam seguir uma estrutura gramatical → **sintático**
3. A estrutura precisa ser representada na memória para ser processada → **AST**
4. As regras semânticas (tipos, declarações) precisam ser verificadas → **semântica**
5. O programa válido precisa virar instruções de máquina → **geração de código**

O projeto implementa exatamente essas cinco etapas, cada uma em seu próprio módulo.

---

## Por que usar Flex e Bison?

A primeira pergunta é: por que não escrever tudo do zero em C?

### O problema do léxico manual

Reconhecer tokens manualmente significa escrever um autômato finito à mão. Para uma linguagem pequena, isso é trabalhoso mas possível. Para qualquer linguagem real, vira um pesadelo de manutenção. Se você adicionar um operador novo, precisa reescrever partes da máquina de estados.

O **Flex** resolve isso com expressões regulares. Você descreve *o que* quer reconhecer, e o Flex gera o autômato. Mudar uma regra é editar uma linha.

### O problema do parser manual

Reconhecer gramáticas manualmente (parser recursivo descendente) funciona para gramáticas simples, mas qualquer ambiguidade ou recursão à esquerda exige reestruturação manual da gramática e controle cuidadoso de precedência.

O **Bison** resolve isso com gramáticas livres de contexto. Você escreve as regras da linguagem em notação BNF, e o Bison gera um parser LALR(1) — um dos algoritmos de parsing mais eficientes para linguagens de programação. Precedência de operadores é declarada, não programada.

### A divisão de responsabilidades

```
g-v1.l   →  Flex   →  lex.yy.c    (analisador léxico)
g-v1.y   →  Bison  →  g-v1.tab.c  (analisador sintático)
```

O Flex e o Bison são projetados para trabalhar juntos: o Bison gera o `g-v1.tab.h` com os códigos numéricos dos tokens, e o Flex inclui esse header para retornar os mesmos códigos. Eles se comunicam através de dois símbolos globais:

- `yylex()` — função chamada pelo Bison para pedir o próximo token
- `yylval` — variável onde o Flex deposita o valor semântico do token

---

## Passo 1 — Definir a linguagem antes de escrever código

O primeiro passo real não é escrever código, é **especificar a linguagem** formalmente.

### Por que isso importa

Se você não sabe exatamente o que sua linguagem aceita, não tem como escrever um léxico nem uma gramática. A especificação da linguagem G-V1 define:

- Quais são as palavras reservadas: `principal`, `se`, `entao`, `senao`, `fimse`, `enquanto`, `leia`, `escreva`, `novalinha`, `int`, `car`
- Quais são os tipos: `int` (inteiro 32 bits), `car` (caractere 8 bits)
- Qual é a estrutura de um programa: sempre começa com `principal`, seguido de um bloco
- O que é um bloco: uma seção opcional de declarações seguida de uma seção de comandos
- Quais operadores existem e qual a precedência de cada um

### A hierarquia de expressões

A precedência de operadores não é óbvia. Sem ela, `1 + 2 * 3` pode ser lido como `(1 + 2) * 3` ou `1 + (2 * 3)`. A solução padrão é criar um não-terminal por nível de precedência:

```
Expr        (menor precedência — atribuição)
  OrExpr    (||)
    AndExpr (&)
      EqExpr (== !=)
        DesigExpr (< > <= >=)
          AddExpr (+ -)
            MulExpr (* /)
              UnExpr (- !)
                PrimExpr (identificador, literal, parênteses)
```

Cada nível só pode "enxergar" os níveis abaixo dele. Isso garante precedência correta sem nenhuma lógica extra — é estrutural.

---

## Passo 2 — O léxico com Flex (`g-v1.l`)

### O que o léxico precisa fazer

O analisador léxico recebe um stream de caracteres e produz um stream de tokens. Um token é um par `(tipo, valor)`. Por exemplo:

- `x` → `(IDENTIFICADOR, "x")`
- `123` → `(INTCONST, "123")`
- `principal` → `(PRINCIPAL, —)` (sem valor, é palavra reservada)

### As opções do Flex e por que foram escolhidas

```lex
%option noyywrap yylineno noinput nounput
```

- `noyywrap`: sem essa opção, o Flex gera uma chamada para `yywrap()` quando o arquivo acaba, exigindo que você implemente essa função. Como o compilador lê um único arquivo, ela não é necessária.
- `yylineno`: o Flex mantém automaticamente um contador de linhas. Sem isso, você teria que contar `\n` manualmente.
- `noinput nounput`: suprime a geração de funções auxiliares que não são utilizadas, evitando warnings de compilação.

### O problema do número de linha

Um detalhe crítico: quando o léxico detecta um erro, precisa apontar a **linha correta**. Para erros simples como um `@` inválido, a linha atual (`yylineno`) é suficiente. Mas para erros que só são detectados no **fechamento**, como um comentário não encerrado ou uma string que atravessa linha, o problema é diferente: quando o EOF chega, `yylineno` aponta para o fim do arquivo, não para onde o comentário ou string **começou**.

A solução foi guardar a linha de início em variáveis separadas:

```c
static int comment_start_line = 1;
static int string_start_line = 1;
```

No momento em que `/*` é reconhecido, `comment_start_line = yylineno`. Se o EOF chegar antes do `*/`, o erro é reportado na linha correta.

### Estados exclusivos: COMMENT e STRING

O Flex permite definir **estados léxicos**. Por padrão, todas as regras estão no estado `INITIAL`. Ao entrar em `BEGIN(COMMENT)`, só as regras marcadas com `<COMMENT>` ficam ativas.

Isso é muito mais limpo do que tentar reconhecer comentários e strings com expressões regulares aninhadas. A lógica de "dentro de um comentário" fica completamente isolada:

```lex
"/*"              { comment_start_line = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"     { BEGIN(INITIAL); }
<COMMENT>\n       { }
<COMMENT>.        { }
<COMMENT><<EOF>>  { lexical_error_at("COMENTARIO NAO TERMINA", comment_start_line); }
```

Para strings, o truque adicional é `yymore()`: em vez de descartar o que foi lido até agora e começar um novo lexema, `yymore()` instrui o Flex a **acumular** o texto no buffer atual. Isso permite que a string inteira (incluindo as aspas) seja capturada em `yytext` de uma vez.

### Por que salvar lexemas com `xstrdup_local`

O buffer `yytext` do Flex é **sobrescrito a cada token**. Se você guardar um ponteiro direto para `yytext`, ele apontará para lixo na próxima chamada. A solução é copiar o texto para memória alocada dinamicamente antes de passá-lo ao parser:

```c
yylval.str = xstrdup_local(yytext);
```

Palavras reservadas não precisam disso — o parser não precisa do texto, só do código do token.

### A regra pega-tudo no final

```lex
. { lexical_error_at("CARACTERE INVALIDO", yylineno); }
```

O Flex aplica as regras em ordem de especificidade. O `.` (qualquer caractere) só é alcançado quando nenhuma outra regra casou. Isso garante que qualquer entrada inesperada seja detectada como erro léxico.

---

## Passo 3 — O parser com Bison (`g-v1.y`)

### A estrutura de um arquivo `.y`

Um arquivo Bison tem três seções separadas por `%%`:

```
[declarações: tokens, tipos, precedências]
%%
[regras da gramática com ações semânticas]
%%
[código C auxiliar]
```

### O `%union` — por que é necessário

Cada token e não-terminal pode carregar um valor diferente. O Bison usa uma `union` C para isso. No projeto, há três tipos possíveis:

```c
%union {
    char *str;      // lexema de texto (vindo do Flex)
    ASTNode *node;  // nó da AST (construído pelas ações)
    int type_code;  // código de tipo (int ou car)
}
```

Sem o `%union`, o Bison só trabalharia com um tipo fixo, o que impossibilitaria passar nós da AST pelas ações semânticas.

### `%code requires` — um detalhe importante

```bison
%code requires {
#include "ast.h"
}
```

O Bison gera dois arquivos: `g-v1.tab.c` e `g-v1.tab.h`. O header é incluído pelo Flex e por quem quiser usar os tokens. O problema é que o `%union` usa `ASTNode *`, então o tipo `ASTNode` precisa estar disponível no header gerado também. O bloco `%code requires` garante que `ast.h` seja incluído **no header**, não só no `.c`.

### `%locations` — rastreando linhas na AST

```bison
%locations
```

Essa diretiva ativa o sistema de localização do Bison. Cada símbolo passa a ter uma posição associada, acessível via `@N.first_line`. Isso permite que cada nó da AST receba o número de linha correto:

```c
$$ = ast_make_if($3, $6, NULL, @1.first_line);
//                          ^^^^^^^^^^^^^^^^
//                          linha do token SE
```

Sem isso, seria impossível reportar erros semânticos com número de linha correto.

### Como a AST é construída durante o parsing

A ideia central do Bison é que cada regra da gramática pode ter uma **ação semântica** — um bloco de código C que executa quando a regra é reconhecida. O valor `$$` é o resultado da regra; `$1`, `$2`, etc. são os valores dos símbolos do lado direito.

Por exemplo, quando o parser reconhece uma atribuição:

```bison
| IDENTIFICADOR '=' Expr
  {
      ASTNode *lhs = ast_make_ident($1, @1.first_line);
      $$ = ast_make_assign(lhs, $3, @2.first_line);
      free($1);
  }
```

- `$1` é a string do identificador (veio do Flex via `yylval.str`)
- `$3` é o nó da AST da expressão do lado direito (construído por ações anteriores)
- A ação cria um nó `AST_ASSIGN` ligando os dois

O `free($1)` é necessário porque o lexema foi alocado dinamicamente pelo Flex e a AST fez sua própria cópia interna.

### Por que a atribuição é uma expressão, não um comando

Em G-V1, a atribuição foi modelada como expressão (`Expr`), não como comando separado. Isso reflete a gramática original da linguagem. A consequência é que `x = y = 5;` é válido sintaticamente (atribuição em cascata), já que `y = 5` é uma expressão cujo resultado pode ser atribuído a `x`.

### `yyerror` — por que ignora a mensagem do Bison

```c
static void yyerror(const char *s) {
    (void)s;
    fprintf(stderr, "ERRO: ERRO SINTATICO %d\n", yylineno);
    exit(EXIT_FAILURE);
}
```

O Bison chama `yyerror` com uma mensagem descritiva como `"syntax error, unexpected FIMSE, expecting ';'"`. Essa mensagem é útil para desenvolvimento, mas os testes esperam um formato exato e simples. O `(void)s` silencia o warning do compilador sobre parâmetro não utilizado.

---

## Passo 4 — A AST (`ast.h` / `ast.c`)

### Por que criar uma AST em vez de processar direto no parser

É tentador fazer tudo nas ações semânticas do Bison: verificar tipos, gerar código, tudo ao mesmo tempo que analisa. Isso é chamado de "compilação de passagem única".

O problema é que muitas verificações precisam de informação que ainda não está disponível no momento do parsing. Por exemplo, para verificar tipos de uma expressão, você precisa saber o tipo de cada variável, o que exige que a tabela de símbolos esteja construída. Para gerar código MIPS correto, você precisa saber o layout de memória, que só é calculado depois da semântica.

A AST é a **representação intermediária** que separa as fases. O parser constrói a árvore, e cada fase posterior a percorre de forma independente.

### A estrutura do nó

```c
typedef struct ASTNode {
    ASTKind     kind;       // que tipo de nó é
    int         line;       // linha no fonte (para erros)
    int         id;         // identificador único
    ASTDataType data_type;  // tipo semântico (anotado depois)
    char       *lexeme;     // texto associado (operador, nome, valor)
    ASTNode    *child1;     // filho 1
    ASTNode    *child2;     // filho 2
    ASTNode    *child3;     // filho 3 (usado pelo if com else)
    ASTNode    *next;       // próximo na lista encadeada
} ASTNode;
```

Por que até três filhos fixos em vez de um array dinâmico? Porque nenhum nó da linguagem G-V1 precisa de mais de três filhos: `AST_IF` usa `child1` (condição), `child2` (then) e `child3` (else). Um array seria mais flexível mas mais complexo de gerenciar.

### O campo `next` — listas sem nó especial de lista

Comandos e declarações formam listas. A opção mais simples seria criar um nó `AST_LIST` que contém filhos. A opção escolhida foi usar o campo `next` diretamente nos nós existentes:

```
AST_ASSIGN → AST_WRITE → AST_NEWLINE → NULL
   next          next        next
```

Isso evita um nível extra de indireção. `ast_append` percorre a lista até o final e encadeia o novo nó:

```c
ASTNode *ast_append(ASTNode *list, ASTNode *node) {
    if (!list) return node;
    ASTNode *tail = list;
    while (tail->next) tail = tail->next;
    tail->next = node;
    return list;
}
```

### O campo `data_type` começa como `INVALID`

Todo nó é criado com `data_type = AST_TYPE_INVALID`. A fase semântica é responsável por **anotar** cada nó com seu tipo real. Isso serve para duas coisas:

1. O codegen pode consultar `data_type` para saber qual syscall usar (`int` → syscall 1, `car` → syscall 11)
2. Se algum nó ainda tiver `INVALID` quando o codegen rodar, é sinal de que a semântica não o processou

### `ast_build_decl_list` — transformação de lista

Durante o parsing de `x, y: int;`, o Bison primeiro constrói uma lista de nós `AST_IDENT`:

```
AST_IDENT("x") → AST_IDENT("y")
```

Só depois, quando o tipo `int` é reconhecido, os identificadores são convertidos em declarações. A função `ast_build_decl_list` percorre a lista de identifiers, cria um `AST_VAR_DECL` para cada um com o tipo correto, e libera os nós temporários de identifier. O resultado é:

```
AST_VAR_DECL("x", int) → AST_VAR_DECL("y", int)
```

---

## Passo 5 — A tabela de símbolos (`symtab.h` / `symtab.c`)

### O problema dos escopos aninhados

G-V1 permite blocos aninhados e shadowing de variáveis:

```g
principal {
    x: int;
} {
    x = 1;          /* x do escopo externo */
    {
        x: car;     /* novo x que "esconde" o externo */
        x = 'A';    /* usa o x local */
    }               /* x local some aqui */
    escreva x;      /* volta a ser o x int */
}
```

Uma única tabela flat não consegue modelar isso. A solução é uma **pilha de escopos**: ao entrar em um bloco, empurra um novo escopo; ao sair, desempilha.

### A estrutura em três camadas

```
SymbolTableStack          (a pilha inteira)
  └── ScopeFrame          (um escopo/bloco)
        └── SymbolEntry   (uma variável declarada)
```

Cada `ScopeFrame` tem um `scope_id` único e um `level` (profundidade). Isso permite imprimir a tabela de forma informativa.

### Duas funções de lookup com propósitos diferentes

```c
SymbolEntry *symtab_lookup_current(stack, nome);  // só no topo
SymbolEntry *symtab_lookup(stack, nome);           // em toda a pilha
```

Por que as duas? Elas têm usos distintos:

- `symtab_lookup_current` é usada para detectar **redeclaração no mesmo escopo**: se `x` já existe no topo, declarar `x` de novo é erro.
- `symtab_lookup` é usada para verificar se uma variável foi declarada antes do uso: procura do escopo mais interno para o mais externo, retornando a primeira ocorrência (que é o comportamento correto de shadowing).

---

## Passo 6 — A análise semântica (`semantic.h` / `semantic.c`)

### Por que uma fase separada

A semântica verifica regras que a gramática livre de contexto não consegue expressar. A gramática diz "uma atribuição é `IDENT = Expr`", mas não sabe se `IDENT` foi declarado, nem se o tipo da expressão é compatível. Isso requer percorrer a AST com a tabela de símbolos em mãos.

### O `SemanticContext` — estado encapsulado

```c
typedef struct {
    SymbolTableStack symtab;
    int error_reported;
} SemanticContext;
```

O estado da análise fica em uma struct local a `semantic_check`. Isso evita variáveis globais e torna o módulo testável de forma isolada (os testes de API constroem ASTs à mão e chamam `semantic_check` diretamente).

### A estratégia de parar no primeiro erro

```c
int error_reported;
```

Quando um erro semântico é encontrado, o flag é marcado e a análise continua apenas superficialmente para não produzir erros em cascata. Por exemplo, se `x` não está declarado, usar `x + 1` geraria um segundo erro de tipo sem sentido. O flag evita isso.

### Anotação de tipos na AST

A função `analyze_identifier_use` não apenas verifica se a variável existe — ela copia o tipo da entrada da tabela para `node->data_type`:

```c
node->data_type = entry->type;
```

Da mesma forma, `analyze_binary_op` calcula e anota o tipo do resultado. Ao final da semântica, todos os nós de expressão têm seu tipo correto anotado. O codegen pode então consultar `data_type` sem precisar repetir nenhuma lógica de tipos.

### O ciclo push/pop em `analyze_block`

```c
symtab_push_scope(&ctx->symtab);   // entra no bloco
analyze_declarations(...);          // insere variáveis locais
analyze_command_list(...);          // verifica comandos
symtab_pop_scope(&ctx->symtab);    // sai do bloco, variáveis somem
```

Esse padrão é a essência do escopo léxico. Quando `symtab_pop_scope` é chamado, todas as variáveis do bloco deixam de existir — exatamente o comportamento que a linguagem especifica.

---

## Passo 7 — A geração de código MIPS (`codegen.h` / `codegen.c`)

### Por que MIPS

MIPS é uma arquitetura RISC de 32 bits com um conjunto de instruções limpo e bem documentado. O simulador MARS permite executar e depurar assembly MIPS sem hardware real. É a escolha padrão em cursos de compiladores e arquitetura de computadores.

### Variáveis locais na pilha

No MIPS, a convenção é usar o frame pointer (`$fp`) como referência fixa para variáveis locais, e o stack pointer (`$sp`) para alocar espaço. A pilha cresce para baixo (endereços decrescentes).

A estratégia é simples:

```
$fp → topo do frame no início do bloco
$sp → posição atual da pilha

variável x: offset = -4 relativo a $fp
variável y: offset = -8 relativo a $fp
```

Cada declaração emite `addiu $sp, $sp, -4` e registra o offset da variável. Para ler `x`, basta `lw $v0, -4($fp)`. Para escrever, `sw $v0, -4($fp)`.

### Escopos aninhados no codegen — `CGScope`

A pilha de escopos do codegen (`CGScope`) é independente da pilha semântica. Ela rastreia quantos bytes cada bloco alocou. Quando o bloco termina:

```c
addiu $sp, $sp, <bytes_alocados>
```

Isso desaloca as variáveis locais, restaurando a pilha ao estado anterior. Blocos aninhados funcionam corretamente porque cada escopo só desaloca o que ele mesmo alocou.

### A pilha temporária para expressões binárias

Avaliar `a + b` exige calcular `a` e `b` e combinar os resultados. O problema é que ambos usam `$v0` como registrador de retorno. A solução é usar a pilha como armazenamento temporário:

```
1. calcula 'a', resultado em $v0
2. push_eval: empilha $v0, decrementa $sp
3. calcula 'b', resultado em $v0
4. pop_eval_to $t0: desempilha para $t0, incrementa $sp
5. addu $v0, $t0, $v0   (combina)
```

Isso funciona para qualquer profundidade de aninhamento, como `(a + b) * (c - d)`.

### Labels únicos para controle de fluxo

Cada `if` e `while` precisa de labels para os saltos. O contexto mantém um contador `next_label_id` que incrementa a cada novo label:

```c
char *lbl_else  = make_label("else",  ctx->next_label_id);
char *lbl_endif = make_label("endif", ctx->next_label_id++);
```

Isso garante que dois `if` aninhados gerem labels distintos (`else_0`/`endif_1` e `else_2`/`endif_3`), evitando colisões.

### Strings na seção `.data`

Strings literais não podem ficar inline no `.text` (código). Elas precisam de um endereço de memória, que em MIPS fica na seção `.data`. O codegen faz uma **pré-passagem** pela AST (`collect_strings`) para coletar todas as strings antes de começar a emitir o `.text`. Cada string recebe um label único (`_str_0`, `_str_1`...).

Se a mesma string aparecer duas vezes, `intern_string_label` reutiliza o mesmo label. Isso é uma otimização simples mas correta.

### Syscalls em vez de chamadas de função

Em vez de implementar I/O do zero (o que exigiria lidar com o sistema operacional), o codegen usa as syscalls do simulador MARS:

```
li $v0, 1   + syscall  →  imprime inteiro em $a0
li $v0, 4   + syscall  →  imprime string no endereço em $a0
li $v0, 5   + syscall  →  lê inteiro, resultado em $v0
li $v0, 10  + syscall  →  encerra o programa
li $v0, 11  + syscall  →  imprime caractere em $a0
li $v0, 12  + syscall  →  lê caractere, resultado em $v0
```

Essas syscalls são específicas do MARS. Um programa MIPS real usaria chamadas de sistema do kernel Linux ou equivalente.

---

## Como as peças se encaixam — o fluxo de dados

```
arquivo.g
   │
   ▼
[g-v1.l / Flex]
   yylex() retorna tokens
   yylval.str carrega lexemas
   │
   ▼
[g-v1.y / Bison]
   yyparse() reconhece a gramática
   ações semânticas chamam ast_make_*()
   │
   ▼
[AST em memória]
   ast_root aponta para AST_PROGRAM
   todos data_type = INVALID
   │
   ▼
[semantic_check() — semantic.c]
   percorre a AST
   usa symtab para rastrear escopos
   anota data_type em cada nó
   │
   ▼
[AST anotada]
   data_type correto em cada expressão
   │
   ▼
[codegen_emit() — codegen.c]
   percorre a AST anotada
   emite assembly MIPS linha a linha
   │
   ▼
arquivo.s (assembly MIPS)
   │
   ▼
[MARS simulator]
   executa o programa
```

Cada seta representa uma passagem sobre os dados. O parser faz uma passagem produzindo a AST. A semântica faz uma passagem sobre a AST anotando tipos. O codegen faz uma passagem sobre a AST anotada emitindo assembly.

---

## Decisões de design que valem destacar

### 1. Módulos independentes com interfaces mínimas

Cada módulo expõe apenas o que é necessário. `semantic.h` expõe uma única função: `semantic_check`. `codegen.h` expõe uma única função: `codegen_emit`. Isso torna cada módulo testável de forma isolada — os testes de API em `tests/` fazem exatamente isso: constroem ASTs à mão e chamam `semantic_check` ou `codegen_emit` diretamente, sem o parser.

### 2. A AST como contrato entre fases

A AST é o único ponto de contato entre o parser, a semântica e o codegen. O parser não sabe nada sobre tipos. O codegen não sabe nada sobre a gramática. Cada fase faz seu trabalho na AST e passa adiante.

### 3. Erros reportados na linha do fonte, não da execução

O campo `line` em cada nó da AST, combinado com `@N.first_line` no Bison, garante que erros semânticos apontem para a linha original do programa-fonte. Sem isso, um erro de tipo em `x + c` (linha 10) poderia ser reportado na linha errada porque o nó foi criado durante uma redução que aconteceu depois.

### 4. Separação entre tabela de símbolos da semântica e do codegen

A fase semântica usa `SymbolTableStack` para verificar tipos. A fase de codegen tem sua própria pilha de escopos (`CGScope`) para rastrear offsets de variáveis. Elas fazem trabalhos diferentes e são mantidas separadas, mesmo que percorram a mesma estrutura de blocos.
