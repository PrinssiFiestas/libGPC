# libGPC General Purpose C

General purpose library for modern C programming with focus on safety, performance, and developer experience.

## What is Included

Dynamic UTF-8 encoded strings with localization, reentrant ASCII byte arrays, type generic dynamic arrays, hash maps, unit testing, polymorphic memory allocators, arena allocators, automatic memory management with the scope allocator, and more.

## Why libGPC

Unlike many other libraries, libGPC integrates polymorphic allocators throughout its components. This makes memory management trivial while also providing performance improvements and reduced memory fragmentation.

libGPC offers a wide range of utilities while being lightweight and easy to integrate into existing projects. You can just use the single header library and you're good to go.

All functions (apart from non-essential `gp_set_utf8_global_locale()`) are thread safe and a big portion of them are reentrant. There are no internal heap allocations other than the ones required by `locale_t` and allocator initialization. Even using the scratch allocator and allocators provided by the user is kept minimal internally for predictable and fast performance.

### Example

The code example below is, of course, silly and not as concise as it could be, but it demonstrates the flexibility of different memory usage strategies.

```c
#include <gpc/gpc.h>

GPString i_am_the_prince(GPAllocator* out_lifetime) {
    GPAllocator* scope = gp_begin(0); // 0 for default scope arena size.
    const size_t init_capacity = 4; // too small, but GPString grows if necessary.
    GPString out = gp_str_new(out_lifetime, init_capacity, "");
    GPString s1  = gp_str_new(scope, init_capacity, "I am ");
    GPString s2  = gp_str_on_stack(scope, 16, "the Prince");
    gp_str_copy(&out, s1, gp_str_length(s1));   // Explicit length argument
    gp_str_append(&out, s2, gp_str_length(s2)); // provides generality without
    gp_str_append(&out, "!!!", strlen("!!!"));  // hidden strlen().
    gp_end(scope); // frees s1 and s2 if s2 allocated.
    return out; // lifetime tied to out_lifetime so it is safe to return it.
}
```

Only one macro was used: `gp_str_on_stack()`, which allows creating a string on stack that reallocates elsewhere, if needed, providing seamless way to use stack memory safely, even for dynamic data.

The scope allocator is arena based which limits heap allocations, and thus is much faster than traditional `malloc()` `free()` pairs. It can also handle forgotten `gp_end()`call or misplaced return statement so memory leaks are practically impossible.

## Documentation

Library installation and usage instructions below. API reference can be found in header files in `include/gpc/`. Code examples can be found in unit tests in `tests/` directory.

## Platform Support

libGPC is regularly tested with MSVC on Windows x64, GCC/Clang on x86_64 Ubuntu, MinGW GCC/Clang on MSYS2 UCRT64, and GCC/Clang on 32-bit ARM Raspbian. Some testing is also done with CompCert on x86_64 Ubuntu.

Full features are enabled with GNU C11. Standard C99 is supported with almost full features if not using MSVC. C++11 is mostly supported with minor differences, although the library must be compiled with a C compiler.

Building the single header library with MSVC requires C11 standard threads, which are only available after Visual Studio 2022 version 17.8 Preview 2. Older versions are not supported.

## Installation and Usage

You can either install the full library from source (easy and recommended) or use the single header library without installing.

### Windows Installation

You need the [MSYS2](https://www.msys2.org/) building platform. Once MSYS2 is installed, you need to run the MSYS2 UCRT64 shell as administrator. If GCC and Make are not installed, install them by running

```
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S make
```

and optionally install GDB with

```
pacman -S mingw-w64-ucrt-x86_64-gdb
```

Then run the following commands:

```
git clone https://github.com/PrinssiFiestas/libGPC.git --depth 1
cd libGPC
make install
```

### Others Installation

Install GCC, Make, and optionally GDB using your package manager, if not already installed. Then run

```
git clone https://github.com/PrinssiFiestas/libGPC.git --depth 1
cd libGPC
sudo make install
```

If sanitizers with GCC give linking errors, install Clang and try

```
sudo make clean
sudo make install CC=clang
```

### Usage

Once installed, with GCC you link with `-lgpc -lm -lpthread` or optionally `-lgpcd -lm -lpthread` for debug builds. With `-lgpcd` you also might need `-fsanitize=address -fsanitize=undefined` depending on your system. Using sanitizers is massively encouraged anyway. If sanitizers give you problems with `LD_PRELOAD`, try `-static-libasan`.

If `-Wpedantic` or `-std=cXX` is used, `-DGP_PEDANTIC` can be used to disable compiler specific features. This is only required if not using GCC or MSVC. With Clang, it is also possible to use [-isystem](https://clangd.llvm.org/guides/system-headers) to enable full features.

To enable localization for some UTF-8 string functions, use `-D_GNU_SOURCE`.

### What is being installed

Shared library `libgpc.so` with it's headers, shared debug library `libgpcd.so`, a GDB pretty printer for `GPString`, and a command line utility `gprun`.

### `gprun`

A simple command line utility that just runs C/C++ files with sane default compiler flags. Useful for quickly testing things. Syntax:

```
gprun FILE ARGUMENTS
```

or

```
gprun "COMPILER_ARGUMENTS" ARGUMENTS
```

`gprun`uses `cc`, which is usually a soft link to GCC, to compile `FILE` to an executable which it runs immediately with `ARGUMENTS`. The first argument to `gprun` contains all the arguments passed to the compiler so if you want to pass multiple files or other arguments to the compiler, use quotes and put all files and arguments inside them. `-Wall -lgpcd -lm -lpthread`is passed by default. If `-o MY_EXECUTABLE_OUT` is not passed, the executable will be removed after execution. If optimization flags `-O1`, `-O2`, or `-O3`, or their MSVC counterparts with `/`, are not passed, debug symbols and sanitizers will be enabled if applicable, else `-DNDEBUG` is defined.

If `-c`or `/c`is passed, the input files are only compiled to object files without linking or executing. This differs from directly calling `cc` in the sense that the default flags are more sensible.

On Windows, `gprun` first looks for `cc`, and if it cannot find it, `cl.exe` (Microsoft compiler shipping with Visual Studio) is invoked instead. This allows the user to choose between `cc` and `cl` simply by choosing between MSYS2 UCRT shell and Developer Command Prompt for VS.

### `gpc.h` Single Header Library

If you only need the library, the easiest and most portable way is to download the [stb](https://github.com/nothings/stb)-style single header library `gpc.h` from releases and copy it into your project. Then, you select exactly one C file, or preferably create a dedicated one, and add this code snippet to it:

```c
#define GPC_IMPLEMENTATION
#include "gpc.h"
```

The `#define` must be before the `#include` to expose the actual code in the header that gets compiled. Note that this file must be compiled as C, not C++. If you are using MSVC, compiler flag `/std:c11` or `/std:c17` is required and `/utf-8` is highly recommended, else you should link with `-lm -lpthread`.

If you install the full library in Unix-like systems, the single header library will be copied to your system headers and can be included with

```c
#include <gpc/gpc.h>
```
