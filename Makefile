EXTENSION = .exe
CC = gcc

.Phony: tests

tests: $(patsubst src/%.c,tests/test_%$(EXTENSION),$(wildcard src/*.c))

tests/test_%$(EXTENSION): src/%.c tests/test_%.c
	$(CC) -ggdb3 -DDEBUG -Wall -Wextra $(patsubst src/%.c,tests/test_%.c,$<) src/assert.c $(patsubst src/assert.c,,$<) -o $@
	./$@
	rm $@
