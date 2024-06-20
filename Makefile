# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -Wno-comment
CFLAGS += -Iinclude
CFLAGS += -D_GNU_SOURCE # memmem(), stat64()
CFLAGS += -lm -lpthread

DEBUG_CFLAGS   = -ggdb3 -gdwarf
RELEASE_CFLAGS = -O3 -DNDEBUG
ifeq ($(CC), clang) # in some systems Clang ignores -lm and crashes with -flto
	CFLAGS += -Wno-unused-command-line-argument
else
	RELEASE_CFLAGS += -flto
endif

NPROC        = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS   += -j$(THREAD_COUNT)

ifeq ($(OS), Windows_NT)
	EXE_EXT = .exe
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

TEST_SRCS     = $(wildcard tests/test_*.c)
TESTS         = $(patsubst tests/test_%.c, build/test_%d$(EXE_EXT), $(TEST_SRCS))
RELEASE_TESTS = $(patsubst tests/test_%.c, build/test_%$(EXE_EXT),  $(TEST_SRCS))

.PHONY: all release debug install tests build_tests run_tests release_tests
.PHONY: build_release_tests run_release_tests cl_tests single_header analyze clean

.PRECIOUS: $(TESTS) $(RELEASE_TESTS)

# MSVC can be used to run tests if you can get cl.exe to run in MSYS2 shell.
# Here's how to do it:
# https://anadoxin.org/blog/bringing-visual-studio-compiler-into-msys2-environment.html/

CL_OBJS   = $(OBJS:.o=.obj)
CL_CFLAGS = -std:c17 -experimental:c11atomics -Iinclude -utf-8
CL_TESTS  = $(TESTS:d$(EXE_EXT)=cl.exe)
$(CL_OBJS): $(wildcard src/*.h)
$(CL_OBJS): $(wildcard include/gpc/*.h)
$(CL_OBJS): $(wildcard include/printf/*.h)
$(CL_OBJS): build/%.obj : src/%.c
	@mkdir -p build
	cl.exe $< -c $(CL_CFLAGS) -Fo"$@"

$(CL_TESTS): build/test_%cl.exe : tests/test_%.c $(CL_OBJS)
	cl.exe $< $(CL_CFLAGS) $(filter-out build/$(notdir $(patsubst tests/test_%.c,%.obj,$<)),$(CL_OBJS)) -Fe"$@" -Fo"build/"

run_cl_tests: $(CL_TESTS)
	for test in $(CL_TESTS) ; do \
		./$$test || exit 1 ; \
		echo ; \
	done

cl_tests: $(CL_OBJS) $(CL_TESTS) run_cl_tests

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
	cp -r include/gpc   /usr/local/include/
	cp build/gpc.h      /usr/local/include/gpc/
	cp build/gprun      /usr/local/bin/
	cp build/libgpc.so  /usr/local/lib/
	cp build/libgpcd.so /usr/local/lib/
	chmod 0755          /usr/local/lib/libgpc.so
	chmod 0755          /usr/local/lib/libgpcd.so
	ldconfig

build/singleheadergen$(EXE_EXT): tools/singleheadergen.c | build/libgpc.so
	$(CC) $? build/libgpc.so $(CFLAGS) $(RELEASE_CFLAGS) -o $@

release: CFLAGS += $(RELEASE_CFLAGS)
release: build/libgpc.so
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: build/libgpcd.so

analyze: CFLAGS += -fanalyzer
analyze: build_tests

build/libgpc.so: $(OBJS)
	$(CC) -shared -o $@ $^

build/libgpcd.so: $(DEBUG_OBJS)
	$(CC) -shared -o $@ $^

$(OBJS): build/%.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c -fpic $(CFLAGS) $< -o $@

$(DEBUG_OBJS): build/%d.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c -fpic $(CFLAGS) $< -o $@

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

