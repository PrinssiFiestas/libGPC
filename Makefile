# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

# -----------------------------------------------------------------------------
#
#          PUBLIC TARGETS
#
# -----------------------------------------------------------------------------

.DEFAULT_GOAL := all

# Build release and debug static libraries and shared libraries if Linux. Also
# generate single header library.
.PHONY: all

# Install libGPC, GDB pretty printer for GPString, gprun, and single header
# library.
.PHONY: install # TODO

# Build optimized static library.
.PHONY: release

# Build debug static library.
.PHONY: debug

# Build optimized shared library. Linux only.
.PHONY: shared_release

# Build debug shared library. Linux only.
.PHONY: shared_debug

# Build gprun # TODO rewrite
.PHONY: gprun

# Build documentation. Requires Doxygen.
.PHONY: docs

# Remove build artifacts.
.PHONY: clean

# -----------------------------------------------------------------------------
#
#          END OF PUBLIC TARGETS
#
# -----------------------------------------------------------------------------
# Common Variables

GPC_VERSION = 0.3.0-dev

SRCS      = $(wildcard src/*.c)
TEST_SRCS = $(wildcard tests/test_*.c)

RELEASE_OBJS        = $(patsubst src/%.c, build/junk/%.o,        $(SRCS))
DEBUG_OBJS          = $(patsubst src/%.c, build/junk/%d.o,       $(SRCS))
SHARED_RELEASE_OBJS = $(patsubst src/%.c, build/junk/shared_%.o,  $(SRCS))
SHARED_DEBUG_OBJS   = $(patsubst src/%.c, build/junk/shared_%d.o, $(SRCS))

ifeq ($(OS), Windows_NT)
EXE = .exe
endif

# -----------------------------------------------------------------------------
# Common Variables for GCC and Clang

CC = # will be set below to Clang if MacOS or MSYS2 CLANG64, GCC otherwise.

CFLAGS = -Wall -Wextra -Wswitch-enum -Wundef -Iinclude -D_GNU_SOURCE
LFLAGS = -lm # only relevant for test executables

DEBUG_CFLAGS   = -ggdb3 -gdwarf
RELEASE_CFLAGS = -O3 -DNDEBUG
TEST_CFLAGS    = -DGP_TESTS=1 -Werror -pthread -no-pie # -no-pie prevents some sanitizer crashes

SO_CFLAGS = -shared -fPIC -fno-semantic-interposition -Wl,-Bsymbolic-functions

# Multithreaded build by default
NPROC        = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS   += -j$(THREAD_COUNT)

# -----------------------------------------------------------------------------
# Platform Specific Variables for GCC and Clang

MSYS_VERSION = $(if $(findstring Msys, $(shell uname -o)),$(word 1, $(subst ., ,$(shell uname -r))),0)

ifeq ($(MSYS_VERSION), 0)
MSYS_ENVIRONMENT =
else
MSYS_ENVIRONMENT = $(patsubst /%/bin/gcc,%,$(shell which gcc))
endif

UNAME_S = $(shell uname -s)

ifeq ($(MSYS_ENVIRONMENT),clang64)
CC = clang
else ifeq ($(UNAME_S),Darwin)
CC = clang
else
CC = gcc
endif

ifeq ($(CC), gcc) # only gcc has portable link-time optimizations.
RELEASE_CFLAGS += -flto=auto
endif
# TODO I can't remember exactly where -flto=auto with Clang breaks, but we
# probably should try it on x86_64 Linux and enable if works.

ifeq ($(MSYS_ENVIRONMENT),clang64)
	SANITIZERS = -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all
else ifneq ($(OS), Windows_NT)
	SANITIZERS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
	# Link asan statically for easier distribution. Also trap for better
	# debugging experience.
	ifeq ($(CC), gcc)
		SANITIZERS = -static-libasan -fno-sanitize-recover=all
	else ifeq($(CC) clang)
		SANITIZERS = -static-libsan -fsanitize-trap=all
	endif
endif

# -----------------------------------------------------------------------------
# Rules for Public Targets

all: release debug
ifeq ($(UNAME_S),Linux)
all: shared_release shared_debug
endif

-include $(RELEASE_OBJS:.o=.d)
-include $(DEBUG_OBJS:.o=.d)
-include $(SHARED_RELEASE_OBJS:.o=.d)
-include $(SHARED_DEBUG_OBJS:.o=.d)

$(RELEASE_OBJS): build/junk/%.o : src/%.c
	@mkdir -p build/junk
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $@

$(DEBUG_OBJS): build/junk/%d.o : src/%.c
	@mkdir -p build/junk
	$(CC) -MMD -MP -c $(CFLAGS) $< -o $o

$(SHARED_RELEASE_OBJS): build/junk/shared_%.o : src/%.c
	@mkdir -p build/junk
	$(CC) -MMD -MP -c $(CFLAGS) $(SO_CFLAGS)  $< -o $@

$(SHARED_DEBUG_OBJS): build/junk/shared_%d.o : src/%.c
	@mkdir -p build/junk
	$(CC) -MMD -MP -c $(CFLAGS) $(SO_CFLAGS) $< -o $o

release: build/libgpc.a
build/libgpc.a: $(RELEASE_OBJS)
	ar -crs $@ $^

debug: build/libgpcd.a
build/libgpcd.a: $(DEBUG_OBJS)
	ar -crs $@ $^

shared_release: build/libgpc.so
build/libgpc.so: $(SHARED_RELEASE_OBJS)
	$(CC) -o $@ $(SO_CFLAGS) $^

shared_debug: build/libgpcd.so
build/libgpcd.so: $(SHARED_DEBUG_OBJS)
	$(CC) -o $@ $(SO_CFLAGS) $^

gprun: build/gprun$(EXE)
build/gprun$(EXE): tools/gprun.c
	@mkdir -p build
	$(CC) -o $@ $(CFLAGS) $(DEBUG_CFLAGS) $^

docs:
	@mkdir -p build/docs
	@doxygen

clean:
	rm -rf build

# TODO we don't use -Wconversion, but many people do, so we need to add
# pragmas to suppress it in single header implementation and have a test target
# that includes single header without defining GPC_IMPLEMENTATION to enforce
# always converting in our inline functions. And do the same for C++ just in
# case.
