# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

GPC_VERSION = 0.2.2

override CFLAGS += -Wall -Wextra -Werror -Wpedantic
override CFLAGS += -DGP_PEDANTIC
override CFLAGS += -D_GNU_SOURCE # memmem(), stat64(), locale_t
override CFLAGS += -Iinclude
override CFLAGS += -lm -lpthread

DEBUG_CFLAGS   = -ggdb3 -gdwarf
RELEASE_CFLAGS = -O3 -DNDEBUG -fno-math-errno

MSYS_VERSION = $(if $(findstring Msys, $(shell uname -o)),$(word 1, $(subst ., ,$(shell uname -r))),0)

ifeq ($(MSYS_VERSION), 0)
MSYS_ENVIRONMENT =
else
MSYS_ENVIRONMENT = $(patsubst /%/bin/gcc,%,$(shell which gcc))
endif

UNAME_S = $(shell uname -s)

ifeq ($(MSYS_ENVIRONMENT),clang64)
CC = clang
DEBUG_CFLAGS += -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all
else ifeq ($(UNAME_S),Darwin)
CC = clang
else
CC = gcc
endif

ifeq ($(CC), clang) # in some systems Clang ignores -lm
override CFLAGS += -Wno-unused-command-line-argument
else ifeq ($(CC), gcc) # faster multithreaded incremental release build
RELEASE_CFLAGS += -flto=auto
endif # non gcc uses unity build which is more portable than -flto

NPROC        = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS   += -j$(THREAD_COUNT)

ifeq ($(OS), Windows_NT)
EXE_EXT = .exe
LIB_EXT = .a
else
EXE_EXT =
DEBUG_CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
ifeq ($(CC), gcc)
DEBUG_CFLAGS += -static-libasan -fno-sanitize-recover=all
else ifeq ($(CC), clang)
DEBUG_CFLAGS += -static-libsan -fsanitize-trap=all
endif
LIB_EXT = .so
endif

all: release debug build/gprun$(EXE_EXT) single_header

SRCS       = $(wildcard src/*.c)
OBJS       = $(patsubst src/%.c, build/%.o,  $(wildcard src/*.c))
DEBUG_OBJS = $(patsubst src/%.c, build/%d.o, $(wildcard src/*.c))

TEST_SRCS     = $(wildcard tests/test_*.c)
TESTS         = $(patsubst tests/test_%.c, build/test_%d$(EXE_EXT), $(TEST_SRCS))
RELEASE_TESTS = $(patsubst tests/test_%.c, build/test_%$(EXE_EXT),  $(TEST_SRCS))

.PHONY: all release debug install tests build_tests run_tests release_tests
.PHONY: build_release_tests run_release_tests cl_tests single_header analyze
.PHONY: test_all clean

.PRECIOUS: $(TESTS) $(RELEASE_TESTS)

# MSVC can be used to run tests if you can get cl.exe to run in MSYS2 shell.
# Here's how to do it:
# https://anadoxin.org/blog/bringing-visual-studio-compiler-into-msys2-environment.html/

CL_OBJS   = $(OBJS:.o=.obj)
CL_CFLAGS = -std:c17 -Iinclude -utf-8
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

# Run all tests sequentially to see where breaks. Requires MSYS2 UCRT64, WSL2,
# and MSVC running in MSYS2 shell as explained above. Also requires CompCert on
# Linux.
ifeq ($(OS), Windows_NT)
test_all:
	wsl make test_all
	make clean
	make tests CC=clang
	make release_tests CC=clang
	make clean
	make release_tests
	make cl_tests
	make release_tests
	make analyze
	make tests
	clang -Wall -Wextra -Werror tests/singleheadertest.c -o build/singleheadertest.exe
	./build/singleheadertest.exe
	cl tests/singleheadertest.c $(CL_CFLAGS) -Fo"build/" -Fe"build/singleheadertest.exe"
	./build/singleheadertest.exe
	@echo -e "\e[92m\nPassed ALL tests!\e[0m"
else
test_all:
	make clean
	make tests CC=clang
	ccomp -o build/singleheadertest -Wall -Wno-c11-extensions -Werror -fstruct-passing -lm -lpthread tests/singleheadertest.c
	./build/singleheadertest
	gcc -o build/singleheadertest -Wall -Wextra -Werror -Wpedantic -std=c99 tests/singleheadertest.c -lm -lpthread
	./build/singleheadertest
	clang -o build/singleheadertest -Wall -Wextra -Werror -Wpedantic -std=c99 -isystem build -lm tests/singleheadertest.c
	./build/singleheadertest
	make release_tests CC=clang
	make clean
	make release_tests
	make analyze
	make tests
	@echo "\e[92m\nPassed all tests.\e[0m"
endif

ifeq ($(MSYS_VERSION), 0)
INSTALL_PATH = /usr/local/
GDBINIT_PATH = /etc/gdb/
FULL_GDBINIT_PATH = $(GDBINIT_PATH)
else
INSTALL_PATH = /$(MSYS_ENVIRONMENT)/
GDBINIT_PATH = /$(MSYS_ENVIRONMENT)/etc/
FULL_GDBINIT_PATH = $(shell cygpath -m /)$(MSYS_ENVIRONMENT)/etc/
endif

single_header: build/singleheadergen$(EXE_EXT)
	./$< build/gpc.h $(GPC_VERSION)

build/gprun$(EXE_EXT): tools/gprun.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $? -o $@

$(GDBINIT_PATH)gdbinit:
	mkdir -p $(GDBINIT_PATH)
	touch $(GDBINIT_PATH)gdbinit

$(GDBINIT_PATH)gpstring.py: $(GDBINIT_PATH)gdbinit
	cp tools/gpstring.py $(GDBINIT_PATH)
	$(file >> $(GDBINIT_PATH)gdbinit,source $(FULL_GDBINIT_PATH)gpstring.py)

VISUAL_STUDIO_DIR = C:/Program Files/Microsoft Visual Studio
VISUAL_STUDIO_VERSION = $(lastword $(sort $(shell ls "$(VISUAL_STUDIO_DIR)/")))
VISUAL_STUDIO_EDITION = $(lastword $(shell ls "$(VISUAL_STUDIO_DIR)/$(VISUAL_STUDIO_VERSION)/"))
VISUAL_STUDIO_PATH = $(VISUAL_STUDIO_DIR)/$(VISUAL_STUDIO_VERSION)/$(VISUAL_STUDIO_EDITION)
GPRUN_CL_PATH = $(VISUAL_STUDIO_PATH)/Common7/Tools/

install: all $(GDBINIT_PATH)gpstring.py
ifeq ($(OS), Windows_NT)
install:
	rm -f              $(INSTALL_PATH)lib/libgpc$(LIB_EXT)
	rm -f              $(INSTALL_PATH)lib/libgpcd$(LIB_EXT)
	cp -r include/gpc  $(INSTALL_PATH)include/
	cp build/gpc.h     $(INSTALL_PATH)include/gpc/
	cp build/gprun.exe $(INSTALL_PATH)bin/
	cp build/libgpc$(LIB_EXT)  $(INSTALL_PATH)lib/
	cp build/libgpcd$(LIB_EXT) $(INSTALL_PATH)lib/
	chmod 0755 $(INSTALL_PATH)lib/libgpc$(LIB_EXT)
	chmod 0755 $(INSTALL_PATH)lib/libgpcd$(LIB_EXT)
	$(if $(VISUAL_STUDIO_VERSION),cp build/gprun.exe "$(GPRUN_CL_PATH)")
	$(if $(VISUAL_STUDIO_VERSION),mkdir -p "$(GPRUN_CL_PATH)libgpc")
	$(if $(VISUAL_STUDIO_VERSION),cp -r include/gpc "$(GPRUN_CL_PATH)libgpc")
	$(if $(VISUAL_STUDIO_VERSION),cp build/gpc.h "$(GPRUN_CL_PATH)libgpc/gpc")
	$(if $(VISUAL_STUDIO_VERSION),echo -e "#define GPC_IMPLEMENTATION\n#include \"gpc/gpc.h\"\n" > "$(GPRUN_CL_PATH)libgpc/gpc.c")
	$(if $(VISUAL_STUDIO_VERSION),touch "$(GPRUN_CL_PATH)libgpc/gpc.obj")     # for boostrap
	$(if $(VISUAL_STUDIO_VERSION),chmod 333 "$(GPRUN_CL_PATH)libgpc/gpc.obj") # without admin
	@echo Installation succeeded.
else
install:
	rm -f               $(INSTALL_PATH)lib/libgpc$(LIB_EXT)
	rm -f               $(INSTALL_PATH)lib/libgpcd$(LIB_EXT)
	cp -r include/gpc   $(INSTALL_PATH)include/
	cp build/gpc.h      $(INSTALL_PATH)include/gpc/
	cp build/gprun      $(INSTALL_PATH)bin/
	cp build/libgpc$(LIB_EXT)  $(INSTALL_PATH)lib/
	cp build/libgpcd$(LIB_EXT) $(INSTALL_PATH)lib/
	chmod 0755          $(INSTALL_PATH)lib/libgpc$(LIB_EXT)
	chmod 0755          $(INSTALL_PATH)lib/libgpcd$(LIB_EXT)
	mv    $(INSTALL_PATH)lib/libgpc$(LIB_EXT)  $(INSTALL_PATH)lib/libgpc$(LIB_EXT).$(GPC_VERSION)
	mv    $(INSTALL_PATH)lib/libgpcd$(LIB_EXT) $(INSTALL_PATH)lib/libgpcd$(LIB_EXT).$(GPC_VERSION)
	ln -s $(INSTALL_PATH)lib/libgpc$(LIB_EXT).$(GPC_VERSION)  $(INSTALL_PATH)lib/libgpc$(LIB_EXT)
	ln -s $(INSTALL_PATH)lib/libgpcd$(LIB_EXT).$(GPC_VERSION) $(INSTALL_PATH)lib/libgpcd$(LIB_EXT)
	ldconfig
	@echo Installation succeeded.
endif

# libGPC release build is built from single header library when using Clang.
# Therefore, singleheadergen must be built using the debug library.
build/singleheadergen$(EXE_EXT): tools/singleheadergen.c | build/libgpcd$(LIB_EXT)
	$(CC) $? build/libgpcd$(LIB_EXT) $(CFLAGS) $(DEBUG_CFLAGS) -o $@

release: override CFLAGS += $(RELEASE_CFLAGS)
release: build/libgpc$(LIB_EXT)
debug: override CFLAGS += $(DEBUG_CFLAGS)
debug: build/libgpcd$(LIB_EXT)
tests: override CFLAGS += $(DEBUG_CFLAGS)
release_tests: override CFLAGS += $(RELEASE_CFLAGS)

# GCC static analyzer has been available since version 10, but the early
# versions are way too slow with way too much false positives so use v12 or newer.
STATIC_ANALYZER_AVAILABLE = $(shell expr `gcc -dumpversion | cut -f1 -d.` \>= 12)
ifeq ($(STATIC_ANALYZER_AVAILABLE), 1)
analyze: override CFLAGS += -fanalyzer
endif
analyze: build_tests # you probably want to run make tests after

ifeq ($(OS), Windows_NT)
build/libgpc$(LIB_EXT): $(OBJS)
	ar -crs $@ $^
else ifeq ($(CC), gcc)
build/libgpc$(LIB_EXT): $(OBJS)
	$(CC) -shared -O3 -flto=auto -o $@ $^
else # unity build
build/libgpc$(LIB_EXT): single_header
build/libgpc$(LIB_EXT):
	$(CC) $(RELEASE_CFLAGS) -xc -fpic src/gpc._c -shared -o $@
endif

ifeq ($(OS), Windows_NT)
build/libgpcd$(LIB_EXT): $(DEBUG_OBJS)
	ar -crs $@ $^
else
build/libgpcd$(LIB_EXT): $(DEBUG_OBJS)
	$(CC) -shared -o $@ $^
endif

$(OBJS): build/%.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c -fpic $(CFLAGS) $< -o $@

$(DEBUG_OBJS): build/%d.o : src/%.c
	mkdir -p build
	$(CC) -MMD -MP -c -fpic $(CFLAGS) $< -o $@

-include $(OBJS:.o=.d)
-include $(DEBUG_OBJS:.o=.d)

build_tests: override CFLAGS += -DGP_TESTS $(DEBUG_CFLAGS)
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

build_release_tests: override CFLAGS += -DGP_TESTS $(RELEASE_CFLAGS)
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

