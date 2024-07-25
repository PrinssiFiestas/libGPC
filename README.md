# libGPC General Purpose C

General purpose library to make C programming easier, safer, and more performant.

Safety and performance are achieved by integrating allocators everywhere where dynamic memory is needed. No ghost allocations, crazy global state, null terminated strings, or other stupidity. There are additional type safe generic macros, but most functionality can be accessed with just regular C functions with zero macro magic.

All functions (apart from non-essential `gp_set_utf8_global_locale()`) are thread safe and a big portion of them are reentrant. There are no internal heap allocations other than the ones required by `locale_t` and allocator initialization. Even using the scratch allocator and allocators provided by the user is kept minimal internally for predictable and fast performance.

## What is Included

Dynamic UTF-8 encoded strings with localization, reentrant and `malloc()` free ASCII byte arrays, type generic dynamic arrays, hash maps, unit testing, polymorphic memory allocators, arena allocators, automatic memory management with the scope allocator, and more. Check the header files to see everything provided.

### Example

```c
#include <gpc/gpc.h>

GPString i_am_the_prince(GPAllocator* out_lifetime) {
    GPAllocator* scope = gp_begin(0); // 0 for default scope arena size.
    const size_t init_capacity = 4; // too small, but GPString just grows if necessary.
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

The code example above is, of course, silly and not as concise as it could be, but it demonstrates the flexibility of different memory usage strategies.

Only one macro was used: `gp_str_on_stack()`, which allows creating a string on stack that reallocates elsewhere, if needed, providing seamless way to use stack memory safely, even for dynamic data.

The scope allocator is arena based which limits heap allocations, and thus is much faster than traditional `malloc()` `free()` pairs. It can also handle forgotten `gp_end()`call or misplaced return statement so memory leaks are practically impossible.

## Documentation

On the works. Installation instructions below. Effort has been made to make the unit tests well documenting so check the tests directory for detailed code examples.

## Platform Support

libGPC is regularly tested with MSVC on Windows x64, GCC/Clang on x86_64 Ubuntu, MinGW GCC/Clang on MSYS2 UCRT64, and GCC/Clang on 32-bit ARM Raspbian. Portability has been a key consideration when designing and writing the library, so it is likely that it works in other platforms as well, although this is not regularly tested.

### C++

The single header library cannot be compiled with a C++ compiler. However, the headers and the library work fine. The only behavioral difference with C++ is that `gp_assert()`, `gp_expect()`, and `gp_print()` family of macros are not capable of handling format strings and they use `std::ostringstream` internally which unfortunately plummets the performance due to allocations. As an upside though, they are more generic than their C counterparts.

The other difference is that `gp_str_on_stack()` and `gp_arr_on_stack()` macros are unfortunately impossible to implement in C++, at least as far as the authors have tried. Please, leave a PR if you figure something out. Fortunately, their functionality can be replicated manually with the only cost being verbosity. Check `string.h` and `array.h` for details.

## Installation and Usage

You can either install the full library from source (easy and recommended) or just use the single header library.

### Windows Installation

You need the [MSYS2](https://www.msys2.org/) building platform. Once MSYS2 is installed, you need to run the MSYS2 UCRT64 shell as administrator. If GCC and Make are not installed, install them by running

```
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S make
```

Then run the following commands:

```
git clone https://github.com/PrinssiFiestas/libGPC.git --depth 1
cd libGPC
make install
```

### Others Installation

Install GCC and Make using your package manager, if not already installed. Then run

```
git clone https://github.com/PrinssiFiestas/libGPC.git --depth 1
cd libGPC
sudo make install
```

where `sudo` may or may not be needed depending on your system.

### Linking

Once installed, with GCC you link with `-lgpc -lm -lpthread` or optionally `-lgpcd -lm -lpthread` for debug builds.`-lm` is optional with Clang. With `-lgpcd` you also might need `-fsanitize=address -fsanitize=undefined` depending on your system. Using sanitizers is massively encouraged anyway. If sanitizers give you problems with LD_PRELOAD, try `-static-libasan`.

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

The `#define` must be before the `#include` to expose the actual code in the header that gets compiled. Note that this file must be compiled as C, not C++. If you are using MSVC, `/utf-8` compiler flag is highly recommended, else you should link with `-lm -lpthread`, but not with `-lgpc` or `-lgpcd` since you are compiling the library as part of your project.

If you install the full library in Unix-like systems, the single header library will be copied to your system headers and can be included with

```c
#include <gpc/gpc.h>
```
