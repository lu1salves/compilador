CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
BISON   = bison
FLEX    = flex
TARGET  = g-v1

all: $(TARGET)

$(TARGET): g-v1.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o $@ g-v1.tab.c lex.yy.c

g-v1.tab.c g-v1.tab.h: g-v1.y
	$(BISON) -d -o g-v1.tab.c g-v1.y

lex.yy.c: g-v1.l g-v1.tab.h
	$(FLEX) -o lex.yy.c g-v1.l

clean:
	rm -f $(TARGET) g-v1.tab.c g-v1.tab.h lex.yy.c

.PHONY: all clean

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
