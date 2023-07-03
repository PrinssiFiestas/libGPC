EXTENSION = .exe
CFLAGS = -Wall -Wextra -Werror -Wpedantic
CC = gcc

.PHONY: tests

tests: $(patsubst src/%.c,tests/build/test_%$(EXTENSION),$(wildcard src/*.c))

tests/build/test_%$(EXTENSION): src/%.c tests/test_%.c include/gpc/%.h
	mkdir -p tests/build
	$(CC) -ggdb3 -DTESTS $(CFLAGS) $(patsubst src/%.c,tests/test_%.c,$<) src/assert.c -o $@
	./$@
	
clean:
	rm -rf tests/build build
