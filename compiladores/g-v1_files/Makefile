CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
BISON   = bison
FLEX    = flex
TARGET  = g-v1

SRCS    = g-v1.tab.c lex.yy.c ast.c tabela.c

all: $(TARGET)

$(TARGET): $(SRCS) ast.h tabela.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# Compila com saída de depuração da AST (-DDEBUG)
debug: CFLAGS += -DDEBUG
debug: $(TARGET)

g-v1.tab.c g-v1.tab.h: g-v1.y
	$(BISON) -d -o g-v1.tab.c g-v1.y

lex.yy.c: g-v1.l g-v1.tab.h
	$(FLEX) -o lex.yy.c g-v1.l

clean:
	rm -f $(TARGET) g-v1.tab.c g-v1.tab.h lex.yy.c

.PHONY: all debug clean test

test:
	-./g-v1 teste.g
	-./g-v1 teste2.g
	-./g-v1 teste3.g
	-./g-v1 teste4.g
	-./g-v1 teste5.g
	-./g-v1 teste6.g
	-./g-v1 teste7.g
	-./g-v1 teste8.g
	-./g-v1 teste9.g
	-./g-v1 teste10.g
