# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -Wno-comment -Wno-gnu-zero-variadic-macro-arguments

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))

TESTS = $(patsubst tests/test_%.c,build/test_%.exe,$(wildcard tests/test_*.c))

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


$(TESTS): build/test_%.exe : tests/test_%.c $(OBJS)
	$(CC) -ggdb3 -DTESTS $(CFLAGS) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%.o,$<)),$(OBJS)) -o $@
	./$@

clean:
	rm -rf build

