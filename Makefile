EXTENSION = .exe
CC = gcc

.PHONY: tests

tests: $(patsubst src/%.c,tests/test_%$(EXTENSION),$(wildcard src/*.c))

tests/test_%$(EXTENSION): src/%.c tests/test_%.c include/gpc/%.h
	$(CC) -ggdb3 -DTESTS -Wall -Wextra -Werror $(patsubst src/%.c,tests/test_%.c,$<) src/assert.c -o $@
	./$@
	
clean:
