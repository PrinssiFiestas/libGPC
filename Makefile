# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -Wno-missing-field-initializers -Wno-comment -Wno-missing-braces
CFLAGS += -Iinclude
CFLAGS += -D_GNU_SOURCE # memmem(), stat64()
ifeq ($(CC), gcc)
CFLAGS += -lm -lpthread
endif
DEBUG_CFLAGS   = -ggdb3 -gdwarf
RELEASE_CFLAGS = -O3 -flto -DNDEBUG

NPROC        = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS   += -j$(THREAD_COUNT)

ifeq ($(OS), Windows_NT)
	EXE_EXT = .exe
	ifeq ($(CC), clang) # get rid of -flto problems on some platforms
		RELEASE_CFLAGS = -O3 -DNDEBUG
	endif
else
	EXE_EXT =
	DEBUG_CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
	ifeq ($(CC), gcc)
		DEBUG_CFLAGS += -static-libasan
	else # clang
		DEBUG_CFLAGS += -static-libsan
	endif
endif

SRCS       = $(wildcard src/*.c)
OBJS       = $(patsubst src/%.c, build/%.o,  $(wildcard src/*.c))
DEBUG_OBJS = $(patsubst src/%.c, build/%d.o, $(wildcard src/*.c))

TESTS         = $(patsubst tests/test_%.c, build/test_%d$(EXE_EXT), $(wildcard tests/test_*.c))
RELEASE_TESTS = $(patsubst tests/test_%.c, build/test_%$(EXE_EXT),  $(wildcard tests/test_*.c))

.PHONY: all release debug install tests build_tests run_tests release_tests
.PHONY: build_release_tests run_release_tests single_header analyze clean

.PRECIOUS: $(TESTS)

all: release debug build/gprun$(EXE_EXT) single_header

single_header: build/singleheadergen$(EXE_EXT)
	./$<

build/gprun$(EXE_EXT): tools/gprun.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $? -o $@

/etc/gdb/gpstring.py:
	cp tools/gpstring.py /etc/gdb/
	$(file >> /etc/gdb/gdbinit,source /etc/gdb/gpstring.py)
	$(file >> /etc/gdb/gdbinit,define gpdebug)
	$(file >> /etc/gdb/gdbinit,  shell cc -Wall $$arg0 -ggdb3 -fsanitize=address -fsanitize=undefined -lgpcd -lm -lpthread)
	$(file >> /etc/gdb/gdbinit,  file a.out)
	$(file >> /etc/gdb/gdbinit,  start)
	$(file >> /etc/gdb/gdbinit,end)

install: all /etc/gdb/gpstring.py
install:
	cp -r include/gpc  /usr/local/include/
	cp gpc.h           /usr/local/include/gpc/
	cp build/libgpc.a  /usr/local/lib/
	cp build/libgpcd.a /usr/local/lib/
	cp build/gprun     /usr/local/bin/

build/singleheadergen$(EXE_EXT): tools/singleheadergen.c build/libgpc.a
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $^ -o $@

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
	make single_header

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
	make single_header

clean:
	rm -rf build

