CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
BISON   = bison
FLEX    = flex
TARGET  = g-v1

SRCS    = g-v1.tab.c lex.yy.c ast.c symtab.c semantic.c codegen.c
OBJS    = g-v1.tab.o lex.yy.o ast.o symtab.o semantic.o codegen.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

g-v1.tab.c g-v1.tab.h: g-v1.y ast.h symtab.h semantic.h codegen.h
	$(BISON) -d -o g-v1.tab.c g-v1.y

lex.yy.c: g-v1.l g-v1.tab.h
	$(FLEX) -o lex.yy.c g-v1.l

g-v1.tab.o: g-v1.tab.c ast.h symtab.h semantic.h codegen.h
	$(CC) $(CFLAGS) -c g-v1.tab.c

lex.yy.o: lex.yy.c g-v1.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symtab.o: symtab.c symtab.h ast.h
	$(CC) $(CFLAGS) -c symtab.c

semantic.o: semantic.c semantic.h symtab.h ast.h
	$(CC) $(CFLAGS) -c semantic.c

codegen.o: codegen.c codegen.h ast.h
	$(CC) $(CFLAGS) -c codegen.c

ast: $(TARGET)
	./$(TARGET) --ast tests/ast_01_assign.g

symtab: $(TARGET)
	./$(TARGET) --symtab tests/symtab_01_nested.g

asm: $(TARGET)
	./$(TARGET) --code tests/codegen_01_assign_write.g

test: test-all

test-all: test-valid test-lex test-syntax test-semantic test-ast test-symtab test-codegen

test-invalid: test-lex test-syntax test-semantic

test-valid: $(TARGET)
	@set -e; \
	for f in tests/validos/example/*.g; do \
		exp="tests/validos/correct/$$(basename $$f .g).s"; \
		printf "[VALID]    %s ... " "$$f"; \
		out=$$(./$(TARGET) --code "$$f"); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-lex: $(TARGET)
	@set -e; \
	for f in tests/lexicos/example/invalid_lex_*.g; do \
		exp="tests/lexicos/correct/$$(basename $$f .g).out"; \
		printf "[LEX]      %s ... " "$$f"; \
		out=$$(./$(TARGET) "$$f" 2>&1 || true); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-syntax: $(TARGET)
	@set -e; \
	for f in tests/sintaticos/example/invalid_syn_*.g; do \
		exp="tests/sintaticos/correct/$$(basename $$f .g).out"; \
		printf "[SINTAXE]  %s ... " "$$f"; \
		out=$$(./$(TARGET) "$$f" 2>&1 || true); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-semantic: $(TARGET)
	@set -e; \
	for f in tests/semanticos/example/invalid_sem_*.g; do \
		exp="tests/semanticos/correct/$$(basename $$f .g).out"; \
		printf "[SEM]      %s ... " "$$f"; \
		out=$$(./$(TARGET) "$$f" 2>&1 || true); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-ast: $(TARGET)
	@set -e; \
	for f in tests/structures/example/ast_*.g; do \
		exp="tests/structures/correct/$$(basename $$f .g).out"; \
		printf "[AST]      %s ... " "$$f"; \
		out=$$(./$(TARGET) --ast "$$f"); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-symtab: $(TARGET)
	@set -e; \
	for f in tests/structures/example/symtab_*.g; do \
		exp="tests/structures/correct/$$(basename $$f .g).out"; \
		printf "[SYMTAB]   %s ... " "$$f"; \
		out=$$(./$(TARGET) --symtab "$$f"); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

test-codegen: $(TARGET)
	@set -e; \
	for f in tests/structures/example/codegen_*.g; do \
		exp="tests/structures/correct/$$(basename $$f .g).s"; \
		printf "[CODEGEN]  %s ... " "$$f"; \
		out=$$(./$(TARGET) --code "$$f"); \
		expected=$$(cat "$$exp"); \
		if [ "$$out" != "$$expected" ]; then \
			echo "FAIL"; \
			echo "Esperado:"; \
			printf "%s\n" "$$expected"; \
			echo "Obtido:"; \
			printf "%s\n" "$$out"; \
			exit 1; \
		fi; \
		echo "OK"; \
	done

clean:
	rm -f $(TARGET) $(OBJS) g-v1.tab.c g-v1.tab.h lex.yy.c
