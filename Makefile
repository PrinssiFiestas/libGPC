EXTENSION = .exe
CFLAGS = -Wall -Wextra -Werror -Wpedantic
CC = gcc

.PHONY: tests

tests: $(patsubst src/%.c,tests/test_%$(EXTENSION),$(wildcard src/*.c))

tests/test_%$(EXTENSION): src/%.c tests/test_%.c include/gpc/%.h
	$(CC) -ggdb3 -DTESTS $(CFLAGS) $(patsubst src/%.c,tests/test_%.c,$<) src/assert.c -o $@
	./$@
	
clean:
	rm tests/*.exe
