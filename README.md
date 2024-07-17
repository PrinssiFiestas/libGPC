# libGPC General Purpose C

General purpose library to make C programming easier, safer, and more performant. This is achieved by a combination of ideas, like design by contract and KISS principle, but one dominant factor is heavy use of allocators. As an example, dynamic data structures, like strings, encapsulate an allocator which, first, serves as lifetime annotation, and second, allows them to grow on need so there is practically no buffer overflows, memory leaks, or other memory bugs.

## What's included

Dynamic UTF-8 encoded strings, reentrant and `malloc()` free ASCII byte arrays, type generic dynamic arrays, hash maps, unit testing, polymorphic memory allocators, arena allocators, automatic memory management via scope allocator, and more. Check the header files to see everything provided.

```c
#include <gpc/string.h>
// This silly function could be written more concisely, but here we demostrate how
// memory might be managed as well.
GPString i_am_the_prince(const GPAllocator* out_lifetime) {
    GPAllocator* scope = gp_begin(0); // 0 for default scope arena size
    // Capacity of 4 is too small, but GPString just grows if necessary
    GPString out = gp_str_new(out_lifetime, 4, "");
    GPString s1  = gp_str_new(scope, 4, "I am ");
    GPString s2  = gp_str_on_stack(scope, 16, "the Prince");
    gp_str_copy(&out, s1, gp_str_length(s1));
    gp_str_append(&out, s2, gp_str_length(s2));
    gp_end(scope); // frees s1 and s2 if s2 allocated
    return out; // lifetime tied to out_lifetime so it is safe to return it
}
```

The code example above only uses one macro: `gp_str_on_stack()` which allows creating a string on stack that reallocates elsewhere, if needed, providing seamless way to use stack memory even for dynamic data. 

The scope allocator is arena based which limits heap allocations and is much faster than traditional `malloc()` `free()` pairs. It can also handle forgotten `gp_end()`call or misplaced return statement so memory leaks are practically impossible.

Most functionality is thread safe and a big portion is reentrant. There is no internal heap allocations other than the ones required by `locale_t` and allocator initialization. Even using the scratch allocator and allocators provided by the user is kept minimal internally. 

## Documentation

Writing dedicated up-to-date documentation in this very early stage of development is futile. However, effort has been made to make the unit tests well documenting so check the tests directory for detailed usage examples.

## Dependencies and platform support

Currently uses C11, Make, and GCC or Clang. Pthreads has to be available, but no other dependencies. Tested to build on x86_64 Ubuntu, Windows MSYS2 UCRT, and Raspbian 32-bit ARM. Support for other tools are on the works, including C99, C++, MSVC, and others.

## Installation

Currently on Windows, the installation has to be done manually by building the library and copying it and the header files to their appropriate locations. Headers in `include/printf` are not required. On Linux, download or clone the source code, navigate to project root and

```
make install
```

or

```
sudo make install
```

depending on your platform.

### What is being installed

Shared library `libgpc.so`, shared debug library `libgpcd.so`, a GDB pretty printer for `GPString`, and a command line utility `gprun`.

### gprun

A simple command line utility that just runs C/C++ files. Useful for quickly testing things. Syntax:

```
gprun FILE ARGUMENTS
```

or

```
gprun "COMPILER_ARGUMENTS" ARGUMENTS
```

`gprun`uses `cc`to compile `FILE` to an executable which it runs immediately with `ARGUMENTS`. The first argument to `gprun` contains all the arguments passed to the compiler so if you want to pass multiple files or other arguments to the compiler, use quotes and put all files and arguments inside them. `-Wall -lgpcd -lm -lpthread`is passed by default. If `-o MY_EXECUTABLE_OUT` is not passed, the executable will be removed after execution. If `-OX` is not passed, debug symbols and sanitizers will be enabled if applicable.

## Building from source

Download or clone the source code, navigate to project root and

```
make
```

or

```
make CC=clang
```

Debug unit tests can be built and run with

```
make tests
```

Release tests can be built and run with

```
make release_tests
```

## Usage

After installation, `#include <gpc/MODULE.h>` and link with `-lgpc -lm -lpthread` or `-lgpcd -lm -lpthread` in debug mode. Check the headers for available modules.
