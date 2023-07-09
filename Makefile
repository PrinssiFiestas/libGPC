# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic

OBJS = $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))
SRCS = $(wildcard src/*.c)

TESTS = $(patsubst src/%.c,tests/build/test_%.exe,$(wildcard src/*.c))

.PHONY: tests all release debug

all: release

release: CFLAGS += -O2 -DNDEBUG
release: build/libgpc.a

debug: CFLAGS += -ggdb3
debug: build/libgpc.a

build/libgpc.a: $(OBJS)
	ar -rcs $@ $^

$(OBJS): build/%.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $@

-include $(OBJS:.o=.d)

tests: $(TESTS)
$(TESTS): $(SRCS)
	mkdir -p tests/build
	$(CC) -ggdb3 -DTESTS $(CFLAGS) $(patsubst src/%.c,tests/test_%.c,$<) src/assert.c -o $@
	./$@

clean:
	rm -rf tests/build build

