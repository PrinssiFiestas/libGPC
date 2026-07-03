// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TYPES_INCLUDED
#define GP_TYPES_INCLUDED 1

#include <gpc/attributes.h>
#include <gpc/overload.h>
#include <stddef.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup types Types
/// @code
/// #include <gpc/types.h>
/// @endcode
/// Miscellaneous type utilities.
/// @{

/** Type descriptor.
 *
 * Only primitives, `char*`, @ref GPString, `GPUInt128`, and `GPInt128` have
 * distinct enumeration constants, everything else will be assumed to be `void*`.
 */
typedef enum gp_type
{
    GP_NO_TYPE,
    GP_TYPE_BOOL,
    GP_TYPE_UNSIGNED_CHAR,
    GP_TYPE_UNSIGNED_SHORT,
    GP_TYPE_UNSIGNED,
    GP_TYPE_UNSIGNED_LONG,
    GP_TYPE_UNSIGNED_LONG_LONG,
    GP_TYPE_UINT128,
    GP_TYPE_CHAR,
    GP_TYPE_SIGNED_CHAR,
    GP_TYPE_SHORT,
    GP_TYPE_INT,
    GP_TYPE_LONG,
    GP_TYPE_LONG_LONG,
    GP_TYPE_INT128,
    GP_TYPE_FLOAT,
    GP_TYPE_DOUBLE,
    GP_TYPE_LONG_DOUBLE,
    GP_TYPE_CHAR_PTR,
    GP_TYPE_STRING,
    GP_TYPE_PTR,
    #define GP_TYPE_LENGTH (GP_TYPE_PTR + 1) ///< Number of defined `enum gp_type` constants.
} gp_type_t;

#ifdef GP_DOXYGEN

/** Portable `typeof` operator.
 *
 * Yields the type-name representing the type of it's operand. No implicit
 * conversions are applied to the argument.
 *
 * Defined when using C23, GNUC, MSVC, TCC, or C++11.
 */
#define GP_TYPEOF(...) typeof(__VA_ARGS__)

/** Get type descriptor enumeration constant of given value.
 *
 * Defined when using C11 or C++11.
 * @return a @ref gp_type_t enumeration constant describing the type.
 * Only primitives, `char*`, @ref GPString, `GPUInt128`, and `GPInt128` have
 * distinct enumeration constants, everything else will be assumed to be
 * `void*`.
 */
#define GP_TYPE(VALUE) (enum gp_type)_Generic((VALUE), ...)

/** Pointer type without spiral rule.
 *
 * A pointer to given non `void` argument type. For example `GP_PTR_TO(int)` is
 * the same as `int*` and `int(**fptrs)(int);` is the same as
 * `GP_PTR_TO(int(*)(int) fptrs;`. This is mostly used for macros that take a
 * type argument to avoid spiral rule. Example use:
 * @code
 * #define PTR_CAST(T, X) ((T*)(X)) // wrong, breaks for function pointers
 * #define PTR_CAST(T, X) ((GP_PTR_TO(T))(X)) // correct
 * @endcode
 */
#define GP_PTR_TO(...) GP_TYPEOF(&(__VA_ARGS__){0})

/** Type without spiral rule.
 *
 * Non `void` type without spiral rule. For example `GP_TYPEOF_TYPE(int)` is the
 * same as `int` and `int(*fptr)(int);` is the same as
 * `GP_TYPEOF_TYPE(int(*)(int)) fptr;`. This is mostly used for macros that take
 * a type argument to avoid spiral rule. Example use:
 * @code
 * #define Container(T) struct { T elem; } // wrong, breaks with function pointers
 * #define Container(T) struct { GP_TYPEOF_TYPE(T) elem; } // correct
 * @endcode
 */
#define GP_TYPEOF_TYPE(...) GP_TYPEOF((__VA_ARGS__){0})

#endif // GP_DOXYGEN

/** Get size of type from type descriptor.
 *
 * @return value such that `sizeof(X) == gp_type_size(GP_TYPE(X))`.
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

    case GP_NO_TYPE:;
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
 * Example:
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

// typeof() operator. GNUC and MSVC already covers mostly used compilers, but
// not all compilers are supported.
#if __STDC_VERSION__ >= 202311L || defined(__TINYC__)
#  define GP_HAS_TYPEOF 1
#  define GP_TYPEOF(...) typeof(__VA_ARGS__)
#elif defined(_MSC_VER) || defined(__GNUC__)
#  define GP_HAS_TYPEOF 1
#  define GP_TYPEOF(...) __typeof__(__VA_ARGS__)
#elif __cplusplus >= 201103L
#  define GP_HAS_TYPEOF 1
#  define GP_TYPEOF(...) decltype(__VA_ARGS__)
#endif

#ifndef __cplusplus // Available in C++ as constexpr functions, not availaible in C99.
// Note: we don't differentiate between gp_tetra_uint_t and GPUInt128 here. This
// is because gp_tetra_uint_t is supposed to be internal implementation detail,
// we only added it here so it can be printed for debugging purposes.
#define GP_TYPE(...)                                   \
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
    default:               GP_TYPE_PTR)

#define/* bool */GP_IS_SIGNED(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_SIGNED_TYPE(1)     , default: 0))
#define/* bool */GP_IS_UNSIGNED(...)         ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_UNSIGNED_TYPE(1)   , default: 0))
#define/* bool */GP_IS_FLOAT(...)            ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_FLOAT(1)           , default: 0))
#define/* bool */GP_IS_NUMBER(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_NUMBER(1)          , default: 0))
#define/* bool */GP_IS_INTEGRAL(...)         ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_INTEGRAL_TYPE(1)   , default: 0))
#define/* bool */GP_IS_SIGNED_INTEGER(...)   ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_SIGNED_INTEGER(1)  , default: 0))
#define/* bool */GP_IS_UNSIGNED_INTEGER(...) ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_UNSIGNED_INTEGER(1), default: 0))
#define/* bool */GP_IS_INTEGER(...)          ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_INTEGER(1)         , default: 0))
#define/* bool */GP_IS_ARITHMETIC(...)       ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_ARITHMETIC_TYPE(1) , default: 0))
#define/* bool */GP_IS_STRING(...)           ((bool)_Generic((__VA_ARGS__), GP_C11_GENERIC_STRING(1)          , default: 0))
#elif __cplusplus >= 201103L // __cplusplus
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(bool               x) { (void)x; return GP_TYPE_BOOL;               }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(short              x) { (void)x; return GP_TYPE_SHORT;              }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(int                x) { (void)x; return GP_TYPE_INT;                }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(long               x) { (void)x; return GP_TYPE_LONG;               }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(long long          x) { (void)x; return GP_TYPE_LONG_LONG;          }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(GPInt128           x) { (void)x; return GP_TYPE_INT128;             }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(unsigned short     x) { (void)x; return GP_TYPE_UNSIGNED_SHORT;     }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(unsigned           x) { (void)x; return GP_TYPE_UNSIGNED;           }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(unsigned long      x) { (void)x; return GP_TYPE_UNSIGNED_LONG;      }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(unsigned long long x) { (void)x; return GP_TYPE_UNSIGNED_LONG_LONG; }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(GPUInt128          x) { (void)x; return GP_TYPE_UINT128;            }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(float              x) { (void)x; return GP_TYPE_FLOAT;              }
GP_NODISCARD static inline constexpr gp_type_t GP_TYPE(double             x) { (void)x; return GP_TYPE_DOUBLE;             }
#if !_MSC_VER
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(char               x) { (void)x; return GP_TYPE_CHAR;               }
#endif
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(unsigned char      x) { (void)x; return GP_TYPE_UNSIGNED_CHAR;      }
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(signed char        x) { (void)x; return GP_TYPE_SIGNED_CHAR;        }
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(char*              x) { (void)x; return GP_TYPE_CHAR_PTR;           }
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(const char*        x) { (void)x; return GP_TYPE_CHAR_PTR;           }
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(struct gp_char*    x) { (void)x; return GP_TYPE_STRING;             }
GP_NODSICARD static inline constexpr gp_type_t GP_TYPE(const void*        x) { (void)x; return GP_TYPE_PTR;                }

template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_SIGNED(T X)           { return gp_type_is_signed(GP_TYPE(X))          ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_UNSIGNED(T X)         { return gp_type_is_unsigned(GP_TYPE(X))        ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_FLOAT(T X)            { return gp_type_is_float(GP_TYPE(X))           ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_NUMBER(T X)           { return gp_type_is_number(GP_TYPE(X))          ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_INTEGRAL(T X)         { return gp_type_is_integral(GP_TYPE(X))        ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_SIGNED_INTEGER(T X)   { return gp_type_is_signed_integer(GP_TYPE(X))  ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_UNSIGNED_INTEGER(T X) { return gp_type_is_unsigned_integer(GP_TYPE(X)); }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_INTEGER(T X)          { return gp_type_is_integer(GP_TYPE(X))         ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_ARITHMETIC(T X)       { return gp_type_is_arithmetic(GP_TYPE(X))      ; }
template <typename T> GP_NODISCARD static inline constexpr bool GP_IS_STRING(T X)           { return gp_type_is_string(GP_TYPE(X))          ; }
#endif

///@endcond
#endif // GP_TYPES_INCLUDED
