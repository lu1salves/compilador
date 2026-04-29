# Compilador G-V1

Compilador didático para a linguagem **G-V1**, uma linguagem imperativa simples com palavras-chave em português. Implementa o pipeline completo: análise léxica → sintática → AST → semântica → geração de código MIPS Assembly.

---

## Dependências

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install gcc flex bison make default-jre
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install gcc flex bison make java-latest-openjdk
```

| Ferramenta | Versão mínima | Função |
|------------|---------------|--------|
| gcc | 9+ | Compilar o compilador |
| flex | 2.6+ | Gerar o analisador léxico |
| bison | 3.0+ | Gerar o analisador sintático |
| make | 4.0+ | Automatizar o build |
| Java (JRE) | 8+ | Executar o emulador MARS |

### Emulador MIPS — MARS

O MARS (MIPS Assembler and Runtime Simulator) é necessário para executar os arquivos `.s` gerados pelo compilador.

1. Baixe o JAR do MARS em: **https://courses.missouristate.edu/KenVollmar/MARS/**
2. Salve o arquivo (ex.: `MARS_4_5.jar`) em um diretório de sua preferência.
3. Execute via terminal:

```bash
java -jar MARS_4_5.jar
```

Ou abra a interface gráfica clicando duas vezes no JAR (requer JRE instalado).

---

## Build

```bash
cd compiladores/g-v1_files
make
```

O executável `g-v1` será gerado na mesma pasta.

---

## Uso

```bash
# Verificar erros léxicos, sintáticos e semânticos
./g-v1 programa.g

# Exibir a AST
./g-v1 --ast programa.g

# Exibir a tabela de símbolos
./g-v1 --symtab programa.g

# Gerar assembly MIPS na saída padrão
./g-v1 --code programa.g

# Salvar assembly MIPS em arquivo
./g-v1 -o programa.s programa.g
```

### Exemplo completo: compilar e rodar no MARS

```bash
./g-v1 -o saida.s tests/valid_03_if_while.g
java -jar MARS_4_5.jar saida.s
```

---

## Testes

Todos os testes ficam em `compiladores/g-v1_files/tests/`.

```
tests/
├── valid_*.g                        # Programas válidos (devem compilar sem erro)
├── invalid_lex_*.g                  # Erros léxicos
├── invalid_syn_*.g                  # Erros sintáticos
├── invalid_sem_*.g                  # Erros semânticos
├── ast_*.g                          # Testes de saída da AST
├── symtab_*.g                       # Testes da tabela de símbolos
├── codegen_*.g                      # Testes de geração de código MIPS
├── *_api_test.c                     # Testes unitários dos módulos
└── expected/                        # Saídas esperadas para comparação
```

### Executar os testes

```bash
make test-all        # Todos os testes
make test-valid      # Só programas válidos
make test-lex        # Só erros léxicos
make test-syntax     # Só erros sintáticos
make test-semantic   # Só erros semânticos
make test-ast        # Só saída de AST
make test-symtab     # Só tabela de símbolos
make test-codegen    # Só geração de código
make test-api        # Testes unitários dos módulos
```

---

## Limpeza

```bash
make clean
```

Remove o executável, objetos e arquivos gerados pelo Flex/Bison.

---

## Linguagem G-V1 — resumo rápido

```g
principal
{
    x: int;
    c: car;
}
{
    leia x;
    se (x > 0) entao
        escreva "positivo";
    senao
        escreva "nao positivo";
    fimse
    novalinha;
}
```

| Elemento | Sintaxe |
|----------|---------|
| Tipos | `int`, `car` |
| Entrada | `leia x` |
| Saída | `escreva expr` / `escreva "texto"` |
| Quebra de linha | `novalinha` |
| Condicional | `se (...) entao ... senao ... fimse` |
| Laço | `enquanto (...) cmd` |
| Comentário | `/* ... */` |

---

---

# Documentação técnica

---

## 1. Visão geral do compilador

O projeto implementa um compilador didático para a linguagem **G-V1**. O fluxo geral é este:

1. O usuário executa `./g-v1 arquivo.g`.
2. O `main`, definido no arquivo `g-v1.y`, processa as opções de linha de comando.
3. O parser (`yyparse`) começa a analisar o arquivo fonte.
4. Durante o parsing, o analisador léxico (`yylex`), gerado a partir de `g-v1.l`, reconhece tokens.
5. As ações semânticas do Bison montam a **AST** usando as funções de `ast.c`.
6. Depois do parsing, o compilador executa a **análise semântica** em `semantic.c`.
7. Se não houver erro, o compilador pode:
   - imprimir a AST (`--ast`),
   - imprimir os escopos/tabela de símbolos (`--symtab`),
   - gerar assembly MIPS (`--code` ou `-S`).
8. A geração de código é feita em `codegen.c`.

Os blocos principais do projeto são:

- `g-v1.l`: léxico
- `g-v1.y`: sintático + AST + main
- `ast.[ch]`: estrutura da árvore sintática abstrata
- `symtab.[ch]`: pilha de tabelas de símbolos
- `semantic.[ch]`: checagem semântica
- `codegen.[ch]`: geração de código MIPS
- `Makefile`: build e testes
- `tests/`: suíte de validação

---

## 2. Arquitetura lógica do projeto

### 2.1. Léxico

O léxico transforma caracteres em tokens. Exemplos:

- `principal` → token `PRINCIPAL`
- `x` → token `IDENTIFICADOR`
- `123` → token `INTCONST`
- `'a'` → token `CARCONST`
- `"abc"` → token `CADEIACARACTERES`

Também detecta erros como caractere inválido, comentário não encerrado e string que atravessa linha.

### 2.2. Sintático

O parser verifica se a sequência de tokens obedece à gramática da G-V1. Ao mesmo tempo, constrói a AST: o parser não só diz "está certo" ou "está errado"; ele também cria uma representação estruturada do programa.

### 2.3. AST

A AST representa o programa de forma mais compacta do que a árvore sintática concreta. Ela guarda:

- tipo de nó (`AST_IF`, `AST_ASSIGN`, `AST_IDENT` etc.);
- linha do fonte;
- tipo semântico quando disponível (`int`, `car`);
- lexema quando necessário;
- ponteiros para filhos e próximo nó da lista.

### 2.4. Tabela de símbolos

A tabela de símbolos controla escopos. Como a linguagem permite blocos aninhados, um identificador pode existir em um escopo externo e outro com o mesmo nome em um bloco interno. A estrutura foi implementada como **pilha de escopos**.

### 2.5. Semântica

A análise semântica verifica regras que a gramática sozinha não consegue garantir:

- uso de variável não declarada;
- redeclaração no mesmo escopo;
- tipos incompatíveis em atribuição;
- uso de `car` em operação aritmética;
- operação lógica com tipos errados.

### 2.6. Geração de código

Depois que o programa passa pela semântica, o compilador percorre a AST e emite assembly MIPS:

- alocação de variáveis locais na pilha;
- leitura e escrita;
- avaliação de expressões;
- emissão de labels para `if` e `while`;
- suporte a blocos e escopos aninhados;
- seção `.data` para strings.

---

## 3. Arquivo `g-v1.l`

### 3.1. Função do arquivo

`g-v1.l` é a especificação do **Flex**. Ele define como o analisador léxico reconhece os tokens da linguagem.

### 3.2. Opções do Flex

```lex
%option noyywrap yylineno noinput nounput
```

- `noyywrap`: evita precisar implementar `yywrap`.
- `yylineno`: o Flex controla o número da linha atual.
- `noinput` e `nounput`: evitam geração de funções auxiliares não usadas.

### 3.3. Bloco `%{ ... %}`

Inclui `stdio.h`, `stdlib.h`, `string.h` e `g-v1.tab.h`. Define a macro `YY_USER_ACTION`, que preenche `yylloc` com a linha atual a cada token reconhecido. Guarda `comment_start_line` e `string_start_line` para apontar a linha correta nos erros.

### 3.4. Estados léxicos

```lex
%x COMMENT STRING
```

- `COMMENT`: enquanto lê um comentário `/* ... */`
- `STRING`: enquanto lê uma string `"..."`

### 3.5. Macros léxicas

| Macro | O que reconhece |
|-------|----------------|
| `LETRA` | letras ou `_` |
| `DIGITO` | dígitos |
| `ID` | identificadores válidos |
| `INTLIT` | inteiros sem sinal |
| `CAR_ESC` | escapes como `\n`, `\t`, `\'`, `\\` |
| `CARACTERE` | literais como `'a'` ou `'\n'` |

### 3.6. Palavras reservadas

`principal`, `int`, `car`, `leia`, `escreva`, `novalinha`, `se`, `entao`, `senao`, `fimse`, `enquanto` — cada uma retorna seu token correspondente.

### 3.7. Operadores compostos

`||` → `OU`, `&` → `E`, `==` → `IGUAL`, `!=` → `DIFERENTE`, `>=` → `MAIORIGUAL`, `<=` → `MENORIGUAL`.

### 3.8. Literais e identificadores

`CARACTERE`, `INTLIT` e `ID` armazenam o texto bruto em `yylval.str`. O texto bruto é importante porque o parser usa esse lexema para criar nós da AST.

### 3.9. Regra de erro genérica

```lex
. { lexical_error_at("CARACTERE INVALIDO", yylineno); }
```

Qualquer caractere não reconhecido cai aqui.

### 3.10. Funções auxiliares

- `xstrdup_local`: duplica lexemas em memória dinâmica, evitando depender do buffer interno do Flex.
- `lexical_error_at`: imprime a mensagem no formato exato esperado pelos testes e encerra com falha.

---

## 4. Arquivo `g-v1.y`

### 4.1. Função do arquivo

`g-v1.y` é a especificação do **Bison** e faz três papéis:

1. define a gramática da linguagem;
2. cria a AST durante o parsing;
3. implementa o `main` do compilador e integra semântica, AST, tabela e codegen.

### 4.2. `%union`

O parser manipula três tipos de valores semânticos:

- `char *str`: lexemas vindos do léxico
- `ASTNode *node`: nós da árvore
- `int type_code`: código de tipo (`AST_TYPE_INT`, `AST_TYPE_CAR`)

### 4.3. Variáveis de controle

- `ast_root`: raiz da AST
- `opt_print_ast`: ativa impressão da árvore
- `opt_print_symtab`: ativa impressão dos escopos
- `opt_emit_code`: ativa geração de assembly
- `opt_output_path`: caminho de saída do `.s`

### 4.4. Regras da gramática e ações semânticas

| Não-terminal | Nó gerado |
|-------------|-----------|
| `DeclPrograma` | `AST_PROGRAM` |
| `Bloco` | `AST_BLOCK` (child1 = decls, child2 = cmds) |
| `ListaDeclVar` | lista de `AST_VAR_DECL` |
| `Comando: SE...FIMSE` | `AST_IF` |
| `Comando: ENQUANTO` | `AST_WHILE` |
| `Comando: LEIA` | `AST_READ` |
| `Comando: ESCREVA` | `AST_WRITE` |
| `Expr: IDENT = Expr` | `AST_ASSIGN` |
| `BinaryOp` | `AST_BINARY_OP` |
| `UnExpr` | `AST_UNARY_OP` |

A precedência de operadores é implementada pelos níveis `OrExpr → AndExpr → EqExpr → DesigExpr → AddExpr → MulExpr → UnExpr`.

### 4.5. `yyerror`

O parser ignora a mensagem detalhada do Bison e imprime sempre:

```text
ERRO: ERRO SINTATICO <linha>
```

Isso mantém a saída exatamente como a suíte de testes espera.

### 4.6. `main` — fluxo

1. lê as opções com `parse_cli`;
2. abre o arquivo de entrada em `yyin`;
3. chama `yyparse()`;
4. se bem-sucedido, roda `semantic_check`;
5. se a semântica passar: imprime AST / escopos / gera assembly conforme as flags;
6. libera a AST.

---

## 5. Arquivo `ast.h` / `ast.c`

### 5.1. `ASTKind` — 16 tipos de nós

`AST_PROGRAM`, `AST_BLOCK`, `AST_VAR_DECL`, `AST_EMPTY_STMT`, `AST_ASSIGN`, `AST_READ`, `AST_WRITE`, `AST_NEWLINE`, `AST_IF`, `AST_WHILE`, `AST_BINARY_OP`, `AST_UNARY_OP`, `AST_IDENT`, `AST_INT_CONST`, `AST_CHAR_CONST`, `AST_STRING_LITERAL`.

### 5.2. `ASTDataType`

- `AST_TYPE_INVALID`
- `AST_TYPE_INT`
- `AST_TYPE_CAR`

### 5.3. `struct ASTNode`

```c
typedef struct ASTNode {
    ASTKind      kind;
    int          line;
    ASTDataType  data_type;
    char        *lexeme;
    struct ASTNode *child1, *child2, *child3;
    struct ASTNode *next;   // encadeamento de listas
} ASTNode;
```

O campo `next` é fundamental porque várias listas da AST (declarações, comandos) são listas ligadas.

### 5.4. Funções principais

| Função | Descrição |
|--------|-----------|
| `ast_new` | função-base de criação de nós |
| `ast_make_*` | construtores específicos para cada tipo de nó |
| `ast_append` | concatena listas encadeadas |
| `ast_build_decl_list` | transforma `IDENT(x)->IDENT(y)` em `VAR_DECL(x:int)->VAR_DECL(y:int)` |
| `ast_print` | imprime a árvore textualmente |
| `ast_free` | libera recursivamente toda a memória |

---

## 6. Arquivo `symtab.h` / `symtab.c`

### 6.1. Estruturas

```c
typedef struct SymbolEntry {
    char *name;
    ASTDataType type;
    int decl_line, scope_level, scope_id;
    struct SymbolEntry *next;
} SymbolEntry;

typedef struct ScopeFrame {
    int scope_id, level;
    SymbolEntry *symbols;
    struct ScopeFrame *next;
} ScopeFrame;

typedef struct SymbolTableStack {
    ScopeFrame *top;
    int size, next_scope_id;
} SymbolTableStack;
```

### 6.2. Funções principais

| Função | Descrição |
|--------|-----------|
| `symtab_init` | inicializa a pilha vazia |
| `symtab_push_scope` | empilha novo escopo (id único, nível = tamanho atual) |
| `symtab_pop_scope` | remove o escopo do topo e libera seus símbolos |
| `symtab_insert` | insere variável no escopo atual; rejeita redeclaração |
| `symtab_lookup_current` | procura apenas no escopo do topo (detecta redeclaração) |
| `symtab_lookup` | procura do topo até a base (implementa escopo léxico / shadowing) |
| `symtab_print_stack` | imprime todos os escopos no formato esperado pelos testes |
| `symtab_dump_from_ast` | reconstrói e imprime escopos percorrendo a AST |

### 6.3. Como o shadowing funciona

`symtab_lookup` começa do topo da pilha. O identificador do escopo mais interno é encontrado antes do externo.

---

## 7. Arquivo `semantic.h` / `semantic.c`

### 7.1. Interface pública

```c
int semantic_check(ASTNode *root);
```

Retorna `1` se o programa é semanticamente válido, `0` se houver erro.

### 7.2. `SemanticContext`

Agrupa estado interno: pilha de símbolos e flag `error_reported` para parar na primeira violação detectada.

### 7.3. Regras verificadas

| Regra | Erro emitido |
|-------|-------------|
| Variável usada sem declaração | `IDENTIFICADOR NAO DECLARADO` |
| Variável declarada duas vezes no mesmo escopo | `IDENTIFICADOR JA DECLARADO` |
| Atribuição com tipos incompatíveis | `TIPOS INCOMPATIVEIS` |
| Aritmética com `car` | `OPERACAO ARITMETICA REQUER INT` |
| Lógica com `car` | `OPERACAO LOGICA REQUER INT` |
| Relacional entre tipos diferentes | `TIPOS INCOMPATIVEIS` |

### 7.4. Anotação de tipos

`analyze_identifier_use` copia o tipo da entrada da tabela para `node->data_type`. Isso é importante porque a geração de código consulta esse campo para escolher a syscall correta de I/O.

### 7.5. Fluxo em `analyze_block`

1. empilha escopo;
2. analisa declarações;
3. analisa comandos;
4. desempilha escopo.

Isso faz a tabela de símbolos acompanhar corretamente blocos aninhados.

---

## 8. Arquivo `codegen.h` / `codegen.c`

### 8.1. Interface pública

```c
int codegen_emit(FILE *out, const ASTNode *root);
```

Recebe um `FILE *` porque a saída pode ir tanto para `stdout` quanto para um arquivo `.s`.

### 8.2. Estruturas internas

| Estrutura | Descrição |
|-----------|-----------|
| `CGVar` | variável com `name`, `type`, `offset` relativo ao `$fp` |
| `CGScope` | escopo com `alloc_bytes` e lista de `CGVar` |
| `StringLiteralEntry` | tabela de strings com label para a seção `.data` |
| `CodegenContext` | estado global: arquivo de saída, pilha de escopos, contadores de labels |

### 8.3. Alocação de variáveis

Cada declaração:
1. soma 4 bytes ao escopo atual e ao `frame_bytes` total;
2. emite `addiu $sp, $sp, -4`;
3. inicializa a posição com zero;
4. cria `CGVar` com `offset = -frame_bytes`.

O offset negativo relativo a `$fp` funciona porque o frame cresce para baixo na pilha.

### 8.4. Avaliação de expressões — pilha temporária

Para expressões binárias:
1. gera filho esquerdo em `$v0` e empilha (`push_eval`);
2. gera filho direito em `$v0`;
3. desempilha para `$t0` (`pop_eval_to`);
4. combina `$t0` e `$v0`.

### 8.5. Operadores binários emitidos

| Operador | Instrução MIPS |
|----------|---------------|
| `+` | `addu` |
| `-` | `subu` |
| `*` | `mul` |
| `/` | `div` + `mflo` |
| `\|\|` | normaliza com `sltu` + `or` |
| `&` | normaliza com `sltu` + `and` |
| `==` | `xor` + `sltiu` |
| `!=` | `xor` + `sltu` |
| `<` | `slt` |
| `>` | `slt` invertendo operandos |
| `<=` | `slt` + `xori 1` |
| `>=` | `slt` invertido + `xori 1` |

### 8.6. Syscalls de I/O

| Operação | Syscall |
|----------|---------|
| `escreva` (int) | 1 |
| `escreva` (string) | 4 |
| `leia` (int) | 5 |
| encerramento | 10 |
| `escreva` (car) | 11 |
| `leia` (car) | 12 |

### 8.7. Controle de fluxo

- `AST_IF`: gera labels `else_N` e `endif_N`; salta para `else_N` se condição for zero.
- `AST_WHILE`: gera labels `while_begin_N` e `while_end_N`; volta ao início após o corpo.

### 8.8. Fluxo de `codegen_emit`

1. coleta strings da AST;
2. emite `.data` com as strings;
3. emite `.text` e `main:`;
4. inicializa `move $fp, $sp`;
5. gera o bloco principal;
6. emite syscall 10 (encerramento).

---

## 9. Descrição dos testes

### 9.1. Programas válidos

| Arquivo | O que valida |
|---------|-------------|
| `valid_01_minimo.g` | programa mínimo; teste de fumaça mais básico |
| `valid_02_nested_scopes.g` | tipos `int` e `car`, shadowing de variável, blocos aninhados |
| `valid_03_if_while.g` | `while`, `if/else`, relacionais, escrita de string |
| `valid_04_io_and_types.g` | `leia` e `escreva` para ambos os tipos |
| `valid_05_arith_write.g` | atribuição aritmética e escrita de inteiro |
| `valid_06_if_sem_senao.g` | `se/entao/fimse` sem ramo `senao` |
| `valid_07_if_else_simples.g` | `se/entao/senao/fimse` simples |
| `valid_08_if_aninhado.g` | `if/else` aninhado dois níveis |
| `valid_09_if_triplo_aninhado.g` | `if` aninhado três níveis com escrita de string |

Saída esperada: nenhuma (compilação silenciosa).

### 9.2. Erros léxicos

| Arquivo | Erro validado |
|---------|--------------|
| `invalid_lex_01_caractere.g` | `CARACTERE INVALIDO` — símbolo `@` |
| `invalid_lex_02_comment.g` | `COMENTARIO NAO TERMINA` — `/*` sem fechamento |
| `invalid_lex_03_string.g` | `CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA` |

### 9.3. Erros sintáticos

| Arquivo | Erro validado |
|---------|--------------|
| `invalid_syn_01_missing_semicolon.g` | `;` faltando depois de atribuição |
| `invalid_syn_02_missing_fimse.g` | `se/entao` sem `fimse` |
| `invalid_syn_03_escreva_sem_ponto_virgula.g` | `;` faltando depois de `escreva` |

### 9.4. Erros semânticos

| Arquivo | Erro validado |
|---------|--------------|
| `invalid_sem_01_undeclared.g` | uso de variável não declarada |
| `invalid_sem_02_redecl_same_scope.g` | declaração duplicada no mesmo bloco |
| `invalid_sem_03_assign_type.g` | `int = car` — tipos incompatíveis |
| `invalid_sem_04_arith_requires_int.g` | `car + car` — aritmética requer `int` |
| `invalid_sem_05_relational_mismatch.g` | `int == car` — relacional entre tipos diferentes |
| `invalid_sem_06_logic_requires_int.g` | `car & car` — lógica requer `int` |

### 9.5. Testes de AST

| Arquivo | O que valida |
|---------|-------------|
| `ast_01_assign.g` | nós `AST_ASSIGN` e `AST_WRITE` |
| `ast_02_control.g` | nós `AST_WHILE` e `AST_IF` aninhados |

### 9.6. Testes de tabela de símbolos

| Arquivo | O que valida |
|---------|-------------|
| `symtab_01_nested.g` | dois escopos distintos, nível e linha da declaração |
| `symtab_02_empty_scope.g` | escopo vazio (imprime `<vazio>`) |

### 9.7. Testes de geração de código

| Arquivo | O que valida |
|---------|-------------|
| `codegen_01_assign_write.g` | alocação, atribuição, syscall de escrita de inteiro |
| `codegen_02_if_else.g` | seção `.data`, labels `if/else`, syscall de escrita de string |
| `codegen_03_nested_scope.g` | offsets `$fp` em escopos aninhados, desalocação correta |

### 9.8. Testes de API (unitários em C)

| Arquivo | O que testa |
|---------|------------|
| `symtab_api_test.c` | inserção, redeclaração, lookup, shadowing, pop |
| `semantic_api_test.c` | semântica em ASTs construídas à mão (válida, não declarada, tipos) |
| `codegen_api_test.c` | emissão de assembly a partir de AST construída à mão |

---

## 10. Pontos fortes do projeto

1. **Separação por fases**: cada fase do compilador está em um módulo distinto.
2. **AST limpa**: compacta e reutilizável em semântica e codegen.
3. **Tabela de símbolos por pilha**: combina bem com blocos aninhados e shadowing.
4. **Semântica independente do parser**: testável com ASTs construídas à mão.
5. **Codegen desacoplado**: também pode ser testado a partir de AST pronta.
6. **Suíte de testes por fase**: facilita depuração e demonstra robustez por camada.

---

## 11. Perguntas frequentes

**Por que usar `next` na AST?**
Várias partes da árvore representam listas (comandos, declarações). Em vez de criar um nó "lista", foi mais simples encadear nós homogêneos.

**Por que guardar `line` em cada nó?**
Para que erros semânticos possam apontar a linha correta mesmo depois que o parser já terminou.

**Como o shadowing funciona?**
`symtab_lookup` começa do topo da pilha; o identificador do escopo mais interno é encontrado antes do externo.

**Por que o parser já cria a AST?**
Elimina uma fase extra de reconstrução. O próprio reconhecimento sintático monta a representação intermediária.

**Como o codegen sabe o tipo do que está imprimindo?**
A fase semântica anota `data_type` nos nós da AST; o codegen consulta esse campo para escolher a syscall correta.

**Como variáveis locais são alocadas?**
Cada declaração reserva 4 bytes na pilha com um `offset` negativo relativo ao `$fp`.

**Como `if` e `while` são gerados?**
Com labels únicos (`else_N`, `endif_N`, `while_begin_N`, `while_end_N`) e instruções de desvio (`beq`, `j`).

---

## 12. Resumo do fluxo (para apresentação)

> "O compilador recebe um programa G-V1, faz análise léxica com Flex, análise sintática com Bison, constrói uma AST durante o parsing, percorre essa AST para fazer análise semântica com apoio de uma pilha de tabelas de símbolos e, se o programa for válido, percorre a AST novamente para gerar código MIPS. O projeto também tem uma suíte de testes por fase, cobrindo léxico, sintaxe, semântica, AST, escopos e geração de código."
