# MIT License
# Copyright (c) 2023 Lauri Lorenzo Fiestas
# https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

# Public targets
.PHONY: docs  # Build documentation. Requires Doxygen.
.PHONY: clean # Remove build artifacts.

CC = gcc

CFLAGS = -Wall -Wextra -Wswitch-enum -Wundef -Iinclude -D_GNU_SOURCE
LFLAGS = -lm -lpthread

DEBUG_CFLAGS = -ggdb3 -gdwarf

SO_CFLAGS = -shared -fPIC -fno-semantic-interposition -Wl,-Bsymbolic-functions

ifeq ($(CC), gcc) # only gcc has portable link-time optimizations.
	RELEASE_CFLAGS += -flto=auto
endif

NPROC        = $(shell echo `nproc`)
THREAD_COUNT = $(if $(NPROC),$(NPROC),4)
MAKEFLAGS   += -j$(THREAD_COUNT)

docs:
	@mkdir -p build/docs
	@doxygen

clean:
	rm -rf build
