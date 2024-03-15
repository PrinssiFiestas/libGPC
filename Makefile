# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -Wdouble-promotion
CFLAGS += -Wno-missing-field-initializers -Wno-comment
CFLAGS += -Iinclude

ifeq ($(OS), Windows_NT)
	EXE_EXT = .exe
else
	EXE_EXT =
endif

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))

TESTS = $(patsubst tests/test_%.c,build/test_%$(EXE_EXT),$(wildcard tests/test_*.c))

.PHONY: tests all release debug

.PRECIOUS: $(TESTS)

all: release

release: CFLAGS += -O3
release: build/libgpc.a

debug: CFLAGS += -ggdb3 -DGP_DEBUG -Og
debug: build/libgpcd.a

build/libgpc.a: $(OBJS)
	ar -rcs $@ $^

$(OBJS): build/%.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $@

-include $(OBJS:.o=.d)

tests: CFLAGS += -DTESTS -ggdb3
tests: $(TESTS)
$(TESTS): build/test_%$(EXE_EXT) : tests/test_%.c $(OBJS)
	$(CC) $(CFLAGS) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%.o,$<)),$(OBJS)) -o $@
	./$@

clean:
	rm -rf build

