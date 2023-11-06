# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC = gcc
CFLAGS = -Wall -Wextra -Wconversion -Werror

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))

TESTS = $(patsubst tests/test_%.c,build/test_%.exe,$(wildcard tests/test_*.c))

.PHONY: tests all release debug testallc
.PRECIOUS: $(TESTS)

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

ifneq ($(patsubst msbuild%,msbuild,$(CC)),msbuild)
tests: CFLAGS += -DTESTS -ggdb3
tests: $(TESTS)
$(TESTS): build/test_%.exe : tests/test_%.c $(OBJS)
	$(CC) $(CFLAGS) $< $(filter-out build/$(notdir $(patsubst tests/test_%.c,%.o,$<)),$(OBJS)) -o $@
	./$@
else
define NEWLINE


endef
tests:
	msbuild.exe libGPC.sln
	$(foreach test,$(TESTS),./$(test)$(NEWLINE))
endif

testallc:
	make clean
	make tests CC=gcc.exe
	make clean
	make tests CC=clang.exe
	make clean
	wsl make tests CC=gcc
	make clean
	wsl make tests CC=clang
	make tests CC=msbuild.exe
	make clean

clean:
	rm -rf build

