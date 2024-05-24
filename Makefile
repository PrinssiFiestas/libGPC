# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -Wno-missing-field-initializers -Wno-comment
CFLAGS += -Iinclude -lm
CFLAGS += -D_GNU_SOURCE # memmem(), stat64()
DEBUG_CFLAGS   = -ggdb3
RELEASE_CFLAGS = -O3 -flto -DNDEBUG
ifneq (&(OS), Windows_NT)
DEBUG_CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
endif

NPROC = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS += -j$(THREAD_COUNT)

ifeq ($(OS), Windows_NT)
	EXE_EXT = .exe
else
	EXE_EXT =
endif

SRCS       = $(wildcard src/*.c)
OBJS       = $(patsubst src/%.c, build/%.o,  $(wildcard src/*.c))
DEBUG_OBJS = $(patsubst src/%.c, build/%d.o, $(wildcard src/*.c))

TESTS         = $(patsubst tests/test_%.c, build/test_%d$(EXE_EXT), $(wildcard tests/test_*.c))
RELEASE_TESTS = $(patsubst tests/test_%.c, build/test_%$(EXE_EXT),  $(wildcard tests/test_*.c))

.PHONY: all release debug tests build_tests run_tests analyze clean

.PRECIOUS: $(TESTS)

all: release

release: CFLAGS += $(RELEASE_CFLAGS)
release: build/libgpc.a

debug: CFLAGS += $(DEBUG_CFLAGS)
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

build_tests: CFLAGS += -DGP_TESTS $(DEBUG_CFLAGS)
build_tests: $(TESTS)

$(TESTS): build/test_%d$(EXE_EXT) : tests/test_%.c $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%d.o,$<)),$(DEBUG_OBJS)) -o $@

run_tests:
	for test in $(TESTS) ; do \
		./$$test || exit 1 ; \
		echo ; \
	done

tests:
	make build_tests
	make run_tests

build_release_tests: CFLAGS += -DGP_TESTS $(RELEASE_CFLAGS)
build_release_tests: $(RELEASE_TESTS)

$(RELEASE_TESTS): build/test_%$(EXE_EXT) : tests/test_%.c $(OBJS)
	$(CC) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%.o,$<)),$(OBJS)) -o $@ $(CFLAGS)

run_release_tests:
	for test in $(RELEASE_TESTS) ; do \
		./$$test || exit 1 ; \
		echo ; \
	done

release_tests:
	make build_release_tests
	make run_release_tests

clean:
	rm -rf build

