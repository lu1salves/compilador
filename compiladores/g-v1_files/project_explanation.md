# Guia completo do projeto G-V1

Este documento foi feito para te ajudar a **explicar o trabalho inteiro** em apresentação, revisão e estudo. A ideia é percorrer **arquivo por arquivo**, mostrando:

- qual é o papel daquele arquivo no compilador;
- como cada parte foi organizada;
- como as funções se conectam;
- o que cada teste valida;
- como o fluxo completo do compilador funciona.

> **Observação importante:** este guia explica os **arquivos-fonte e os arquivos de teste** do projeto. Arquivos gerados (`.o`, binários de teste, `g-v1.tab.c`, `lex.yy.c`, zip de entrega etc.) não foram detalhados individualmente porque são artefatos de compilação ou empacotamento, não lógica-fonte do projeto.

---

# 1. Visão geral do compilador

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

Em outras palavras, os blocos principais do projeto são:

- `g-v1.l`: léxico
- `g-v1.y`: sintático + AST + main
- `ast.[ch]`: estrutura da árvore sintática abstrata
- `symtab.[ch]`: pilha de tabelas de símbolos
- `semantic.[ch]`: checagem semântica
- `codegen.[ch]`: geração de código MIPS
- `Makefile`: build e testes
- `tests/`: suíte de validação

---

# 2. Arquitetura lógica do projeto

## 2.1. Léxico

O léxico transforma caracteres em tokens. Exemplos:

- `principal` -> token `PRINCIPAL`
- `x` -> token `IDENTIFICADOR`
- `123` -> token `INTCONST`
- `'a'` -> token `CARCONST`
- `"abc"` -> token `CADEIACARACTERES`

Também detecta erros como:

- caractere inválido;
- comentário não encerrado;
- string que atravessa linha.

## 2.2. Sintático

O parser verifica se a sequência de tokens obedece à gramática da G-V1.

Ao mesmo tempo, ele constrói a AST. Isso é importante: o parser não só diz “está certo” ou “está errado”; ele também cria uma representação estruturada do programa.

## 2.3. AST

A AST representa o programa de forma mais compacta do que a árvore sintática concreta. Ela guarda:

- tipo de nó (`AST_IF`, `AST_ASSIGN`, `AST_IDENT` etc.);
- linha do fonte;
- tipo semântico quando disponível (`int`, `car`);
- lexema quando necessário;
- ponteiros para filhos e próximo nó da lista.

## 2.4. Tabela de símbolos

A tabela de símbolos é usada para controlar escopos. Como a linguagem permite blocos aninhados, um identificador pode existir em um escopo externo e outro com o mesmo nome em um bloco interno.

A estrutura foi implementada como **pilha de escopos**.

## 2.5. Semântica

A análise semântica verifica regras que a gramática sozinha não consegue garantir, por exemplo:

- uso de variável não declarada;
- redeclaração no mesmo escopo;
- tipos incompatíveis em atribuição;
- uso de `car` em operação aritmética;
- operação lógica com tipos errados.

## 2.6. Geração de código

Depois que o programa passa pela semântica, o compilador percorre a AST e emite assembly MIPS.

A geração faz:

- alocação de variáveis locais na pilha;
- leitura e escrita;
- avaliação de expressões;
- emissão de labels para `if` e `while`;
- suporte a blocos e escopos aninhados;
- seção `.data` para strings.

---

# 3. Arquivo `g-v1.l`

## 3.1. Função do arquivo

`g-v1.l` é a especificação do **Flex**. Ele define como o analisador léxico reconhece os tokens da linguagem.

## 3.2. Opções do Flex

```lex
%option noyywrap yylineno noinput nounput
```

Essas opções fazem o seguinte:

- `noyywrap`: evita precisar implementar `yywrap`.
- `yylineno`: o Flex passa a controlar o número da linha atual.
- `noinput` e `nounput`: evitam geração de funções auxiliares que não são usadas.

## 3.3. Bloco `%{ ... %}`

Esse bloco é copiado diretamente para o arquivo C gerado pelo Flex.

### Includes

- `stdio.h`, `stdlib.h`, `string.h`
- `g-v1.tab.h`: cabeçalho gerado pelo Bison com os tokens e o `YYSTYPE`

### `YY_USER_ACTION`

```c
#define YY_USER_ACTION do { ... } while (0)
```

Essa macro é executada a cada token reconhecido e preenche `yylloc` com a linha atual. Isso faz o parser saber em que linha cada token apareceu.

### Variáveis auxiliares

- `comment_start_line`: guarda a linha onde um comentário começou.
- `string_start_line`: guarda a linha onde uma string começou.

Isso é importante porque, se o comentário ou a string não terminarem, a mensagem de erro precisa apontar a linha correta.

### Protótipos auxiliares

- `xstrdup_local`: duplica strings com alocação dinâmica.
- `lexical_error_at`: imprime erro léxico e encerra.

## 3.4. Estados léxicos

```lex
%x COMMENT STRING
```

O léxico usa dois estados exclusivos:

- `COMMENT`: enquanto está lendo um comentário `/* ... */`
- `STRING`: enquanto está lendo uma string `"..."`

Isso simplifica muito o tratamento de comentários e cadeias de caracteres.

## 3.5. Macros léxicas

### `LETRA`
Reconhece letras ou `_`.

### `DIGITO`
Reconhece dígitos.

### `ID`
Reconhece identificadores válidos.

### `INTLIT`
Reconhece inteiros sem sinal, compostos por dígitos.

### `CAR_ESC`
Reconhece um escape como `\n`, `\t`, `\'`, `\\`.

### `CAR_BODY`
Define o conteúdo permitido dentro de uma constante caractere.

### `CARACTERE`
Reconhece um literal como `'a'` ou `'\n'`.

## 3.6. Regras de espaços e quebras de linha

```lex
[ \t\r]+   { }
\n         { }
```

O compilador ignora espaços, tabs e quebras de linha. O número de linha continua sendo atualizado pelo próprio Flex por causa de `yylineno`.

## 3.7. Regras de comentário

### Início do comentário
```lex
"/*" { comment_start_line = yylineno; BEGIN(COMMENT); }
```

Guarda a linha inicial e entra no estado `COMMENT`.

### Fim do comentário
```lex
<COMMENT>"*/" { BEGIN(INITIAL); }
```

Sai do estado de comentário.

### Conteúdo do comentário

- `<COMMENT>\n { }`
- `<COMMENT>. { }`

Tudo é descartado.

### EOF dentro de comentário
```lex
<COMMENT><<EOF>> { lexical_error_at("COMENTARIO NAO TERMINA", comment_start_line); }
```

Se o arquivo terminar antes de `*/`, o compilador imprime o erro e aborta.

## 3.8. Regras de string

### Início
```lex
\" { string_start_line = yylineno; BEGIN(STRING); yymore(); }
```

Entra no estado `STRING` e usa `yymore()` para continuar acumulando o lexema completo.

### Corpo da string

- `<STRING>[^\\\n\"]+ { yymore(); }`
- `<STRING>{CAR_ESC} { yymore(); }`

Essas regras consomem texto comum e escapes.

### Fim da string
```lex
<STRING>\" { BEGIN(INITIAL); yylval.str = xstrdup_local(yytext); return CADEIACARACTERES; }
```

Quando a aspas de fechamento aparece, a string completa é copiada para `yylval.str` e o token `CADEIACARACTERES` é retornado.

### Erros de string

- `<STRING>\n ...`
- `<STRING><<EOF>> ...`

Ambos geram a mensagem de string em mais de uma linha.

## 3.9. Palavras reservadas

Essas regras reconhecem palavras-chave da linguagem:

- `principal`
- `int`
- `car`
- `leia`
- `escreva`
- `novalinha`
- `se`
- `entao`
- `senao`
- `fimse`
- `enquanto`

Cada uma retorna seu token correspondente.

## 3.10. Operadores compostos

O arquivo reconhece:

- `||` -> `OU`
- `&` -> `E`
- `==` -> `IGUAL`
- `!=` -> `DIFERENTE`
- `>=` -> `MAIORIGUAL`
- `<=` -> `MENORIGUAL`

## 3.11. Literais e identificadores

### `CARACTERE`
Armazena o texto em `yylval.str` e retorna `CARCONST`.

### `INTLIT`
Armazena o texto em `yylval.str` e retorna `INTCONST`.

### `ID`
Armazena o texto em `yylval.str` e retorna `IDENTIFICADOR`.

A escolha de guardar o texto bruto é importante porque depois o parser usa esse lexema para criar nós da AST.

## 3.12. Pontuação simples

```lex
[{}();,:+\-*/<>=!] { return yytext[0]; }
```

Tokens de um caractere são devolvidos diretamente como o próprio caractere.

## 3.13. Fim de arquivo

```lex
<<EOF>> { return 0; }
```

No Bison, retornar `0` indica fim de entrada.

## 3.14. Regra de erro genérica

```lex
. { lexical_error_at("CARACTERE INVALIDO", yylineno); }
```

Qualquer caractere não reconhecido cai aqui.

## 3.15. Funções auxiliares no final do arquivo

### `xstrdup_local`
Duplica um lexema em memória dinâmica. Isso evita depender do buffer interno do Flex.

### `lexical_error_at`
Imprime a mensagem exatamente no formato esperado pelos testes e encerra com falha.

---

# 4. Arquivo `g-v1.y`

## 4.1. Função do arquivo

`g-v1.y` é a especificação do **Bison**. Ele faz três papéis ao mesmo tempo:

1. define a gramática da linguagem;
2. cria a AST durante o parsing;
3. implementa o `main` do compilador e integra semântica, AST, tabela e codegen.

## 4.2. `%code requires`

```bison
%code requires {
#include "ast.h"
}
```

Esse trecho garante que o tipo `ASTNode` esteja disponível no cabeçalho gerado pelo Bison, porque o `%union` usa ponteiros para AST.

## 4.3. Bloco `%{ ... %}`

Aqui ficam includes e variáveis globais usadas pelo parser.

### Includes

- `ast.h`: criação/impressão/liberação da AST
- `symtab.h`: dump dos escopos
- `semantic.h`: checagem semântica
- `codegen.h`: geração de assembly

### Declarações externas

- `yylineno`
- `yytext`
- `yylex`
- `yyin`

Esses símbolos são definidos pelo analisador léxico gerado pelo Flex.

### Variáveis de controle

- `ast_root`: raiz da AST
- `opt_print_ast`: ativa impressão da árvore
- `opt_print_symtab`: ativa impressão dos escopos
- `opt_emit_code`: ativa geração de assembly
- `opt_output_path`: caminho de saída do `.s`

### Protótipos locais

- `yyerror`
- `parse_cli`

## 4.4. Diretivas do Bison

### `%define parse.error verbose`
Pede mensagens de erro detalhadas. No projeto, isso não é usado diretamente porque `yyerror` padroniza a saída.

### `%locations`
Ativa o rastreamento de localização. Isso permite usar `@1.first_line`, `@2.first_line` etc. para guardar a linha correta em cada nó da AST.

## 4.5. `%union`

O parser manipula três tipos de valores semânticos:

- `char *str`: lexemas vindos do léxico
- `ASTNode *node`: nós da árvore
- `int type_code`: código de tipo (`AST_TYPE_INT`, `AST_TYPE_CAR`)

## 4.6. Tokens

Os tokens são declarados para casar com o que o Flex retorna.

Os tokens com `<str>` são os que carregam texto:

- `IDENTIFICADOR`
- `CADEIACARACTERES`
- `CARCONST`
- `INTCONST`

## 4.7. Tipos sintáticos (`%type`)

O Bison precisa saber que vários não-terminais retornam `ASTNode *`, enquanto `Tipo` retorna um código de tipo inteiro.

## 4.8. Símbolo inicial

```bison
%start Programa
```

Define a raiz da gramática.

## 4.9. Regras da gramática e ações semânticas

### `Programa`
Recebe `DeclPrograma`, salva em `ast_root` e devolve a raiz.

### `DeclPrograma`
Transforma `PRINCIPAL Bloco` em um nó `AST_PROGRAM`.

### `Bloco`
Existem dois formatos:

- bloco sem declaração: `{ ListaComando }`
- bloco com declaração: `VarSection { ListaComando }`

Nos dois casos é criado um nó `AST_BLOCK`. A convenção adotada é:

- `child1` = lista de declarações
- `child2` = lista de comandos

### `VarSection`
Representa a parte de declarações do bloco e devolve apenas a lista de declarações.

### `ListaDeclVar`
Converte listas de identificadores mais tipo em vários nós `AST_VAR_DECL`.

Exemplo:

```g
x, y: int;
```

vira dois nós de declaração: `x:int` e `y:int`.

### `IdList`
Cria uma lista encadeada de nós `AST_IDENT`. Depois essa lista é transformada em declarações por `ast_build_decl_list`.

### `Tipo`
Converte `INT` e `CAR` para `AST_TYPE_INT` e `AST_TYPE_CAR`.

### `ListaComando`
Monta uma lista encadeada de comandos com `ast_append`.

### `Comando`
Cada alternativa cria um tipo de nó diferente:

- `;` -> `AST_EMPTY_STMT`
- `Expr ;` -> a própria expressão
- `LEIA IDENTIFICADOR ;` -> `AST_READ`
- `ESCREVA Expr ;` -> `AST_WRITE`
- `ESCREVA CADEIACARACTERES ;` -> `AST_WRITE` com filho `AST_STRING_LITERAL`
- `NOVALINHA ;` -> `AST_NEWLINE`
- `SE (...) ENTAO ... FIMSE` -> `AST_IF`
- `SE (...) ENTAO ... SENAO ... FIMSE` -> `AST_IF` com ramo else
- `ENQUANTO (...) Comando` -> `AST_WHILE`
- `Bloco` -> o próprio nó de bloco

### `Expr`
Suporta:

- uma expressão comum (`OrExpr`)
- atribuição `IDENTIFICADOR = Expr`

A atribuição é modelada como expressão, o que combina com a gramática fornecida.

### `OrExpr`, `AndExpr`, `EqExpr`, `DesigExpr`, `AddExpr`, `MulExpr`, `UnExpr`
Esses níveis implementam precedência de operadores. Cada operador binário vira `AST_BINARY_OP`; cada operador unário vira `AST_UNARY_OP`.

### `PrimExpr`
Os termos básicos são:

- identificador
- caractere
- inteiro
- subexpressão entre parênteses

## 4.10. `yyerror`

```c
static void yyerror(const char *s)
```

O parser ignora a mensagem detalhada do Bison e imprime sempre:

```text
ERRO: ERRO SINTATICO <linha>
```

Isso mantém a saída exatamente como a suíte de testes espera.

## 4.11. `parse_cli`

Essa função interpreta as opções de linha de comando.

### Opções reconhecidas

- `--ast`
- `--symtab`
- `--code`
- `-S`
- `-o arquivo.s`

Ela também valida se o usuário passou exatamente um arquivo de entrada.

Se o usuário usa `-o`, a função automaticamente ativa também `opt_emit_code`.

## 4.12. `main`

O `main` é a cola do compilador inteiro.

Fluxo:

1. lê as opções com `parse_cli`;
2. abre o arquivo de entrada em `yyin`;
3. chama `yyparse()`;
4. fecha o arquivo;
5. se o parsing foi bem-sucedido e a AST existe, roda `semantic_check`;
6. se a semântica passar:
   - imprime AST se `--ast`;
   - imprime escopos se `--symtab`;
   - gera assembly se `--code` ou `-S`;
7. libera a AST no final.

Esse arquivo, portanto, não é só o parser: ele também é o **ponto de entrada do programa**.

---

# 5. Arquivo `ast.h`

## 5.1. Função do arquivo

Define a estrutura da **árvore sintática abstrata**.

## 5.2. `ASTKind`

Enumera todos os tipos de nós possíveis:

- `AST_PROGRAM`
- `AST_BLOCK`
- `AST_VAR_DECL`
- `AST_EMPTY_STMT`
- `AST_ASSIGN`
- `AST_READ`
- `AST_WRITE`
- `AST_NEWLINE`
- `AST_IF`
- `AST_WHILE`
- `AST_BINARY_OP`
- `AST_UNARY_OP`
- `AST_IDENT`
- `AST_INT_CONST`
- `AST_CHAR_CONST`
- `AST_STRING_LITERAL`

## 5.3. `ASTDataType`

Representa o tipo semântico do nó:

- `AST_TYPE_INVALID`
- `AST_TYPE_INT`
- `AST_TYPE_CAR`

## 5.4. `struct ASTNode`

Campos principais:

- `kind`: que tipo de nó é
- `line`: linha no arquivo fonte
- `data_type`: tipo semântico anotado
- `lexeme`: texto associado, quando existe
- `child1`, `child2`, `child3`: filhos
- `next`: encadeamento de listas

A presença de `next` é importante porque várias listas da AST são listas ligadas, como:

- lista de declarações
- lista de comandos
- lista de identificadores antes de virar declaração

## 5.5. Protótipos declarados

O header declara funções para:

- criar cada tipo de nó;
- anexar listas;
- transformar lista de identificadores em lista de declarações;
- imprimir a árvore;
- liberar memória.

---

# 6. Arquivo `ast.c`

## 6.1. Função do arquivo

Implementa toda a infraestrutura da AST.

## 6.2. `xmalloc`

Função utilitária para alocação segura. Se `malloc` falhar, o compilador imprime erro e encerra.

## 6.3. `xstrdup_ast`

Duplica lexemas para a AST. Isso evita depender do buffer do lexer, que pode mudar de conteúdo a cada token.

## 6.4. `ast_new`

É a função-base de criação de nós.

Recebe:

- tipo do nó
- linha
- lexema opcional
- até três filhos

Ela inicializa também:

- `data_type = AST_TYPE_INVALID`
- `next = NULL`

Quase todas as funções `ast_make_*` chamam `ast_new` por baixo.

## 6.5. Construtores específicos

### `ast_make_program`
Cria a raiz `AST_PROGRAM`.

### `ast_make_block`
Cria um bloco, com declarações em `child1` e comandos em `child2`.

### `ast_make_decl`
Cria `AST_VAR_DECL` e já grava o tipo da variável.

### `ast_make_ident`
Cria um identificador.

### `ast_make_int_const`
Cria constante inteira.

### `ast_make_char_const`
Cria constante caractere.

### `ast_make_string_literal`
Cria nó de string literal.

### `ast_make_empty_stmt`
Cria um comando vazio `;`.

### `ast_make_assign`
Cria uma atribuição com lado esquerdo e direito.

### `ast_make_read`
Cria comando `leia`.

### `ast_make_write`
Cria comando `escreva`.

### `ast_make_newline`
Cria `novalinha`.

### `ast_make_if`
Cria `if`, com `child1` = condição, `child2` = then, `child3` = else.

### `ast_make_while`
Cria `while`, com condição e corpo.

### `ast_make_binary_op`
Cria operador binário e guarda o lexema do operador, por exemplo `+`, `<`, `==`.

### `ast_make_unary_op`
Cria operador unário, como `-` ou `!`.

## 6.6. `ast_append`

Função de lista encadeada.

Se a lista for `NULL`, devolve o nó.
Caso contrário, anda até o fim e coloca o novo nó em `tail->next`.

Ela é usada o tempo todo no parser para:

- concatenar comandos;
- concatenar declarações;
- concatenar identificadores.

## 6.7. `ast_build_decl_list`

Essa função é especialmente importante.

O parser primeiro monta algo como:

- `IDENT(x)` -> `IDENT(y)` -> `IDENT(z)`

Depois, quando encontra `: int;`, chama `ast_build_decl_list` para transformar essa lista em:

- `VAR_DECL(x:int)` -> `VAR_DECL(y:int)` -> `VAR_DECL(z:int)`

Ela também libera os nós temporários de identificador usados apenas nessa etapa.

## 6.8. `ast_type_name`

Converte `ASTDataType` para string humana:

- `int`
- `car`
- `<tipo-invalido>`

Essa função é usada principalmente em impressão de AST e symtab.

## 6.9. Impressão da AST

### `print_indent`
Imprime recuo para a árvore ficar legível.

### `ast_print_list`
Imprime listas com rótulo, por exemplo `decls:` e `commands:`.

### `ast_print_node`
Função recursiva que imprime cada tipo de nó de forma legível.

Exemplos:

- `PROGRAM line=1`
- `BLOCK line=2`
- `VAR_DECL line=3 name=x type=int`
- `BINARY_OP line=6 op=+`

Ela faz o papel de visualizador textual da árvore.

### `ast_print`
Interface pública: chama `ast_print_node` a partir da raiz.

## 6.10. `ast_free`

Libera a árvore inteira recursivamente:

- filhos
- listas encadeadas em `next`
- lexema
- nó atual

Sem essa função, o projeto teria vazamentos significativos de memória.

---

# 7. Arquivo `symtab.h`

## 7.1. Função do arquivo

Define a estrutura da tabela de símbolos com escopos empilhados.

## 7.2. `SymbolEntry`

Representa uma variável declarada.

Campos:

- `name`
- `type`
- `decl_line`
- `scope_level`
- `scope_id`
- `next`

`next` encadeia símbolos dentro do mesmo escopo.

## 7.3. `ScopeFrame`

Representa um escopo individual.

Campos:

- `scope_id`: identificador único do escopo
- `level`: profundidade na pilha
- `symbols`: lista de símbolos daquele escopo
- `next`: escopo abaixo na pilha

## 7.4. `SymbolTableStack`

É a pilha de escopos.

Campos:

- `top`
- `size`
- `next_scope_id`

## 7.5. Funções declaradas

- inicialização e destruição
- push/pop de escopo
- inserção
- lookup no escopo atual
- lookup na pilha inteira
- impressão do stack
- dump a partir da AST

---

# 8. Arquivo `symtab.c`

## 8.1. Função do arquivo

Implementa a pilha de tabelas de símbolos.

## 8.2. `xmalloc` e `xstrdup_sym`

Mesma ideia dos outros módulos: alocação segura e cópia de string.

## 8.3. `free_symbol_list`

Libera toda a lista de símbolos de um escopo.

## 8.4. `symtab_init`

Inicializa a pilha vazia:

- topo nulo
- tamanho zero
- próximo id de escopo em zero

## 8.5. `symtab_destroy`

Vai removendo escopos até a pilha ficar vazia. É um destrutor de alto nível.

## 8.6. `symtab_push_scope`

Cria um novo escopo no topo.

Detalhes importantes:

- o `scope_id` é único e incrementa;
- o `level` recebe o tamanho atual da pilha;
- a lista de símbolos começa vazia.

## 8.7. `symtab_pop_scope`

Remove o escopo atual, libera todos os seus símbolos e atualiza o topo.

Retorna `0` se não havia escopo para remover e `1` em caso de sucesso.

## 8.8. `symtab_lookup_current`

Procura um nome apenas no escopo do topo.

Essa função é usada para detectar **redeclaração no mesmo escopo**.

## 8.9. `symtab_lookup`

Procura um nome do topo até a base da pilha.

Essa função implementa corretamente a regra de escopo léxico da linguagem: o identificador mais interno visível é o primeiro encontrado.

## 8.10. `symtab_insert`

Insere uma nova variável no escopo atual.

Passos:

1. verifica se existe escopo;
2. verifica se o nome já existe no topo;
3. cria a entrada;
4. preenche nome, tipo, linha, nível e id de escopo;
5. coloca a entrada no final da lista de símbolos do escopo.

Ela retorna `NULL` se a inserção não puder ser feita.

## 8.11. `symtab_print_stack`

Imprime todos os escopos da pilha, do topo para baixo.

Formato:

```text
ESCOPO N (nivel L)
  x : int (linha 3)
```

## 8.12. Dump a partir da AST

Esse módulo também implementa uma forma de reconstruir os escopos diretamente percorrendo a AST.

### `dump_node`
Percorre comandos. Só entra em blocos, `if` e `while`, porque são os casos onde podem existir subcomandos e escopos internos.

### `dump_block`
Quando encontra um bloco:

1. empilha um novo escopo;
2. insere declarações do bloco na symtab;
3. imprime o escopo;
4. percorre os comandos internos;
5. desempilha o escopo.

### `symtab_dump_from_ast`
É a interface pública. Cria a pilha, começa pelo bloco principal e depois destrói tudo.

Esse recurso é útil para depurar e para demonstrar na apresentação como os escopos estão sendo montados.

---

# 9. Arquivo `semantic.h`

## 9.1. Função do arquivo

É um header bem pequeno que expõe a função principal da análise semântica:

- `semantic_check(ASTNode *root)`

Essa função retorna sucesso ou falha após percorrer a AST.

---

# 10. Arquivo `semantic.c`

## 10.1. Função do arquivo

Implementa a **análise semântica** da linguagem G-V1.

## 10.2. `SemanticContext`

Agrupa o estado da análise:

- `symtab`: pilha de escopos
- `error_reported`: flag para evitar imprimir vários erros em cascata

## 10.3. `semantic_error`

Imprime a mensagem de erro semântico no formato esperado e marca que já houve erro.

A ideia é parar na primeira violação detectada, como o enunciado pede.

## 10.4. Funções auxiliares de tipo

### `is_valid_type`
Retorna verdadeiro para `int` e `car`.

### `is_int_type`
Retorna verdadeiro só para `int`.

Essas funções deixam as regras semânticas mais legíveis.

## 10.5. Declaração antecipada das funções recursivas

O módulo é mutuamente recursivo, então declara antes:

- `analyze_expression`
- `analyze_command_list`
- `analyze_block`

## 10.6. `analyze_identifier_use`

Verifica uso de identificador.

Passos:

1. procura o nome na pilha de escopos;
2. se não achar, gera erro `IDENTIFICADOR NAO DECLARADO`;
3. se achar, copia o tipo da entrada da tabela para `node->data_type`.

Esse passo é importante porque anota a AST com tipos, o que depois ajuda a geração de código.

## 10.7. `analyze_binary_op`

Analisa operadores binários.

### Primeiro passo
Analisa os dois operandos recursivamente.

### Depois aplica a regra do operador

#### Operadores aritméticos: `+ - * /`
Exigem `int` dos dois lados e produzem `int`.

#### Operadores lógicos: `||` e `&`
Exigem `int` dos dois lados e produzem `int`.

#### Operadores relacionais: `== != < > <= >=`
Exigem que os dois operandos tenham o mesmo tipo válido e produzem `int`.

### Caso de operador desconhecido
Gera erro específico, o que também ajuda a depurar AST corrompida.

## 10.8. `analyze_unary_op`

Trata:

- `-expr`: exige `int`, retorna `int`
- `!expr`: exige `int`, retorna `int`

Se o operador for desconhecido, gera erro.

## 10.9. `analyze_expression`

É a função central da semântica de expressões.

### Casos tratados

#### `AST_IDENT`
Delegado para `analyze_identifier_use`.

#### `AST_INT_CONST`
Anota tipo `int`.

#### `AST_CHAR_CONST`
Anota tipo `car`.

#### `AST_STRING_LITERAL`
Marca como inválido em contexto de expressão comum. String só é aceita diretamente em `escreva`.

#### `AST_ASSIGN`
Analisa lado esquerdo e direito.

Depois verifica:

- o lado esquerdo precisa ser um identificador;
- os tipos devem ser compatíveis.

Se passar, o tipo da expressão de atribuição vira o tipo do lado esquerdo.

#### `AST_BINARY_OP`
Delegado para `analyze_binary_op`.

#### `AST_UNARY_OP`
Delegado para `analyze_unary_op`.

#### Caso padrão
Qualquer outro tipo de nó em posição de expressão é considerado inválido.

## 10.10. `analyze_declarations`

Percorre a lista de declarações do bloco.

Para cada declaração:

1. verifica se o nó é realmente `AST_VAR_DECL`;
2. verifica se o tipo é válido;
3. verifica se o nome já existe no escopo atual;
4. insere na tabela de símbolos.

Isso implementa a regra de redeclaração no mesmo escopo.

## 10.11. `analyze_command`

Analisa um comando individual.

### `AST_EMPTY_STMT` e `AST_NEWLINE`
Não exigem validação adicional.

### `AST_ASSIGN`
Reaproveita a análise de expressão.

### `AST_READ`
Exige que o filho seja um identificador válido já declarado.

### `AST_WRITE`
Se for string literal, aceita diretamente.
Caso contrário, analisa a expressão impressa.

### `AST_IF`
Analisa a condição.
Depois verifica se o tipo é válido para uso como condição.
Na implementação atual, tanto `int` quanto `car` passam por `is_valid_type`, então a condição precisa ser um tipo simples válido.

Depois analisa `then` e `else`.

### `AST_WHILE`
Mesma lógica do `if`, mas com corpo único.

### `AST_BLOCK`
Entra recursivamente em um novo bloco.

### Caso padrão
Qualquer outro nó inesperado em posição de comando gera erro.

## 10.12. `analyze_command_list`

Percorre a lista encadeada de comandos e chama `analyze_command` em cada um.

## 10.13. `analyze_block`

Implementa escopo léxico.

Passos:

1. verifica se o nó é realmente um bloco;
2. empilha um novo escopo;
3. analisa declarações do bloco;
4. se não houve erro, analisa os comandos;
5. desempilha o escopo.

Essa é a função que faz a tabela de símbolos acompanhar corretamente a entrada e saída de blocos aninhados.

## 10.14. `semantic_check`

É a interface pública do módulo.

Fluxo:

1. inicializa o contexto e a pilha de símbolos;
2. valida se a raiz existe e se é `AST_PROGRAM`;
3. chama `analyze_block` no bloco principal;
4. destrói a tabela de símbolos;
5. retorna `1` se tudo passou, `0` se houve erro.

---

# 11. Arquivo `codegen.h`

## 11.1. Função do arquivo

Expõe a função pública da geração de código:

- `codegen_emit(FILE *out, const ASTNode *root)`

Ela recebe um `FILE *` porque a saída pode ir tanto para `stdout` quanto para um arquivo `.s`.

---

# 12. Arquivo `codegen.c`

## 12.1. Função do arquivo

Implementa a geração de assembly MIPS a partir da AST já validada semanticamente.

## 12.2. Estruturas internas

### `CGVar`
Representa uma variável durante a geração.

Campos:

- `name`
- `type`
- `offset`
- `next`

`offset` é a posição relativa a `$fp`.

### `CGScope`
Representa um escopo de geração.

Campos:

- `alloc_bytes`: quantos bytes esse escopo alocou na pilha
- `vars`: lista de variáveis do escopo
- `next`: escopo abaixo

### `StringLiteralEntry`
Tabela de strings literais.

Cada entrada guarda:

- texto da string
- label que será emitido na `.data`
- ponteiro para a próxima string

### `CodegenContext`
Estado global da emissão.

Campos:

- `out`: arquivo de saída
- `scope_top`: topo da pilha de escopos de geração
- `next_label_id`: contador para labels de controle
- `next_string_id`: contador para labels de strings
- `strings`: tabela de strings coletadas
- `emitted_anything`: indica se alguma linha foi emitida
- `frame_bytes`: tamanho total atual do frame

## 12.3. Utilitários básicos

### `xmalloc` e `xstrdup_codegen`
Mesma ideia dos outros módulos: alocação segura e cópia de strings.

### `emit_line`
Emite uma linha formatada para o arquivo de saída e marca que houve emissão.

Essa função centraliza toda a escrita do assembly.

## 12.4. Pilha temporária de avaliação

### `push_eval`
Reserva 4 bytes na pilha e guarda `$v0`.

### `pop_eval_to`
Restaura o topo da pilha para um registrador arbitrário e desfaz a reserva.

Essas funções são usadas ao avaliar expressões binárias: calcula um lado, empilha, calcula o outro, desempilha.

## 12.5. Labels

### `make_label`
Gera nomes únicos como:

- `else_0`
- `endif_1`
- `while_begin_2`

Isso evita colisão entre diferentes `if` e `while`.

## 12.6. Escopos de geração

### `push_scope`
Cria um escopo novo de codegen.

### `free_var_list`
Libera a lista de variáveis de um escopo.

### `pop_scope`
Remove o escopo atual.

Se o escopo tinha variáveis locais, emite:

```asm
addiu $sp, $sp, <bytes>
```

para desalocar o espaço reservado por esse bloco.

Esse ponto é crucial para blocos aninhados.

## 12.7. Lookup e declaração de variáveis

### `lookup_var`
Procura a variável visível mais interna, do escopo atual para fora.

### `declare_var`
Ao declarar uma variável local:

1. soma 4 bytes ao escopo atual;
2. soma 4 bytes ao `frame_bytes` total;
3. emite `addiu $sp, $sp, -4`;
4. inicializa a posição com zero;
5. cria a entrada `CGVar` com offset `-frame_bytes`.

Esse offset negativo relativo a `$fp` funciona porque o frame cresce para baixo na pilha.

## 12.8. `parse_char_const`

Converte o lexema de caractere para valor numérico inteiro.

Exemplos:

- `'A'` -> 65
- `'\n'` -> 10
- `'\t'` -> 9

Isso é necessário porque, no assembly, o caractere precisa virar um inteiro literal.

## 12.9. Tabela de strings

### `intern_string_label`
Se uma string já foi vista, reutiliza o mesmo label. Caso contrário, cria uma entrada nova.

### `collect_strings`
Percorre a AST inteira e coleta todas as strings literais antes de começar a emitir `.text`.

### `emit_data_section`
Se houver strings, emite:

- a diretiva `.data`
- uma linha `.asciiz` para cada string
- uma linha em branco

## 12.10. Geração de expressões

### `gen_binary_truthy`
Transforma um valor arbitrário em 0 ou 1 usando:

```asm
sltu dst, $zero, src
```

É usado nas operações lógicas para simular “verdadeiro/falso”.

### `gen_expr`
Função central da emissão de expressões.

#### `AST_IDENT`
Carrega o valor da variável com `lw` usando o offset relativo a `$fp`.

#### `AST_INT_CONST`
Emite `li $v0, valor`.

#### `AST_CHAR_CONST`
Converte o caractere e emite `li $v0, ascii`.

#### `AST_ASSIGN`
Gera o lado direito, depois armazena em memória com `sw`.

#### `AST_UNARY_OP`
- `-` -> `subu $v0, $zero, $v0`
- `!` -> `sltiu $v0, $v0, 1`

#### `AST_BINARY_OP`
A estratégia é:

1. gerar filho esquerdo em `$v0`;
2. empilhar;
3. gerar filho direito em `$v0`;
4. desempilhar para `$t0`;
5. combinar `$t0` e `$v0`.

Operadores implementados:

- `+` -> `addu`
- `-` -> `subu`
- `*` -> `mul`
- `/` -> `div` + `mflo`
- `||` -> normaliza os dois lados e aplica `or`
- `&` -> normaliza os dois lados e aplica `and`
- `==` -> `xor` + `sltiu`
- `!=` -> `xor` + `sltu`
- `<` -> `slt`
- `>` -> `slt` invertendo ordem
- `<=` -> `slt` + `xori 1`
- `>=` -> `slt` invertido + `xori 1`

#### Caso padrão
Se aparecer algo inesperado, o gerador coloca zero em `$v0`.

## 12.11. Geração de comandos

### `gen_command_list`
Percorre a lista encadeada de comandos.

### `gen_command`
Implementa cada comando da linguagem.

#### `AST_EMPTY_STMT`
Não gera nada.

#### `AST_ASSIGN`
Reaproveita `gen_expr`.

#### `AST_READ`
Escolhe syscall de leitura com base no tipo da variável:

- `int` -> syscall `5`
- `car` -> syscall `12`

Depois armazena o valor lido na variável.

#### `AST_WRITE`
Se for string literal:

- carrega o label em `$a0`
- usa syscall `4`

Se for expressão comum:

1. gera a expressão em `$v0`
2. move para `$a0`
3. escolhe syscall:
   - `11` para `car`
   - `1` para `int`

#### `AST_NEWLINE`
Emite caractere ASCII 10 com syscall `11`.

#### `AST_IF`
Gera dois labels:

- `else`
- `endif`

Fluxo:

1. gera a condição;
2. se `$v0 == 0`, salta para o else ou fim;
3. gera o ramo then;
4. se houver else, salta para o fim e gera o ramo else;
5. emite o label final.

#### `AST_WHILE`
Gera dois labels:

- início do laço
- saída

Fluxo:

1. marca o início;
2. gera a condição;
3. se falsa, salta para saída;
4. gera o corpo;
5. volta ao início;
6. marca a saída.

#### `AST_BLOCK`
Chama `gen_block`, que cria um novo escopo de geração.

#### Caso padrão
Se aparecer um nó de expressão em posição de comando, ele chama `gen_expr`.

## 12.12. Declarações e blocos

### `gen_declarations`
Percorre as declarações do bloco e chama `declare_var` para cada variável.

### `gen_block`
Fluxo do bloco:

1. empilha escopo;
2. aloca variáveis declaradas;
3. gera comandos;
4. desaloca variáveis e desempilha escopo.

Essa função é a chave para fazer escopos internos funcionarem sem sobrescrever os externos.

## 12.13. Liberação das strings

### `free_string_table`
Libera a lista encadeada de strings internadas.

## 12.14. `codegen_emit`

Interface pública do módulo.

Fluxo:

1. valida se a raiz é um programa válido;
2. inicializa o contexto;
3. coleta strings da AST;
4. emite `.data`, se necessário;
5. emite `.text` e o rótulo `main`;
6. inicializa o frame pointer com `move $fp, $sp`;
7. gera o bloco principal;
8. emite syscall de término (`10`);
9. limpa qualquer escopo remanescente;
10. libera a tabela de strings.

O retorno indica se algo foi realmente emitido.

---

# 13. Arquivo `Makefile`

## 13.1. Função do arquivo

Automatiza:

- geração dos arquivos do Flex/Bison;
- compilação dos módulos;
- linkedição do executável;
- execução da suíte de testes.

## 13.2. Variáveis principais

### `CC`, `CFLAGS`
Definem compilador e flags padrão.

### `BISON`, `FLEX`
Permitem trocar os executáveis se necessário.

### `TARGET`
Nome do executável final: `g-v1`.

### `API_BINS`
Lista dos binários de testes em C.

### `OBJS`
Lista dos objetos necessários para o executável final.

## 13.3. `.PHONY`

Marca alvos que não correspondem a arquivos reais, como:

- `clean`
- `test`
- `test-all`
- `ast`
- `asm`

## 13.4. Alvo `all`

Depende de `$(TARGET)`, então `make` puro gera o compilador.

## 13.5. Regra do executável final

Linka todos os objetos em `g-v1`.

## 13.6. Regras de geração de parser e lexer

### `g-v1.tab.c g-v1.tab.h`
Roda o Bison em cima de `g-v1.y`.

### `lex.yy.c`
Roda o Flex em cima de `g-v1.l`.

## 13.7. Regras de compilação dos objetos

Compilam:

- parser
- lexer
- AST
- tabela de símbolos
- semântica
- geração de código

## 13.8. Regras dos testes de API

Compilam três pequenos programas C que testam módulos específicos sem depender do parser completo.

## 13.9. Alvos utilitários

### `ast`
Executa o compilador em modo `--ast`.

### `symtab`
Executa o compilador em modo `--symtab`.

### `asm`
Executa o compilador em modo `--code`.

## 13.10. Alvos de teste agregados

### `test`
Alias para `test-all`.

### `test-all`
Roda toda a suíte.

### `test-quick`
Roda um subconjunto útil para depuração rápida.

### `test-invalid`
Roda apenas os testes que precisam falhar.

## 13.11. Testes por categoria

### `test-valid`
Espera que os programas válidos não imprimam nada.

### `test-lex`
Executa os programas com erro léxico e compara a saída com o arquivo esperado.

### `test-syntax`
Mesma ideia para erros sintáticos.

### `test-semantic`
Mesma ideia para erros semânticos.

### `test-ast`
Executa `--ast` e compara a árvore textual.

### `test-symtab`
Executa `--symtab` e compara a impressão dos escopos.

### `test-codegen`
Executa `--code` e compara o assembly gerado com o `.s` esperado.

### `test-api`
Executa os binários de teste em C. Dois deles silenciam a saída normal porque o importante é o código de retorno.

## 13.12. `clean`

Remove:

- executável principal
- objetos
- arquivos gerados pelo Flex/Bison
- binários dos testes de API

---

# 14. Diretório `tests/`

A suíte foi organizada por objetivo. Isso é ótimo para apresentação porque mostra que o projeto foi validado por camada.

## 14.1. Estratégia geral da suíte

As categorias são:

- válidos
- erros léxicos
- erros sintáticos
- erros semânticos
- impressão de AST
- dump de tabela de símbolos
- geração de código
- testes de API em C

---

# 15. Arquivos `.g` válidos

## 15.1. `tests/valid_01_minimo.g`

### Conteúdo
Programa mínimo:

```g
principal
{
;
}
```

### O que valida

- parsing mais simples possível;
- comando vazio;
- bloco sem declarações.

### Por que ele existe
É o teste de fumaça mais básico do projeto.

## 15.2. `tests/valid_02_nested_scopes.g`

### O que valida

- múltiplas declarações
- tipos `int` e `car`
- atribuições
- operação aritmética
- bloco interno
- sombreamento de variável `x`
- escrita e `novalinha`

### Ideia principal
Mostra que um `x` interno pode esconder o `x` externo sem quebrar o uso do resto do programa.

## 15.3. `tests/valid_03_if_while.g`

### O que valida

- `while`
- `if/else`
- comparação relacional
- incremento
- escrita de string
- mistura de estruturas de controle

### Importância
Esse teste aproxima mais de um programa real e exercita várias partes ao mesmo tempo.

## 15.4. `tests/valid_04_io_and_types.g`

### O que valida

- `leia` para `int`
- `leia` para `car`
- `escreva` para `int`
- `escreva` para `car`
- `escreva` de string
- `novalinha`

### Importância
É o teste que cobre de forma mais direta a parte de I/O da linguagem.

---

# 16. Arquivos de erro léxico

## 16.1. `tests/invalid_lex_01_caractere.g`

Contém `@`, que não pertence à linguagem.

Valida a mensagem:

```text
ERRO: CARACTERE INVALIDO 3
```

## 16.2. `tests/invalid_lex_02_comment.g`

Abre `/*` e nunca fecha.

Valida que o lexer detecta comentário não terminado e usa a linha inicial correta.

## 16.3. `tests/invalid_lex_03_string.g`

A string atravessa uma quebra de linha.

Valida a regra de que string não pode ocupar mais de uma linha.

---

# 17. Arquivos de erro sintático

## 17.1. `tests/invalid_syn_01_missing_semicolon.g`

Falta `;` depois de `x = 1`.

Valida se o parser acusa erro sintático corretamente.

## 17.2. `tests/invalid_syn_02_missing_fimse.g`

Abre um `se ... entao` mas não fecha com `fimse`.

Valida uma estrutura de controle incompleta.

---

# 18. Arquivos de erro semântico

## 18.1. `tests/invalid_sem_01_undeclared.g`

Usa `y = 1;` sem declarar `y`.

Valida lookup de identificador.

## 18.2. `tests/invalid_sem_02_redecl_same_scope.g`

Declara `x` duas vezes no mesmo bloco, com tipos diferentes.

Valida redeclaração no mesmo escopo.

## 18.3. `tests/invalid_sem_03_assign_type.g`

Tenta fazer `x = c;` onde `x` é `int` e `c` é `car`.

Valida compatibilidade de tipos em atribuição.

## 18.4. `tests/invalid_sem_04_arith_requires_int.g`

Faz `c + c` com `car`.

Valida a regra de que operadores aritméticos exigem `int`.

## 18.5. `tests/invalid_sem_05_relational_mismatch.g`

Compara `x == c`, misturando `int` e `car`.

Valida a regra de operandos do mesmo tipo em operador relacional.

## 18.6. `tests/invalid_sem_06_logic_requires_int.g`

Faz `c & c` com `car`.

Valida a regra de que operadores lógicos exigem `int`.

---

# 19. Arquivos de AST

## 19.1. `tests/ast_01_assign.g`

Programa pequeno com atribuição e escrita.

### O que ele valida

- criação correta de `AST_ASSIGN`
- criação correta de `AST_WRITE`
- estrutura geral de bloco com declarações e comandos

## 19.2. `tests/ast_02_control.g`

Programa com `while` e `if/else` encadeados.

### O que ele valida

- nós `AST_WHILE`
- nós `AST_IF`
- operadores relacionais e aritméticos
- aninhamento de estruturas de controle

---

# 20. Arquivos de tabela de símbolos

## 20.1. `tests/symtab_01_nested.g`

Possui `x` no bloco externo e `y` em bloco interno.

### O que valida

- criação de dois escopos distintos
- impressão correta do nível e linha da declaração

## 20.2. `tests/symtab_02_empty_scope.g`

Bloco mínimo sem declarações.

### O que valida

- criação de escopo vazio
- impressão de `<vazio>`

---

# 21. Arquivos de geração de código

## 21.1. `tests/codegen_01_assign_write.g`

Valida um fluxo simples:

- declarar variável
- atribuir inteiro
- escrever inteiro
- imprimir nova linha

## 21.2. `tests/codegen_02_if_else.g`

Valida:

- strings na `.data`
- comparação relacional
- labels de `if/else`
- syscalls de escrita de string

## 21.3. `tests/codegen_03_nested_scope.g`

Valida:

- bloco interno com variável `y`
- expressão usando variável externa `x`
- offset de variável interna
- desalocação correta ao sair do bloco

Esse teste é especialmente importante porque pega erros de gerenciamento de pilha em escopos aninhados.

---

# 22. Arquivos `tests/expected/*.out`

Esses arquivos guardam a saída esperada textual do compilador em vários modos. Eles são a “resposta correta” usada pelo `Makefile`.

## 22.1. `invalid_lex_01_caractere.out`
Saída esperada para caractere inválido.

## 22.2. `invalid_lex_02_comment.out`
Saída esperada para comentário não terminado.

## 22.3. `invalid_lex_03_string.out`
Saída esperada para string em múltiplas linhas.

## 22.4. `invalid_syn_01_missing_semicolon.out`
Saída esperada de erro sintático por ausência de `;`.

## 22.5. `invalid_syn_02_missing_fimse.out`
Saída esperada de erro sintático por ausência de `fimse`.

## 22.6. `invalid_sem_01_undeclared.out`
Saída esperada para variável não declarada.

## 22.7. `invalid_sem_02_redecl_same_scope.out`
Saída esperada para redeclaração no mesmo escopo.

## 22.8. `invalid_sem_03_assign_type.out`
Saída esperada para atribuição com tipos incompatíveis.

## 22.9. `invalid_sem_04_arith_requires_int.out`
Saída esperada para operação aritmética com `car`.

## 22.10. `invalid_sem_05_relational_mismatch.out`
Saída esperada para comparação entre tipos diferentes.

## 22.11. `invalid_sem_06_logic_requires_int.out`
Saída esperada para operação lógica com tipo inválido.

## 22.12. `ast_01_assign.out`
Representação textual esperada da AST do primeiro teste de árvore.

## 22.13. `ast_02_control.out`
Representação textual esperada da AST do teste com controle de fluxo.

## 22.14. `symtab_01_nested.out`
Dump esperado dos escopos do programa com blocos aninhados.

## 22.15. `symtab_02_empty_scope.out`
Dump esperado do escopo vazio.

---

# 23. Arquivos `tests/expected/*.s`

Esses arquivos guardam o assembly MIPS esperado.

## 23.1. `codegen_01_assign_write.s`

Mostra um caso simples com:

- frame pointer inicial
- alocação de `x`
- atribuição literal
- leitura da variável
- syscall de escrita de inteiro
- syscall de newline
- desalocação do bloco
- encerramento do programa

## 23.2. `codegen_02_if_else.s`

Mostra:

- seção `.data` com duas strings
- comparação `x < 2`
- labels `else_0` e `endif_1`
- syscalls de escrita de string

É excelente para explicar como a AST de controle vira saltos em assembly.

## 23.3. `codegen_03_nested_scope.s`

Mostra:

- duas alocações na pilha, uma para `x` e outra para `y`
- uso de offsets `-4($fp)` e `-8($fp)`
- cálculo `x + 2`
- escrita de `y`
- duas desalocações ao final dos escopos

Esse é o melhor teste para explicar escopos na geração de código.

---

# 24. Arquivo `tests/symtab_api_test.c`

## 24.1. Função do arquivo

Testa diretamente o módulo de tabela de símbolos sem depender de parser, lexer ou arquivos `.g`.

## 24.2. Fluxo do teste

1. inicializa a pilha;
2. verifica se começou vazia;
3. empilha escopo;
4. insere `x:int`;
5. tenta redeclarar `x` no mesmo escopo e espera falha;
6. empilha novo escopo;
7. insere `x:car` e espera sucesso, validando shadowing;
8. faz `lookup` e verifica que o `x` visível é o do topo;
9. dá `pop` e verifica que o `x` visível volta a ser o externo.

## 24.3. O que ele prova

- `symtab_init` funciona
- `symtab_insert` respeita escopo atual
- `symtab_lookup` respeita shadowing
- `symtab_pop_scope` restaura o identificador anterior

---

# 25. Arquivo `tests/semantic_api_test.c`

## 25.1. Função do arquivo

Testa a análise semântica diretamente construindo ASTs à mão.

## 25.2. `build_valid`

Cria uma AST válida com:

- `x:int`
- `c:car`
- atribuição a `x`
- bloco interno com `x:car`
- atribuição válida dentro do bloco interno
- `if (x)` com comando vazio

Esse caso valida escopo, shadowing e condição simples.

## 25.3. `build_invalid_undeclared`

Cria uma AST onde `x` é usado sem declaração.

Serve para validar o erro de identificador ausente.

## 25.4. `build_invalid_types`

Cria uma AST com `x:int` e `c:car`, seguida de `x = c`.

Valida erro de tipos incompatíveis.

## 25.5. `main`

Executa os três cenários:

- o válido deve passar;
- os dois inválidos devem falhar.

---

# 26. Arquivo `tests/codegen_api_test.c`

## 26.1. Função do arquivo

Testa geração de código sem depender do parser. Monta uma AST à mão, roda a semântica e emite assembly para um arquivo temporário.

## 26.2. Estrutura do programa gerado no teste

- declara `x:int`
- declara `c:car`
- faz `x = 5`
- faz `c = 'A'`
- escreve `x`
- imprime newline
- faz um `if (x < 10) escreva "ok\n"`

## 26.3. Por que ele é útil

Esse teste prova que:

- a AST construída manualmente é aceita;
- a análise semântica anota corretamente os tipos;
- a geração de código consegue lidar com inteiros, caracteres, strings e `if`.

## 26.4. Uso de `tmpfile`

O teste não precisa salvar um `.s` permanente. Basta garantir que a emissão aconteceu sem falhar.

---

# 27. Como explicar o fluxo inteiro em apresentação

Uma boa sequência oral é esta:

## 27.1. Etapa 1 — Léxico

“Primeiro o Flex lê os caracteres do arquivo e transforma em tokens. Ele reconhece palavras reservadas, identificadores, inteiros, caracteres, strings e operadores. Também detecta erros léxicos.”

## 27.2. Etapa 2 — Sintático

“O Bison recebe esses tokens e verifica se seguem a gramática. Enquanto reconhece a estrutura do programa, ele monta a AST.”

## 27.3. Etapa 3 — AST

“A AST é a representação intermediária do programa. Ela guarda a estrutura lógica do código e também as linhas e lexemas necessários para erro e depuração.”

## 27.4. Etapa 4 — Tabela de símbolos

“A tabela de símbolos é uma pilha de escopos. Cada bloco empilha um escopo novo, e quando o bloco termina esse escopo é removido.”

## 27.5. Etapa 5 — Semântica

“A análise semântica percorre a AST da esquerda para a direita, criando e removendo escopos conforme entra e sai de blocos. Ela valida declaração, uso e compatibilidade de tipos.”

## 27.6. Etapa 6 — Geração de código

“Depois de passar pela semântica, a AST é percorrida de novo para gerar assembly MIPS. Variáveis viram posições na pilha, expressões viram instruções e estruturas de controle viram labels e saltos.”

## 27.7. Etapa 7 — Testes

“Os testes cobrem programas válidos, erros léxicos, sintáticos e semânticos, impressão de AST, tabela de símbolos, geração de código e testes unitários em C.”

---

# 28. Pontos fortes do projeto para destacar

1. **Separação por fases**: cada fase do compilador está em um módulo distinto.
2. **AST limpa**: a árvore é compacta e reutilizável.
3. **Tabela de símbolos por pilha de escopos**: combina bem com blocos aninhados.
4. **Semântica independente do parser**: dá para testar semanticamente ASTs construídas à mão.
5. **Codegen desacoplado**: também pode ser testado a partir de AST pronta.
6. **Suíte de testes bem segmentada**: facilita depuração e demonstra robustez.

---

# 29. Pontos que o professor pode perguntar

## 29.1. “Por que usar `next` na AST?”

Porque várias partes da árvore representam listas, como comandos e declarações. Em vez de criar um nó “lista”, foi mais simples encadear nós homogêneos.

## 29.2. “Por que guardar `line` em cada nó?”

Para que os erros semânticos possam apontar a linha correta, mesmo depois que o parser já terminou.

## 29.3. “Como o shadowing funciona?”

`lookup` começa do topo da pilha. Então o identificador do escopo mais interno é encontrado antes do externo.

## 29.4. “Por que o parser já cria a AST?”

Porque isso elimina uma fase extra de reconstrução da árvore. O próprio reconhecimento sintático já monta a representação intermediária.

## 29.5. “Como o codegen sabe o tipo do que está imprimindo?”

A fase semântica anota `data_type` nos nós da AST. Depois o `codegen` consulta esse campo para escolher a syscall correta.

## 29.6. “Como variáveis locais são alocadas?”

Cada declaração reserva 4 bytes na pilha. O módulo de codegen associa um `offset` negativo relativo ao `$fp`.

## 29.7. “Como `if` e `while` são gerados?”

Com labels únicos e instruções de desvio (`beq`, `j`).

---

# 30. Resumo final

Se você quiser resumir o projeto em poucas frases na apresentação, pode usar algo nesta linha:

> “O compilador recebe um programa G-V1, faz análise léxica com Flex, análise sintática com Bison, constrói uma AST durante o parsing, percorre essa AST para fazer análise semântica com apoio de uma pilha de tabelas de símbolos e, se o programa for válido, percorre a AST novamente para gerar código MIPS. O projeto também tem uma suíte de testes por fase, cobrindo léxico, sintaxe, semântica, AST, escopos e geração de código.”

