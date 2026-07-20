// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/target.h>

#ifndef GP_ATTRIBUTES_INCLUDED
#define GP_ATTRIBUTES_INCLUDED 1

#ifdef GP_DOXYGEN
// ----------------------------------------------------------------------------
/// @defgroup attributes Attributes
/// @code
/// #include <gpc/attributes.h>
/// @endcode
/// This module contains portable macros mostly wrapping GNUC attributes.
/// Attributes are used to give compiler additional information, which can be
/// used for better diagnostics and better optimizations among other things. All
/// unsupported macros expand to nothing, so all of these macros can be used
/// regardless of compiler or C/C++ dialect.
/// @{

/** Disable type checking of custom format strings.
 *
 * Should be defined per header file inclusion if needed.
 *
 * Some functions like `gp_snprintf()` use custom format strings to be able to
 * handle @ref GPString using `%S` among other things. However, compilers might
 * warn when using custom formats. This macro can be used to disable these
 * checks.
 */
#define GP_NO_FORMAT_STRING_CHECK

/** Disable non-standard extensions.
 *
 * Should be defined in command line if needed. Provided Makefile will not need
 * this.
 *
 * This library shamelessly uses non-standard compiler extensions to improve
 * safety, performance, and even portability in some cases. Compiler extensions
 * are rarely used just to add features, they are mostly used to improve
 * existing ones and most features do not use them at all.
 *
 * This library detects different compilers using compiler predefined macros to
 * detect which extensions are available, so all headers should compile out of
 * the box with a wide range of compilers without issues. However, often
 * projects use `-Wpedantic` compiler flag to enforce portability (which is fair
 * enough). In those cases, users can define this macro to configure this
 * library to only use standard features. Most features will not be disabled
 * completely, the features may use a less optimal and/or less safe alternative
 * and portable implementation instead.
 */
#define GP_PEDANTIC

/** Omit machine code of @ref GP_INLINE functions from output binary.
 *
 * Should be defined when compiling this library if desired.
 *
 * By default @ref GP_INLINE functions are compiled to output binary, which
 * allows using inline functions in the foreign function interface. However,
 * pure C/C++ do not benefit from having inline functions in binary, so this
 * macro can be used to reduce output binary size.
 */
#define GP_NO_EXPORT_INLINES

/** Compile to DLL.
 *
 * Should be defined when compiling this library into a DLL. Should not be used
 * with provided Makefile.
 *
 * This library assumes by default that it will be either statically linked or
 * linked to an ELF shared object library. However, it is possible to compile to
 * a DLL on Windows, in which case this macro should be defined.
 */
#define GP_DLL_EXPORT

/** Use this DLL.
 *
 * Should be defined when using this library that has been compiled to a DLL.
 * Should not be used with provided Makefile.
 *
 * This library assumes by default that it will be either statically linked or
 * linked to an ELF shared object library. However, it is possible to compile to
 * a DLL on Windows, in which case this macro should be defined.
 */
#define GP_DLL_IMPORT

#endif // GP_DOXYGEN

// ----------------------------------------------------------------------------
// Alignment

// TODO this probably should go to memory module
/** Alignment of all pointers returned by any valid allocators.*/
#define GP_ALLOC_ALIGNMENT (2*sizeof(size_t))

// ----------------------------------------------------------------------------
// Nodiscard

/** Warn for ignored return value.
 *
 * Function attribute used when ignoring it's return value is clearly a bug.
 */
#if defined(__GNUC__)
#  define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER)
#  define GP_NODISCARD _Check_return_
#elif __cplusplus >= 201703L || __STDC_VERSION__ >= 202311L
#  define GP_NODISCARD [[nodiscard]]
#else
#  define GP_NODISCARD /* implementation defined */
#endif

// ----------------------------------------------------------------------------
// Nonnull

#ifdef GP_DOXYGEN
/** Non-null argument pointers.
 *
 * Function attribute used to specify which arguments must not be null pointers.
 * The arguments to this macro specify which arguments should not be null. If
 * arguments are omitted, then all arguments must not be null.
 *
 * GCC and Clang will omit a warning if they are able to infer that a null
 * pointer is as an argument with non-null attribute. In practice, this will be
 * most often when `NULL` constant is passed. Non-null attribute may also
 * enable some optimizations.
 *
 * To be extra explicit, this library doesn't just annotate non-null arguments,
 * but also arguments in function prototypes that can be null will have their
 * names prefixed with `optional_`.
 *
 * Example:
 * @code
 * GP_NONNULL_ARGS(2, 5)
 * int foo(
 *     void* optional_can_be_null,
 *     char* must_not_be_null,
 *     int   not_a_pointer_can_be_zero,
 *     void* optional_ok_if_null_again,
 *     void* this_must_not_be_null_either);
 * @endcode
 */
#  define GP_NONNULL_ARGS(...) /* implementation defined */

/** Non-null return pointer.
 *
 * Function attribute used to specify that the pointer returned by this function
 * will not be null. This can be used by the compiler for optimizations and
 * serves as documentation.
 *
 * Don't bother check if pointers returned by these functions are null, the
 * compiler will just optimize the check away since it is known that it won't be
 * null.
 */
#  define GP_NONNULL_RETURN /* implementation defined */

/** Non-null argument pointers and return value.
 *
 * Combines @ref GP_NONNULL_ARGS and @ref GP_NONNULL_RETURN. No argument shall
 * be null and the return value will not be null either.
 */
#  define GP_NONNULL_ARGS_AND_RETURN /* implementation defined */
#elif defined(__GNUC__)
#  define GP_NONNULL_ARGS(...) __attribute__((nonnull(__VA_ARGS__)))
#  define GP_NONNULL_RETURN __attribute__((returns_nonnull))
#  define GP_NONNULL_ARGS_AND_RETURN __attribute__((nonnull, returns_nonnull))
#elif defined(_MSC_VER)
#  define GP_NONNULL_ARGS(...)
#  define GP_NONNULL_RETURN _Ret_notnull_
#  define GP_NONNULL_ARGS_AND_RETURN _Ret_notnull_
#else
#  define GP_NONNULL_ARGS(...)
#  define GP_NONNULL_RETURN
#  define GP_NONNULL_ARGS_AND_RETURN
#endif

// ----------------------------------------------------------------------------
// Require Initialized Memory

/** In/out annotation for pointer arguments.
 *
 * Function attribute used to specify which arguments will be in/out, or in
 * other words, which pointers will be read from *and* written to. This allows
 * GCC to warn if an address to an uninitialized value is passed as an argument.
 *
 * First argument specifies which argument the attribute applies to. Second
 * argument is optional and can be omitted. If passed, it specifies which
 * argument contains maximum number of valid elements pointed by the first
 * argument.
 *
 * Example:
 * @code
 * GP_INOUT(2)
 * void foo(int, size_t*);
 *
 * void bar(void)
 * {
 *     size_t var;
 *     foo(0, &var); // Warning: var uninitialized.
 * }
 * @endcode
 */
#if defined(__GNUC__) && __GNUC__ >= 11 && !defined(__clang__)
#  define GP_INOUT(...) __attribute__((access(read_write, __VA_ARGS__)))
#else
#  define GP_INOUT(...) /* implementation defined */
#endif

// ----------------------------------------------------------------------------
// Restrict

/** Portable `restrict` qualifier.
 *
 * `restrict` is a keyword introduced in C99 that is used as a type qualifier
 * for pointer types. Any pointer with such qualifier are non-aliasing.
 *
 * `restrict` is not part of C++, but major compilers offer `__restrict` as an
 * extension. This macro expands to appropriate variant of `restrict`.
 */
#if defined(__GNUG__) || defined(_MSC_VER)
#  define GP_RESTRICT __restrict
#elif defined(__cplusplus)
#  define GP_RESTRICT
#else
#  define GP_RESTRICT restrict
#endif

// ----------------------------------------------------------------------------
// Optimize

#ifdef __clang__
#  define GP_OPTIMIZE_NONE __attribute__((optnone))
#  define GP_OPTIMIZE_HIGH __attribute__((minsize)) // Clang has no -O3 equivalent
#  define GP_OPTIMIZE_SIZE __attribute__((minsize))
#elif defined(__GNUC__)
#  define GP_OPTIMIZE_NONE __attribute__((optimize(0))) ///< Disable optimizations.
#  define GP_OPTIMIZE_HIGH __attribute__((optimize(3))) ///< Maximum optimizations.
#  define GP_OPTIMIZE_SIZE __attribute__((optimize("Os"))) ///< Optimize for size.
#else
/** Function attribute to disable optimizations.
 *
 * GCC documentation discourages using this, but this can be helpful for
 * debugging. GCC and Clang only.
 */
#  define GP_OPTIMIZE_NONE /* implementation defined */
/** Function attribute to optimize for size.
 *
 * GCC documentation discourages using this, but this can be helpful for
 * debugging. GCC and Clang only.
 */
#  define GP_OPTIMIZE_SIZE /* implementation defined */
/** Function attribute to maximize optimizations.
 *
 * GCC documentation discourages using this, but this can be helpful for
 * debugging. GCC and Clang only.
 */
#  define GP_OPTIMIZE_HIGH /* implementation defined */
#endif

// ----------------------------------------------------------------------------
// Disable sanitizers

/** Disable sanitizers.
 *
 * Function attribute to most notably disable the address sanitizer. This can be
 * useful to improve performance of critical and heavy but well tested and/or
 * memory insensitive functions in debug builds.
 */
#if defined(__GNUC__)
#  define GP_NO_SANITIZE __attribute__((no_sanitize("address", "leak", "undefined")))
#elif defined(_MSC_VER)
#  define GP_NO_SANITIZE __declspec(no_sanitize_address)
#else
#  define GP_NO_SANITIZE
#endif

// ----------------------------------------------------------------------------
// Printf Format String Type Checking

/** Type checking for format strings.
 *
 * Function attribute for type checking format string in `printf()`-like
 * functions. @a FORMAT_STRING_ARGUMENT specifies which argument contains the
 * format string. @a FIRST_TO_CHECK will be the first argument to be checked,
 * which is most often the first variadic argument.
 *
 * This macro can be disabled per header file inclusion by defining
 * @ref GP_NO_FORMAT_STRING_CHECK before including this header. This might be
 * necessary when using custom formats like `%S` for @ref GPString with
 * `gp_snprintf()`.
 *
 * Example:
 * @code
 * GP_CHECK_FORMAT_STRING(2, 4)
 * int my_printf(int whatever, const char* format, int something_else, ...);
 * @endcode
 */
#if (defined(__GNUC__) && !defined(GP_NO_FORMAT_STRING_CHECK))
#  define GP_CHECK_FORMAT_STRING(FORMAT_STRING_ARGUMENT, FIRST_TO_CHECK) \
      __attribute__((format(printf, FORMAT_STRING_ARGUMENT, FIRST_TO_CHECK)))
#else
#  define GP_CHECK_FORMAT_STRING(FORMAT_STRING_ARGUMENT, FIRST_TO_CHECK) /* implementation defined */
#endif

// ----------------------------------------------------------------------------
// GNU

/** Any other GNU C attributes.
 *
 * Used for any other attribute that is not part of this library that can be
 * used with GNU C compatible compilers. In practice, this will be any variant
 * of GCC and Clang. Portable code should make sure that all compilers that
 * define `__GNUC__` that are planned to be supported support the given
 * attribute. It should not be assumed that Clang automatically supports a GCC
 * attribute, this is not always true.
 */
#if defined(__GNUC__)
#define GP_GNU_ATTRIB(...) __attribute__((__VA_ARGS__))
#else
#define GP_GNU_ATTRIB(...) /* implementation defined */
#endif

// ----------------------------------------------------------------------------
// Predict

/** Predict branch.
 *
 * Provide branch prediction information for potentially better optimized
 * branching. Probably should not be used if you don't have a good reason to.
 *
 * Example:
 * @code
 * if (GP_UNLIKELY(thing == NULL))
 *     thing = get_thing();
 * if (GP_LIKELY(thing != NULL))
 *     do_the_thing(thing);
 * else
 *     abort();
 * @endcode
 */
#ifdef __GNUC__
#  define GP_LIKELY(...)   __builtin_expect(!!(__VA_ARGS__), 1)
#  define GP_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
#  define GP_LIKELY(...)   /* implementation defined */(!!(__VA_ARGS__))
#  define GP_UNLIKELY(...) /* implementation defined */(!!(__VA_ARGS__)) ///< @copydoc GP_LIKELY
#endif

// ----------------------------------------------------------------------------
// API

/** Hidden global symbol visibility in shared libraries.
 *
 * Attribute to hide global symbols from shared object libraries. Used for
 * functions and global variables that are shared between translation units
 * within a shared library, but are not part of the API exposed to the user.
 *
 * It is strongly recommended to either use this attribute for all symbols when
 * applicable. This dramatically reduces linking and loading times and produces
 * more optimized code.
 *
 * See `-fvisibility` from `man gcc` for more information.
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#  define GP_HIDDEN __attribute__((visibility("hidden")))
#else
#  define GP_HIDDEN
#endif

/** Global variable or function that is part of public API.
 *
 * Attribute for any global symbols for importing and exporting from DLL. When
 * compiling into a DLL, @ref GP_DLL_EXPORT should be defined before including
 * this header. When using the DLL in another application, @ref GP_DLL_IMPORT
 * should be defined instead. Static builds and Unix shared objects do not need
 * to consider any of this.
 */
#ifdef GP_DOXYGEN
#  define GP_API /* implementation defined */
#elif defined(GP_TARGET_OS_WINDOWS) || defined(__CYGWIN__)
#  ifdef GP_DLL_EXPORT
#    define GP_API __declspec(dllexport)
#  elif defined(GP_DLL_IMPORT
#    define GP_API __declspec(dllimport)
#  else
#    define GP_API
#  endif
#else
#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#    define GP_API __attribute__((visibility("default")))
#  else
#    define GP_API
#  endif
#endif

/** Inline function that is also written in binary.
 *
 * Function with `GP_INLINE` specifier has it's implementation exposed for C/C++
 * applications for better optimized code. These functions will be written to
 * output binary. This allows using inline functions also in the foreign
 * function interface.
 *
 * Pure C and C++ applications probably will not need to have inline functions
 * exported to binary. To prevent exporting inline functions,
 * @ref GP_NO_EXPORT_INLINES should be defined when compiling this library.
 */
#ifdef GP_DOXYGEN
#  define GP_INLINE /** GP_API | static inline */
#elif !defined(GPC_IMPLEMENTATION) || defined(GP_NO_EXPORT_INLINES)
#  define GP_INLINE static inline
#elif defined(GP_SHARED_EXPORT)
#  define GP_INLINE GP_API
#else
#  define GP_INLINE
#endif

// ----------------------------------------------------------------------------
// No Return

/** Never return.
 *
 * Function attribute to indicate that the given function does not return. This
 * enables some optimizations and improves diagnostics e.g. suppress false
 * positives of uninitialized variables.
 */
#if __STDC_VERSION__ >= 202311L
#  define GP_NORETURN [[noreturn]]
#elif defined(__GNUC__)
#  define GP_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#  define GP_NORETURN __declspec((noreturn))
#else
#  define GP_NORETURN /** implementation defined */
#endif

// ----------------------------------------------------------------------------
// No Inline

/** Never inline.
 *
 * Function attribute to indicate that the given function shall not be inlined.
 */
#ifdef __GNUC__
#  define GP_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#  define GP_NOINLINE __declspec((noinline))
#else
#  define GP_NOINLINE /** implementation defined */
#endif

// ----------------------------------------------------------------------------
// Deprecated

/** Deprecate function, variable, type, or enumerator.
 *
 * Function, variable, type, and enumerator attribute to deprecate a symbol.
 * Any usage by the user of the given symbol will issue a warning.
 */

#if defined(__cplusplus) && __cplusplus >= 201402L
#  define GP_DEPRECATED [[deprecated]]
#elif defined(__GNUC__)
#  define GP_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#  define GP_DEPRECATED __declspec(deprecated)
#else
#  define GP_DEPRECATED /** implementation defined */
#endif

/// @}
#endif // GP_ATTRIBUTES_INCLUDED
