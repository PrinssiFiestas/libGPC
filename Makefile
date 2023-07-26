CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -Wconversion

.PHONY: tests

tests:
	mkdir -p build
	$(CC) -ggdb3 $(CFLAGS) tests/test_assert.c -o build/test_assert.exe
	./build/test_assert.exe
