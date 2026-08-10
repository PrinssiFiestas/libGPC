// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TYPES_INCLUDED
#define GP_TYPES_INCLUDED 1

#include <gpc/gpattributes.h>
#include <gpc/gpoverload.h>
#include <stddef.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup types Type Utilities
/// @code
/// #include <gpc/gptypes.h>
/// @endcode
/// Miscellaneous type utilities.
/// @{

/** Type descriptor.
 *
 * A type descriptor can be obtained from a value using @ref gp_type().
 *
 * Only primitives, `char*`, @ref GPString, `GPUInt128`, and `GPInt128` have
 * distinct enumeration constants, everything else will be assumed to be `void*`.
 */ // TODO example?
typedef enum gp_type_t
{
    GP_TYPE_NONE              ,
    GP_TYPE_BOOL              ,
    GP_TYPE_UNSIGNED_CHAR     ,
    GP_TYPE_UNSIGNED_SHORT    ,
    GP_TYPE_UNSIGNED          ,
    GP_TYPE_UNSIGNED_LONG     ,
    GP_TYPE_UNSIGNED_LONG_LONG,
    GP_TYPE_UINT128           ,
    GP_TYPE_CHAR              ,
    GP_TYPE_SIGNED_CHAR       ,
    GP_TYPE_SHORT             ,
    GP_TYPE_INT               ,
    GP_TYPE_LONG              ,
    GP_TYPE_LONG_LONG         ,
    GP_TYPE_INT128            ,
    GP_TYPE_FLOAT             ,
    GP_TYPE_DOUBLE            ,
    GP_TYPE_LONG_DOUBLE       ,
    GP_TYPE_CHAR_PTR          ,
    GP_TYPE_STRING            ,
    GP_TYPE_PTR               ,
    #define GP_TYPE_LENGTH 21 ///< Number of `enum gp_type_t` constants.
} gp_type_t;

/** Type descriptor string.
 *
 * Use @ref gp_type_str() to obtain a string describing the type from a value.
 */
static const char* gp_type_strs[GP_TYPE_LENGTH] =
{
    [GP_TYPE_NONE              ] = "(none)",
    [GP_TYPE_BOOL              ] = "bool",
    [GP_TYPE_UNSIGNED_CHAR     ] = "unsigned char",
    [GP_TYPE_UNSIGNED_SHORT    ] = "unsigned short",
    [GP_TYPE_UNSIGNED          ] = "unsigned",
    [GP_TYPE_UNSIGNED_LONG     ] = "unsigned long",
    [GP_TYPE_UNSIGNED_LONG_LONG] = "unsigned long long",
    [GP_TYPE_UINT128           ] = "GPUInt128",
    [GP_TYPE_CHAR              ] = "char",
    [GP_TYPE_SIGNED_CHAR       ] = "signed char",
    [GP_TYPE_SHORT             ] = "short",
    [GP_TYPE_INT               ] = "int",
    [GP_TYPE_LONG              ] = "long",
    [GP_TYPE_LONG_LONG         ] = "long long",
    [GP_TYPE_INT128            ] = "GPInt128",
    [GP_TYPE_FLOAT             ] = "float",
    [GP_TYPE_DOUBLE            ] = "double",
    [GP_TYPE_LONG_DOUBLE       ] = "long double",
    [GP_TYPE_CHAR_PTR          ] = "char*",
    [GP_TYPE_STRING            ] = "GPString",
    [GP_TYPE_PTR               ] = "void*",
};

#ifdef GP_DOXYGEN

/** Get type descriptor enumeration constant of given value.
 *
 * Defined when using C11 or C++11.
 *
 * @return a @ref gp_type_t enumeration constant describing the type.
 * Only primitives, `char*`, @ref GPString, `GPUInt128`, and `GPInt128` have
 * distinct enumeration constants, everything else will be assumed to be
 * `void*`.
 */
#define gp_type(__VA_ARGS__) (enum gp_type_t)_Generic((__VA_ARGS__), ...)

/** Get type descriptor string from a value or a type descriptor.
 *
 * Defined when using C11 or C++11.
 *
 * @return a @ref gp_type_t enumeration constant as a C string describing the type.
 * Only primitives, `char*`, @ref GPString, `GPUInt128`, and `GPInt128` have
 * distinct enumeration constants, everything else will be assumed to be
 * `void*`.
 */
#define gp_type_str(__VA_ARGS__) gp_type_strs \
[ \
    _Generic(__VA_ARGS__, gp_type_t: (__VA_ARGS__), default: gp_type(__VA_ARGS__)) \
]

/** Checks if given type descriptor indicates a signed type.
 *
 * @return `true` for
 * signed primitive integers, `GPInt128`, and plain `char` if signed.
 */
#define gp_type_is_signed(gp_type_t__TYPE) ((bool)(GP_TYPE_CHAR <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_INT128 ))

/** Checks if given type descriptor indicates an unsigned type.
 *
 * @return `true` for
 * unsigned primitive integers, `GPUInt128`, `bool`, and plain `char` if
 * unsigned.
 */
#define gp_type_is_unsigned(gp_type_t__TYPE) ((bool)(GP_TYPE_BOOL <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_UINT128))

/** Checks if given type descriptor indicates a floating type.
 *
 * @return `true` for
 * `float`, `double`, and `long double`.
 */
#define gp_type_is_float(gp_type_t__TYPE) ((bool)(GP_TYPE_FLOAT <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_LONG_DOUBLE ))

/** Checks if given type descriptor indicates a type considered a number.
 *
 * @return `true` for
 * Primitive integers, floats, `bool`, plain `char`, and `GP[U]Int128`.
 */
#define gp_type_is_number(gp_type_t__TYPE) ((bool)(GP_TYPE_BOOL <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_LONG_DOUBLE ))

/** Checks if given type descriptor indicates a type considered an integer..
 *
 * @return `true` for
 * Signed primitive integers, unsigned primitive integers, `GPInt128`,
 * `GPUInt128`, `bool` and plain `char`.
 */
#define gp_type_is_integral(gp_type_t__TYPE) ((bool)(GP_TYPE_BOOL <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_INT128 ))

/** Checks if given type descriptor indicates a signed primitive integer.
 *
 * @return `true` for all signed primitive integers.
 *
 * Plain `char` is not considered as integer by this macro since it is mostly
 * commonly used to represent text or raw bytes.
 */
#define gp_type_is_signed_integer(gp_type_t__TYPE) ((bool)(GP_TYPE_SIGNED_CHAR <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_LONG_LONG ))

/** Checks if given type descriptor indicates an unsigned primitive integer.
 *
 * @return `true` for all unsigned primitive integers.
 *
 * Plain `char` is not considered as integer by this macro since it is most
 * commonly used to represent text or raw bytes. `bool` is also not considered
 * as integer since it is most commonly used to represent logic.
 */
#define gp_type_is_unsigned_integer(gp_type_t__TYPE) ((bool)(GP_TYPE_UNSIGNED_CHAR <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG))

/** Checks if given type descriptor indicates a primitive integer.
 *
 * @return `true` for all primitive integers.
 *
 * Plain `char` is not considered as integer by this macro since it is most
 * commonly used to represent text or raw bytes. `bool` is also not considered
 * as integer since it is most commonly used to represent logic.
 */
#define gp_type_is_integer(gp_type_t__TYPE) ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t__TYPE) && (gp_type_t__TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t__TYPE) && (gp_type_t__TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG)))

/** Checks if given type descriptor indicates an arithmetic type.
 *
 * @return `true` for
 * All primitive integers and floating point types. Plain `char` and `bool` are
 * excluded.
 */
#define gp_type_is_arithmetic(gp_type_t__TYPE) ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t__TYPE) && (gp_type_t__TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t__TYPE) && (gp_type_t__TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG) \
    || (GP_TYPE_FLOAT <= gp_type_t__TYPE && gp_type_t__TYPE <= GP_TYPE_LONG_DOUBLE)))

/** Checks if given type descriptor indicates a string type.
 *
 * @return `true` for null-terminated `char*` and `GPString`.
 */
#define gp_type_is_string(gp_type_t__TYPE) ((bool)(GP_TYPE_CHAR_PTR <= (gp_type_t__TYPE) \
    && (gp_type_t__TYPE) <= GP_TYPE_STRING))

/** Portable `typeof` operator.
 *
 * Yields the type-name representing the type of it's operand. No implicit
 * conversions are applied to the argument.
 *
 * Defined when using C23, GNUC, MSVC, TCC, or C++11. Different compilers with
 * different compiler settings require different variants of this operator.
 * For example, C++ has `decltype` and some compilers might require `__typeof__`.
 * This portably expands to an appropriate one.
 */
#define GP_TYPEOF(...) typeof(__VA_ARGS__)

/** Pointer type without spiral rule.
 *
 * A pointer to given non `void` argument type. For example `GP_PTR_TO(int)` is
 * the same as `int*` and `int(**fptrs)(int);` is the same as
 * `GP_PTR_TO(int(*)(int) fptrs;`. This is mostly used for macros that take a
 * type argument to avoid spiral rule. Example use:
 *
 * ```c
 * #define PTR_CAST(T, X) ((T*)(X)) // WRONG: breaks for function pointers
 * #define PTR_CAST(T, X) ((GP_PTR_TO(T))(X)) // correct
 * ```
 *
 * If no variant of @ref GP_TYPEOF is available, then just expands to `T*`,
 * which will require typedefs for function pointers and pointers to arrays.
 */
#define GP_PTR_TO(...) GP_TYPEOF(&(__VA_ARGS__){0})

/** Type without spiral rule.
 *
 * Non `void` type without spiral rule. For example `GP_TYPEOF_TYPE(int)` is the
 * same as `int` and `int(*fptr)(int);` is the same as
 * `GP_TYPEOF_TYPE(int(*)(int)) fptr;`. This is mostly used for macros that take
 * a type argument to avoid spiral rule. Example use:
 *
 * ```c
 * #define Container(T) struct { T elem; } // WRONG: breaks with function pointers
 * #define Container(T) struct { GP_TYPEOF_TYPE(T) elem; } // correct
 * ```
 *
 * If no variant of @ref GP_TYPEOF is available, then just expands to `T*`,
 * which will require typedefs for function pointers and pointers to arrays.
 */
#define GP_TYPEOF_TYPE(...) GP_TYPEOF((__VA_ARGS__){0})

/** Cast to type.
 *
 * If @ref GP_TYPEOF is defined, this expands to a cast operator that casts to
 * the type of the argument. If @ref GP_TYPEOF is not defined, then this expands
 * to nothing.
 *
 * This is most useful to improve type safety of macro functions that return a
 * `void*`.
 */
#define GP_TYPEOF_CAST(...) (GP_TYPEOF(__VA_ARGS__))

#endif // GP_DOXYGEN

/** Cast integer to it's equivalent signed type.
 * Most useful for casting unsigned types to signed types for comparisons close
 * to zero. This improves type safety compared to cast by converting to an
 * integer of inferred size making the conversion typedef agnostic. It also
 * guarantees that `gp_as_signed(a - b) < 0` will always return 1 for all b > a.
 * If C99, the return value cannot be inferred and will be long long, but the
 * comparison property will still hold.
 */
#define gp_as_signed(X) GP_AS_SIGNED(X)

/** Get size of type from type descriptor.
 *
 * @return value such that `sizeof(X) == gp_type_size(gp_type(X))`.
 */
GP_INLINE size_t gp_type_size(const gp_type_t T)
{
    switch (T) {
    case GP_TYPE_CHAR: case GP_TYPE_SIGNED_CHAR: case GP_TYPE_UNSIGNED_CHAR:
        return sizeof(char);
    case GP_TYPE_SHORT: case GP_TYPE_UNSIGNED_SHORT:
        return sizeof(short);
    case GP_TYPE_BOOL:
        return sizeof(bool);
    case GP_TYPE_INT: case GP_TYPE_UNSIGNED:
        return sizeof(int);
    case GP_TYPE_LONG: case GP_TYPE_UNSIGNED_LONG:
        return sizeof(long);
    case GP_TYPE_LONG_LONG: case GP_TYPE_UNSIGNED_LONG_LONG:
        return sizeof(long long);
    case GP_TYPE_UINT128: case GP_TYPE_INT128:
        return 16;
    case GP_TYPE_FLOAT:
        return sizeof(float);
    case GP_TYPE_LONG_DOUBLE:
        #ifdef GP_HAS_LONG_DOUBLE
        return sizeof(long double);
        #endif
    case GP_TYPE_DOUBLE:
        return sizeof(double);
    case GP_TYPE_CHAR_PTR: case GP_TYPE_STRING: case GP_TYPE_PTR:
        return sizeof(char*);

    case GP_TYPE_NONE:;
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Long Double Support

// Compilers not supporting long double should be explicitly listed here,
// because long double has been in C standards since C89, so it is assumed to be
// supported by default.
#if !defined(__COMPCERT__) && !defined(__SDCC)

/** Check if code using `long double` compiles.
 *
 * This macro is defined if code using `long double` compiles. However, it does
 * not tell anything about it's size and often it will have the same size as
 * `double`. Some compilers may even treat `long double` as an alias to
 * `double`.
 *
 * If `long double` can be enabled via compiler switch (like `-flongdouble` in
 * CompCert), then user can define this macro if needed.
 */
#  define GP_HAS_LONG_DOUBLE 1

/** Check if `long double` is distinct from `double`.
 *
 * This macro is defined if `long double` is treated as a distinct type from
 * `double`. This does not mean that they will differ in size and many compilers
 * will treat them as if they had the same size. Knowing if `long double` is
 * distinct is mostly important for C11 `_Generic` selection, which does not
 * allow duplicate types.
 */
#  define GP_HAS_DIFFERENTIATED_LONG_DOUBLE 1

#endif // compilers (not) supporting long double

// ----------------------------------------------------------------------------
// Misc

/** Check anonymous structure compiler support.
 *
 * Anonymous structures and unions were introduced by C11, however some C99 and
 * C++ compilers (e.g. CompCert and TinyC) also support them.
 *
 * Can be disabled by defining @ref GP_PEDANTIC.
 */
#if (__STDC_VERSION__ >= 201112L || defined(__COMPCERT__) || defined(__TINYC__)) \
    || (defined(__GNUC__) && !defined(GP_PEDANTIC)) \
    || (defined(GP_DOXYGEN))
#  define GP_HAS_ANONYMOUS_STRUCT 1
#endif

/** Anonymous structure name.
 *
 * Used for portable anonymous structures. Anonymous structures and unions were
 * introduced by C11. This macro allows declaring pseudo-anonymous structures or
 * unions by generating an unique name based on `__LINE__` when using C99 or
 * C++. If C11 or higher, then the macro expands to nothing for truly anonymous
 * structures.
 *
 * ### Example
 *
 * @code
 * struct GP_ANONYMOUS_STRUCT
 * {
 *     void* data;
 *     void* more_data;
 * } my_global_data;
 * @endcode
 */
#if defined(GP_HAS_ANONYMOUS_STRUCT) || defined(GP_DOXYGEN)
#  define GP_ANONYMOUS_STRUCT
#else
#  define GP_MAKE_ANONYMOUS_STRUCT(LINE) GP_TOKEN_PASTE(_gp_anonymous_, LINE)
#  define GP_ANONYMOUS_STRUCT GP_MAKE_ANONYMOUS_STRUCT(__LINE__)
#endif

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

#ifdef GP_HAS_C11_GENERIC
#define GP_AS_SIGNED(X) _Generic(X,                         \
    GP_CHAR_SELECTION((signed char)(X),)                    \
    GP_TETRA_UINT_SELECTION((gp_tetra_int_t)(X),)           \
    GP_TETRA_INT_SELECTION((X),)                            \
    unsigned char     : (signed char)(X), signed char: (X), \
    unsigned short    : (short)(X)      , short      : (X), \
    unsigned int      : (int)(X)        , int        : (X), \
    unsigned long     : (long)(X)       , long       : (X), \
    unsigned long long: (long long)(X)  , long long  : (X))
#elif defined(__cplusplus)
GP_NODISCARD static inline constexpr signed char GP_AS_SIGNED(unsigned char      x) { return x; }
GP_NODISCARD static inline constexpr short       GP_AS_SIGNED(unsigned short     x) { return x; }
GP_NODISCARD static inline constexpr int         GP_AS_SIGNED(unsigned int       x) { return x; }
GP_NODISCARD static inline constexpr long        GP_AS_SIGNED(unsigned long      x) { return x; }
GP_NODISCARD static inline constexpr long long   GP_AS_SIGNED(unsigned long long x) { return x; }
#  if defined(GP_HAS_TETRA_INT)
GP_NODISCARD static inline constexpr gp_tetra_int_t GP_AS_SIGNED(gp_tetra_uint_t x) { return x; }
#  endif
template <typename T> GP_NODISCARD static inline constexpr T GP_AS_SIGNED(T x) { return x; }
#else // C99
#define GP_AS_SIGNED(X) \
( \
    sizeof(X) == 1 ? (int8_t )(X) : \
    sizeof(X) == 2 ? (int16_t)(X) : \
    sizeof(X) == 4 ? (int32_t)(X) : \
    sizeof(X) == 8 ? (int64_t)(X) : \
    (int64_t)(X)                    \
)
#endif

#ifdef __cplusplus
#define GP_PTR_TO(...) decltype(new(__VA_ARGS__))
 // decltype(*new(__VA_ARGS__)) is a reference for some reason?? Which is why we
 // need this dummy struct.
template <typename T> struct GPCPPType { T t; };
#define GP_TYPEOF_TYPE(...) decltype(GPCPPType<__VA_ARGS__>{}.t)
#elif defined(GP_TYPEOF)
#define GP_PTR_TO(...)      GP_TYPEOF(&(__VA_ARGS__){0})
#define GP_TYPEOF_TYPE(...) GP_TYPEOF( (__VA_ARGS__){0})
#else // typedefs may be required for function pointers and such
#define GP_PTR_TO(...) __VA_ARGS__*
#define GP_TYPEOF_TYPE(...) __VA_ARGS__
#endif

#if __STDC_VERSION__ >= 202311L || defined(__TINYC__)
#  define GP_TYPEOF(...) typeof(__VA_ARGS__)
#elif defined(_MSC_VER) || defined(__GNUC__)
#  define GP_TYPEOF(...) __typeof__(__VA_ARGS__)
#elif __cplusplus >= 201103L
#  define GP_TYPEOF(...) decltype(__VA_ARGS__)
#endif

#ifdef GP_TYPEOF
#  define GP_TYPEOF_CAST(...) (GP_TYPEOF(__VA_ARGS__))
#else
#  define GP_TYPEOF_CAST(...)
#endif

#ifndef __cplusplus // Available in C++ as constexpr functions, not availaible in C99.
// Note: we don't differentiate between gp_tetra_uint_t and GPUInt128 here. This
// is because gp_tetra_uint_t is supposed to be internal implementation detail,
// we only added it here so it can be printed for debugging purposes.
#define gp_type(...)                                   \
_Generic((__VA_ARGS__),                                \
    bool:                  GP_TYPE_BOOL,               \
    short:                 GP_TYPE_SHORT,              \
    int:                   GP_TYPE_INT,                \
    long:                  GP_TYPE_LONG,               \
    long long:             GP_TYPE_LONG_LONG,          \
    GP_INT128_SELECTION(GP_TYPE_INT128,)               \
    GP_TETRA_INT_SELECTION(GP_TYPE_INT128,)            \
    unsigned short:        GP_TYPE_UNSIGNED_SHORT,     \
    unsigned int:          GP_TYPE_UNSIGNED,           \
    unsigned long:         GP_TYPE_UNSIGNED_LONG,      \
    unsigned long long:    GP_TYPE_UNSIGNED_LONG_LONG, \
    GP_UINT128_SELECTION(GP_TYPE_UINT128,)             \
    GP_TETRA_UINT_SELECTION(GP_TYPE_UINT128,)          \
    float:                 GP_TYPE_FLOAT,              \
    double:                GP_TYPE_DOUBLE,             \
    GP_LONG_DOUBLE_SELECTION(GP_TYPE_LONG_DOUBLE,)     \
    GP_CHAR_SELECTION(GP_TYPE_CHAR,)                   \
    unsigned char:         GP_TYPE_UNSIGNED_CHAR,      \
    signed char:           GP_TYPE_SIGNED_CHAR,        \
    char*:                 GP_TYPE_CHAR_PTR,           \
    const char*:           GP_TYPE_CHAR_PTR,           \
    struct gp_char*:       GP_TYPE_STRING,             \
    const struct gp_char*: GP_TYPE_STRING,             \
    default:               GP_TYPE_PTR)

#define gp_type_str(...) gp_type_strs \
[ \
    _Generic(__VA_ARGS__, gp_type_t: (__VA_ARGS__), default: gp_type(__VA_ARGS__)) \
]

#define GP_IS_SIGNED(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_SIGNED_TYPE(1)     , default: 0))
#define GP_IS_UNSIGNED(...)         ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_UNSIGNED_TYPE(1)   , default: 0))
#define GP_IS_FLOAT(...)            ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_FLOAT(1)           , default: 0))
#define GP_IS_NUMBER(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_NUMBER(1)          , default: 0))
#define GP_IS_INTEGRAL(...)         ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_INTEGRAL_TYPE(1)   , default: 0))
#define GP_IS_SIGNED_INTEGER(...)   ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_SIGNED_INTEGER(1)  , default: 0))
#define GP_IS_UNSIGNED_INTEGER(...) ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_UNSIGNED_INTEGER(1), default: 0))
#define GP_IS_INTEGER(...)          ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_INTEGER(1)         , default: 0))
#define GP_IS_ARITHMETIC(...)       ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_ARITHMETIC_TYPE(1) , default: 0))
#define GP_IS_STRING(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_STRING(1)          , default: 0))
#elif __cplusplus >= 201103L // __cplusplus
GP_NODISCARD static inline constexpr gp_type_t gp_type(bool               x) { (void)x; return GP_TYPE_BOOL;               }
GP_NODISCARD static inline constexpr gp_type_t gp_type(short              x) { (void)x; return GP_TYPE_SHORT;              }
GP_NODISCARD static inline constexpr gp_type_t gp_type(int                x) { (void)x; return GP_TYPE_INT;                }
GP_NODISCARD static inline constexpr gp_type_t gp_type(long               x) { (void)x; return GP_TYPE_LONG;               }
GP_NODISCARD static inline constexpr gp_type_t gp_type(long long          x) { (void)x; return GP_TYPE_LONG_LONG;          }
GP_NODISCARD static inline constexpr gp_type_t gp_type(unsigned short     x) { (void)x; return GP_TYPE_UNSIGNED_SHORT;     }
GP_NODISCARD static inline constexpr gp_type_t gp_type(unsigned           x) { (void)x; return GP_TYPE_UNSIGNED;           }
GP_NODISCARD static inline constexpr gp_type_t gp_type(unsigned long      x) { (void)x; return GP_TYPE_UNSIGNED_LONG;      }
GP_NODISCARD static inline constexpr gp_type_t gp_type(unsigned long long x) { (void)x; return GP_TYPE_UNSIGNED_LONG_LONG; }
GP_NODISCARD static inline constexpr gp_type_t gp_type(float              x) { (void)x; return GP_TYPE_FLOAT;              }
GP_NODISCARD static inline constexpr gp_type_t gp_type(double             x) { (void)x; return GP_TYPE_DOUBLE;             }
#if !_MSC_VER
GP_NODISCARD static inline constexpr gp_type_t gp_type(char               x) { (void)x; return GP_TYPE_CHAR;               }
#endif
GP_NODISCARD static inline constexpr gp_type_t gp_type(unsigned char      x) { (void)x; return GP_TYPE_UNSIGNED_CHAR;      }
GP_NODISCARD static inline constexpr gp_type_t gp_type(signed char        x) { (void)x; return GP_TYPE_SIGNED_CHAR;        }
GP_NODISCARD static inline constexpr gp_type_t gp_type(char*              x) { (void)x; return GP_TYPE_CHAR_PTR;           }
GP_NODISCARD static inline constexpr gp_type_t gp_type(const char*        x) { (void)x; return GP_TYPE_CHAR_PTR;           }
GP_NODISCARD static inline constexpr gp_type_t gp_type(struct gp_char*    x) { (void)x; return GP_TYPE_STRING;             }
GP_NODISCARD static inline constexpr gp_type_t gp_type(const struct gp_char*x){(void)x; return GP_TYPE_STRING;             }
GP_NODISCARD static inline constexpr gp_type_t gp_type(const void*        x) { (void)x; return GP_TYPE_PTR;                }

GP_NODISCARD static inline constexpr const char* gp_type_str(gp_type_t x) { return gp_type_strs[x]; }
template <typename T> GP_NODISCARD static inline constexpr const char* gp_type_str(T x)
{ return gp_type_strs[gp_type(x)]; }

template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_SIGNED(T X)           { return gp_type_is_signed(gp_type(X))          ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_UNSIGNED(T X)         { return gp_type_is_unsigned(gp_type(X))        ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_FLOAT(T X)            { return gp_type_is_float(gp_type(X))           ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_NUMBER(T X)           { return gp_type_is_number(gp_type(X))          ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_INTEGRAL(T X)         { return gp_type_is_integral(gp_type(X))        ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_SIGNED_INTEGER(T X)   { return gp_type_is_signed_integer(gp_type(X))  ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_UNSIGNED_INTEGER(T X) { return gp_type_is_unsigned_integer(gp_type(X)); }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_INTEGER(T X)          { return gp_type_is_integer(gp_type(X))         ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_ARITHMETIC(T X)       { return gp_type_is_arithmetic(gp_type(X))      ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_STRING(T X)           { return gp_type_is_string(gp_type(X))          ; }
#endif // __cplusplus

#if CHAR_MAX == INT8_MAX // char is signed
#define gp_type_is_signed(gp_type_t_TYPE)           ((bool)(GP_TYPE_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128 ))
#define gp_type_is_unsigned(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UINT128))
#else // char is unsigned
#define gp_type_is_signed(gp_type_t_TYPE)           ((bool)(GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128))
#define gp_type_is_unsigned(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL        <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_CHAR  ))
#endif
#define gp_type_is_float(gp_type_t_TYPE)            ((bool)(GP_TYPE_FLOAT         <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_DOUBLE       ))
#define gp_type_is_number(gp_type_t_TYPE)           ((bool)(GP_TYPE_BOOL          <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_DOUBLE       ))
#define gp_type_is_integral(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL          <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128            ))
#define gp_type_is_signed_integer(gp_type_t_TYPE)   ((bool)(GP_TYPE_SIGNED_CHAR   <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG         ))
#define gp_type_is_unsigned_integer(gp_type_t_TYPE) ((bool)(GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG))

#define gp_type_is_integer(gp_type_t_TYPE)          ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG)))

#define gp_type_is_arithmetic(gp_type_t_TYPE)       ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG) \
    || (GP_TYPE_FLOAT <= gp_type_t_TYPE && gp_type_t_TYPE <= GP_TYPE_LONG_DOUBLE)))

#define gp_type_is_string(gp_type_t_TYPE)           ((bool)(GP_TYPE_CHAR_PTR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_STRING))

///@endcond
#endif // GP_TYPES_INCLUDED
