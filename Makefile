# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -Wno-missing-field-initializers -Wno-comment
CFLAGS += -Iinclude -lm

NPROC = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS += -j$(THREAD_COUNT)

ifeq ($(OS), Windows_NT)
	EXE_EXT = .exe
else
	EXE_EXT =
endif

SRCS       = $(wildcard src/*.c)
OBJS       = $(patsubst src/%.c,build/%.o, $(wildcard src/*.c))
DEBUG_OBJS = $(patsubst src/%.c,build/%d.o,$(wildcard src/*.c))

TESTS = $(patsubst tests/test_%.c,build/test_%$(EXE_EXT),$(wildcard tests/test_*.c))

.PHONY: all release debug tests build_tests run_tests analyze clean

.PRECIOUS: $(TESTS)

all: release

release: CFLAGS += -O3 -DNDEBUG
release: build/libgpc.a

debug: CFLAGS += -ggdb3
debug: CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
debug: build/libgpcd.a

analyze: CFLAGS += -fanalyzer
analyze: build_tests

build/libgpc.a: $(OBJS)
	ar -rcs $@ $^

build/libgpcd.a: $(DEBUG_OBJS)
	ar -rcs $@ $^

$(OBJS): build/%.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $@

$(DEBUG_OBJS): build/%d.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $@

-include $(OBJS:.o=.d)
-include $(DEBUG_OBJS:.o=.d)

build_tests: CFLAGS += -DGP_TESTS -ggdb3
build_tests: CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
build_tests: $(TESTS)
$(TESTS): build/test_%$(EXE_EXT) : tests/test_%.c $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%d.o,$<)),$(DEBUG_OBJS)) -o $@

run_tests:
	for test in $(TESTS) ; do \
		./$$test || exit 1 ; \
		echo ; \
	done

tests:
	make build_tests
	make run_tests

clean:
	rm -rf build

