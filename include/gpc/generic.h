// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file generic.h
 * Type generic macros.
 */

#ifndef GP_GENERIC_INCLUDED
#define GP_GENERIC_INCLUDED

#include <gpc/array.h>
#include <gpc/string.h>
#include <gpc/hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Note: macros may take variadic arguments even when not necessary for better
// error messages. Also in some occasions, but not always, allows using
// compound literals as macros arguments.

#define GPDictionary(T) T*

#ifdef GP_GENERIC_AVAILABLE
// TODO rest of C11 implementations. Only the macros with 99 postfix needs to be
// changed.

// Constructors
#define gp_arr(...)                 GP_ARR_NEW(__VA_ARGS__)
#define gp_arr_ro(T,...)            GP_ARR_READ_ONLY(T,__VA_ARGS__)
#define gp_str(...)                 GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)                GP_HMAP_NEW(__VA_ARGS__)
#define gp_dict(...)                GP_DICT_NEW(__VA_ARGS__)

// Bytes and strings
#define gp_equal(...)               GP_EQUAL(__VA_ARGS__)
#define gp_count(...)               GP_COUNT(__VA_ARGS__)
#define gp_codepoint_length(...)    GP_CODEPOINT_LENGTH(__VA_ARGS__)
#define gp_codepoint_classify(...)  GP_CODEPOINT_CLASSIFY(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT11(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE11(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL11(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM11(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
#define gp_length(...)              gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)            gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)          gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)           gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)             GP_RESERVE11(__VA_ARGS__)
#define gp_copy(...)                GP_COPY11(__VA_ARGS__)
#define gp_slice(...)               GP_SLICE99(__VA_ARGS__)
#define gp_append(...)              GP_APPEND99(__VA_ARGS__)
#define gp_insert(...)              GP_INSERT99(__VA_ARGS__)

// Arrays
#define gp_push(...)                GP_PUSH(__VA_ARGS__)
#define gp_pop(...)                 GP_POP(__VA_ARGS__)
#define gp_erase(...)               GP_ERASE(__VA_ARGS__)
#define gp_map(...)                 GP_MAP99(__VA_ARGS__)
#define gp_fold(...)                GP_FOLD(__VA_ARGS__)
#define gp_foldr(...)               GP_FOLDR(__VA_ARGS__)
#define gp_filter(...)              GP_FILTER99(__VA_ARGS__)

// Dictionarys
#define gp_get(...)                 GP_GET(__VA_ARGS__)
#define gp_put(...)                 GP_PUT(__VA_ARGS__)
#define gp_remove(...)              GP_REMOVE(__VA_ARGS__)

// Memory
#define gp_alloc(...)               GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)          GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...)        GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)             GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)             GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)                GP_FILE99(__VA_ARGS__)
#define gp_read_line(...)           gp_file_read_line(...)
#define gp_read_until(...)          gp_file_read_until(...)
#define gp_read_strip(...)          gp_file_read_strip(...)

#else // C99 ------------------------------------------------------------------

// Constructors
#define gp_arr(...)                 GP_ARR_NEW(__VA_ARGS__)
#define gp_arr_ro(T,...)            GP_ARR_READ_ONLY(T,__VA_ARGS__)
#define gp_str(...)                 GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)                GP_HMAP_NEW(__VA_ARGS__)
#define gp_dict(...)                GP_DICT_NEW(__VA_ARGS__)

// Bytes and strings
#define gp_equal(...)               GP_EQUAL(__VA_ARGS__)
#define gp_count(...)               GP_COUNT(__VA_ARGS__)
#define gp_codepoint_length(...)    GP_CODEPOINT_LENGTH(__VA_ARGS__)
#define gp_codepoint_classify(...)  GP_CODEPOINT_CLASSIFY(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT99(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE99(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL99(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM99(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
#define gp_length(...)              gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)            gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)          gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)           gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)             GP_RESERVE99(__VA_ARGS__)
#define gp_copy(...)                GP_COPY99(__VA_ARGS__)
#define gp_slice(...)               GP_SLICE99(__VA_ARGS__)
#define gp_append(...)              GP_APPEND99(__VA_ARGS__)
#define gp_insert(...)              GP_INSERT99(__VA_ARGS__)

// Arrays
#define gp_push(...)                GP_PUSH(__VA_ARGS__)
#define gp_pop(...)                 GP_POP(__VA_ARGS__)
#define gp_erase(...)               GP_ERASE(__VA_ARGS__)
#define gp_map(...)                 GP_MAP99(__VA_ARGS__)
#define gp_fold(...)                GP_FOLD(__VA_ARGS__)
#define gp_foldr(...)               GP_FOLDR(__VA_ARGS__)
#define gp_filter(...)              GP_FILTER99(__VA_ARGS__)

// Dictionarys
#define gp_get(...)                 GP_GET(__VA_ARGS__)
#define gp_put(...)                 GP_PUT(__VA_ARGS__)
#define gp_remove(...)              GP_REMOVE(__VA_ARGS__)

// Memory
#define gp_alloc(...)               GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)          GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...)        GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)             GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)             GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)                GP_FILE99(__VA_ARGS__)
#define gp_read_line(...)           gp_file_read_line(...)
#define gp_read_until(...)          gp_file_read_until(...)
#define gp_read_strip(...)          gp_file_read_strip(...)

#endif


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


typedef struct gp_str_in { const uint8_t* data; const size_t length; } GPStrIn;
#if GP_GENERIC_AVAILABLE
#define GP_STR_IN(...) GP_STR_IN11(__VA_ARGS__)
#define GP_ALC(...) GP_ALC11(__VA_ARGS__)
#else
#define GP_STR_IN(...) GP_STR_IN99(__VA_ARGS__)
#define GP_ALC(...) GP_ALC99(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
//
//          C11 IMPLEMENTATIONS
//
//

#ifndef GP_USER_ALLOCATORS
typedef struct { GPAllocator alc; } GPDummyAlc; // for comma issues in GP_ALC_TYPE
#define GP_USER_ALLOCATORS GPDummyAlc
#endif

#define GP_ALC_TYPES GP_USER_ALLOCATORS, GPAllocator, GPArena
#define GP_ALC_SELECTION(T) const T*: 0, T*: 0
#define GP_ALC11(A) ((int){0} = _Generic(A, \
    GP_PROCESS_ALL_ARGS(GP_ALC_SELECTION, GP_COMMA, GP_ALC_TYPES)), \
    (const GPAllocator*)(A))

// ----------------------------------------------------------------------------
// Bytes and strings

#define GP_STR_T(S) _Generic(S, GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR)
static inline GPStrIn gp_str_in11(const GPType T, const void*const data, const size_t length)
{
    switch (T)
    {
        case GP_STRING:   return (GPStrIn){ data, gp_arr_length(data) };
        case GP_CHAR_PTR: return (GPStrIn){ data, strlen(data) };
        default: break;
    }
    return (GPStrIn){ data, length };
}
#define GP_STR_IN11_1(S)    gp_str_in11(GP_STR_T(S), S, 0)
#define GP_STR_IN11_2(S, L) gp_str_in11(GP_PTR,      S, L)
#define GP_STR_IN11(...) GP_OVERLOAD2(__VA_ARGS__, GP_STR_IN11_2, GP_STR_IN11_1)(__VA_ARGS__)

static inline size_t gp_length_in11(const GPType T_unused, const size_t length, const size_t unused)
{
    (void)T_unused; (void)unused;
    return length;
}
 #define GP_STR_OR_LEN1(A) _Generic(A, \
     GPString: gp_str_in11, char*: gp_str_in11, const char*: gp_str_in11, default: gp_length_in11) \
     (_Generic(A, GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR, default: -1), A, 0)

#define GP_STR_OR_LEN(...) GP_OVERLOAD2(__VA_ARGS__, GP_STR_IN11_2, GP_STR_OR_LEN1)(__VA_ARGS__)

static inline GPString gp_str_repeat_new(const void* alc, const size_t count, GPStrIn in)
{
    GPString out = gp_str_new(alc, count * in.length, "");
    gp_bytes_repeat(out, count, in.data, in.length);
    ((GPStringHeader*)out - 1)->length = count * in.length;
    return out;
}
static inline void gp_str_repeat_str(GPString* dest, const size_t count, GPStrIn in)
{
    gp_str_repeat(dest, count, in.data, in.length);
}
#define GP_REPEAT_SELECTION(T) const T*: gp_str_repeat_new, T*: gp_str_repeat_new
#define GP_REPEAT11(A, COUNT, ...) _Generic(A, GPString*: gp_str_repeat_str, \
    GP_PROCESS_ALL_ARGS(GP_REPEAT_SELECTION, GP_COMMA, GP_ALC_TYPES))(A, COUNT, GP_STR_IN(__VA_ARGS__))

static inline void gp_replace11(GPString* hay, GPStrIn ndl, GPStrIn repl, const size_t start)
{
    gp_str_replace(hay, ndl.data, ndl.length, repl.data, repl.length, start);
}
GPString gp_replace_new(const GPAllocator* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl, size_t start);
static inline GPString gp_replace_new4(const void* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl)
{
    return gp_replace_new(alc, hay, ndl, repl, 0);
}
#define GP_REPLACE11_3(HAY, NDL, REPL) gp_replace11(HAY, GP_STR_IN11(NDL), GP_STR_IN11(REPL), 0)
#define GP_REPLACE_SELECTION(T) const T*: gp_replace_new4, T*: gp_replace_new4
#define GP_REPLACE11_4(A, B, C, D) _Generic(A, GPString*: gp_replace11, \
    GP_PROCESS_ALL_ARGS(GP_REPLACE_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, GP_STR_IN11(B), GP_STR_IN(C), GP_STR_OR_LEN(D))
#define GP_REPLACE11_5(ALC, HAY, NDL, REPL, START) gp_replace_new( \
    GP_ALC(ALC), GP_STR_IN11(HAY), GP_STR_IN11(NDL), GP_STR_IN11(REPL), START)
#define GP_REPLACE11(A,B,...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_REPLACE11_5, GP_REPLACE11_4, GP_REPLACE11_3)(A,B,__VA_ARGS__)

static inline size_t gp_replace_all11(GPString* hay, GPStrIn ndl, GPStrIn repl)
{
    return gp_str_replace_all(hay, ndl.data, ndl.length, repl.data, repl.length);
}
GPString gp_reaplce_all_new(const void* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl);
#define GP_REPLACE_ALL11_3(HAY, NDL, REPL) gp_replace_all11(HAY, GP_STR_IN11(NDL), GP_STR_IN11(REPL))
#define GP_REPLACE_ALL11_4(ALC, HAY, NDL, REPL) gp_replace_all_new( \
    GP_ALC(ALC), GP_STR_IN11(HAY), GP_STR_IN11(NDL), GP_STR_IN11(REPL))
#define GP_REPLACE_ALL11(A,B,...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_REPLACE_ALL11_4, GP_REPLACE_ALL11_3)(A,B,__VA_ARGS__)

GPString gp_str_trim_new(const void* alc, GPStrIn str, const char* char_set, int flags);
static inline void gp_str_trim2(GPString* str, const char*const char_set)
{
    gp_str_trim(str, char_set, 'l' | 'r');
}
static inline GPString gp_str_trim_new2(const void*const alc, GPStrIn str)
{
    return gp_str_trim_new(alc, str, NULL, 'l' | 'r');
}
static inline GPString gp_str_trim_new3(const void*const alc, GPStrIn str, const char*const char_set)
{
    return gp_str_trim_new(alc, str, char_set, 'l' | 'r');
}
#define GP_TRIM2_SELECTION(T) T*: gp_str_trim_new2, const T*: gp_str_trim_new2
#define GP_TRIM3_SELECTION(T) T*: gp_str_trim_new3, const T*: gp_str_trim_new3
#define GP_TRIM1(STR) gp_str_trim(STR, NULL, 'l' | 'r')
#define GP_TRIM11_2(A, B) _Generic(A, GPString*: gp_str_trim2, \
    GP_PROCESS_ALL_ARGS(GP_TRIM2_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, _Generic(A, GPString*: B, default: GP_STR_IN(B)))
#define GP_TRIM11_3(A, B, C) _Generic(A, GPString*: gp_str_trim, \
    GP_PROCESS_ALL_ARGS(GP_TRIM3_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, _Generic(A, GPString*: B, default: GP_STR_IN(B)), C)
#define GP_TRIM4(ALC, STR, CHARS, FLAGS) gp_str_trim_new(GP_ALC(ALC), GP_STR_IN(STR), CHARS, FLAGS)
#define GP_TRIM11(...) GP_OVERLOAD4(__VA_ARGS__, GP_TRIM4, GP_TRIM11_3, GP_TRIM11_2, GP_TRIM1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Strings and arrays

static inline void gp_str_reserve11(const size_t unused, GPString* str, const size_t size)
{
    (void)unused;
    gp_str_reserve(str, size);
}
#define GP_RESERVE11(A, SIZE) _Generic(A, \
    GPString: gp_str_reserve11, default: gp_arr_reserve)(sizeof**(A), A, SIZE)

typedef GPStrIn GPArrIn;
#define GP_ARR_T(A) _Generic(A, \
    GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR, default: GP_PTR)
static inline GPStrIn gp_arr_in11(const GPType T, const void*const data, const size_t length)
{
    if (length != SIZE_MAX)
        return (GPArrIn){ data, length };
    else if (T == GP_CHAR_PTR)
        return (GPArrIn){ data, strlen(data) };
    return (GPArrIn){ data, gp_arr_length(data) };
}
#define GP_ARR_IN11_1(A)    gp_arr_in11(GP_ARR_T(A), A, SIZE_MAX)
#define GP_ARR_IN11_2(A, L) gp_arr_in11(GP_PTR,      A, L)
#define GP_ARR_IN11(...) GP_OVERLOAD2(__VA_ARGS__, GP_ARR_IN11_2, GP_ARR_IN11_1)(__VA_ARGS__)

typedef struct { int dummy; } GPDummyType;
#define GP_TYPE_CHECK_SELECTION(T) T*: &(GPDummyType){0}, const T*: &(GPDummyType){0}
#define GP_TYPE_CHECK(PA, B) *_Generic(PA, \
    GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: *(PA)) = *_Generic(PA, \
        GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
        GPString*: _Generic(B, char*: (GPString)(B), const char*: (GPString)(B), default: B), \
        default: B)

static inline void gp_str_copy11(const size_t unused, GPString* dest, GPStrIn src)
{
    (void)unused;
    gp_str_copy(dest, src.data, src.length);
}
static inline void* gp_arr_copy11(const size_t elem_size, void* _dest, GPArrIn src)
{
    GPArray(void)* dest = _dest;
    return *dest = gp_arr_copy(elem_size, *dest, src.data, src.length);
}
static inline void* gp_arr_copy_new11(const size_t elem_size, const void* alc, GPArrIn src)
{
    void* out = gp_arr_new(alc, elem_size, src.length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = src.length;
    return memcpy(out, src.data, src.length * elem_size);
}
#define GP_COPY_SELECTION(T) T*: gp_arr_copy_new11, const T*: gp_arr_copy_new11
#define GP_COPY11(A,...) _Generic((GP_TYPE_CHECK(A, GP_1ST_ARG(__VA_ARGS__)), A), \
    GPString*: gp_str_copy11, \
    GP_PROCESS_ALL_ARGS(GP_COPY_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_arr_copy11) \
    (GP_SIZEOF_TYPEOF(*(GP_1ST_ARG(__VA_ARGS__))), A, GP_ARR_IN11(__VA_ARGS__))

// ----------------------------------------------------------------------------
//
//          C99 IMPLEMENTATIONS
//
//

#ifdef __GNUC__
// Suppress suspicious usage of sizeof warning.
#define GP_SIZEOF_TYPEOF(X) sizeof(typeof(X))
#else
#define GP_SIZEOF_TYPEOF(X) sizeof(X)
#endif

#define GP_ALC99(A) ((const GPAllocator*)(A))

// ----------------------------------------------------------------------------
// Constructors

inline GPArray(void) gp_arr99(const GPAllocator* alc,
    const size_t elem_size, const void*const init, const size_t init_length)
{
    GPArray(void) out = gp_arr_new(alc, elem_size, init_length > 4 ? init_length : 4);
    ((GPArrayHeader*)out - 1)->length = init_length;
    return memcpy(out, init, elem_size * init_length);
}
#define GP_ARR_NEW(ALC, TYPE, ...) (TYPE*)gp_arr99( \
    (GPAllocator*)(ALC), \
    sizeof(TYPE), \
    (TYPE[]){(TYPE){0},__VA_ARGS__} + 1, \
    sizeof((TYPE[]){(TYPE){0},__VA_ARGS__}) / sizeof(TYPE) - 1)

#if __GNUC__
#define GP_ARR_READ_ONLY(T, ...) (T const *)({ \
    static const struct GP_C99_UNIQUE_STRUCT(__LINE__) { \
        GPArrayHeader header; T data[GP_COUNT_ARGS(__VA_ARGS__)]; \
    }_gp_arr_ro = {.header = { \
        .length = GP_COUNT_ARGS(__VA_ARGS__), .capacity = GP_COUNT_ARGS(__VA_ARGS__), \
        .allocator = NULL, .allocation = NULL \
    }, .data = {__VA_ARGS__}}; \
    _gp_arr_ro.data; \
})
#else
#define GP_ARR_READ_ONLY(T, ...) \
    (T const *)(gp_arr_on_stack(NULL, GP_COUNT_ARGS(__VA_ARGS__), T, __VA_ARGS__))
#endif

#define GP_STR_NEW1(ALC)            gp_str_new((GPAllocator*)(ALC), 16, "")
#define GP_STR_NEW2(ALC, INIT)      gp_str_new((GPAllocator*)(ALC), 16, INIT)
#define GP_STR_NEW3(ALC, CAP, INIT) gp_str_new((GPAllocator*)(ALC), CAP, INIT)
#define GP_STR_NEW(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_STR_NEW3, GP_STR_NEW2, GP_STR_NEW1)(__VA_ARGS__)

#define GP_HMAP1(ALC) gp_hash_map_new((GPAllocator*)(ALC), NULL)
#define GP_HMAP2(ALC, ELEM_SIZE) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = NULL})
#define GP_HMAP3(ALC, ELEM_SIZE, DCTOR) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP4(ALC, ELEM_SIZE, DCTOR, CAP) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = CAP, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP_NEW(...) GP_OVERLOAD4(__VA_ARGS__, GP_HMAP4, GP_HMAP3, GP_HMAP2, GP_GMAP1)(__VA_ARGS__)

#define GP_DICT2(ALC, TYPE) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = sizeof(TYPE), .capacity = 0, .destructor = NULL})
#define GP_DICT3(ALC, TYPE, DCTOR) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = sizeof(TYPE), .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_DICT4(ALC, TYPE, DCTOR, CAP) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = sizeof(TYPE), .capacity = CAP, .destructor = (void(*)(void*))(DCTOR)})
#define GP_DICT_NEW(A,...) GP_OVERLOAD3(__VA_ARGS__, GP_DICT4, GP_DICT3, GP_DICT2)(A, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Bytes and strings

inline GPStrIn gp_str_in99(const void* data, const size_t length)
{
    return (GPStrIn) {
        .data   = data,
        .length = length != SIZE_MAX ? length : gp_arr_length(data)
    };
}
#define GP_STR_IN1(A) gp_str_in99( \
    (void*)(A), #A[0] == '"' ? GP_SIZEOF_TYPEOF(A) - sizeof "" : SIZE_MAX)
#define GP_STR_IN99(...) GP_OVERLOAD2(__VA_ARGS__, gp_str_in99, GP_STR_IN1)(__VA_ARGS__)

inline bool gp_equal99(const GPString a, GPStrIn b) {
    return gp_bytes_equal(a, gp_str_length(a), b.data, b.length);
}
#define GP_EQUAL2(A, B)               gp_equal99(A, GP_STR_IN(B))
#define GP_EQUAL3(A, B, B_LENGTH)     gp_str_equal(A, B, B_LENGTH)
#define GP_EQUAL4(A, A_LEN, B, B_LEN) gp_bytes_equal(A, A_LEN, B, B_LEN)
#define GP_EQUAL(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_EQUAL4, GP_EQUAL3, GP_EQUAL2)(A, __VA_ARGS__)

inline size_t gp_count99(GPStrIn haystack, GPStrIn needle) {
    return gp_bytes_count(haystack.data, haystack.length, needle.data, needle.length);
}
#define GP_COUNT2(A, B)       gp_count99(GP_STR_IN(A), GP_STR_IN(B))
#define GP_COUNT3(A, B, C)    gp_count99(GP_STR_IN(A), GP_STR_IN(B, C))
#define GP_COUNT4(A, B, C, D) gp_count99(GP_STR_IN(A, B), GP_STR_IN(C, D))
#define GP_COUNT(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_COUNT4, GP_COUNT3, GP_COUNT2)(A, __VA_ARGS__)

#define GP_CODEPOINT_LENGTH1(PTR)    gp_str_codepoint_length((GPString)(PTR), 0)
#define GP_CODEPOINT_LENGTH2(STR, I) gp_str_codepoint_length((GPString)(STR), I)
#define GP_CODEPOINT_LENGTH(...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_CODEPOINT_LENGTH2, GP_CODEPOINT_LENGTH1)(__VA_ARGS__)

#define GP_CODEPOINT_CLASSIFY2(PTR, F)    gp_str_codepoint_classify((GPString)(PTR), 0, F)
#define GP_CODEPOINT_CLASSIFY3(PTR, I, F) gp_str_codepoint_classify((GPString)(PTR), I, F)
#define GP_CODEPOINT_CLASSIFY(P, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_CODEPOINT_CLASSIFY3, GP_CODEPOINT_CLASSIFY2)(P, __VA_ARGS__)

// ----------------------------------------------------------------------------
// String

inline GPString gp_repeat99(
    const size_t a_size, const void* a, const size_t count, GPStrIn in)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_repeat((GPString*)a, count, in.data, in.length);
        return *(GPString*)a;
    }
    GPString out = gp_str_new(a, count * in.length, "");
    gp_str_repeat(&out, count, in.data, in.length);
    return out;
}
#define GP_REPEAT99(A, COUNT, ...) gp_repeat99(GP_SIZEOF_TYPEOF(*(A)), A, COUNT, GP_STR_IN99(__VA_ARGS__))

GPString gp_replace99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d,
    const size_t start);
#define GP_REPLACE99_3(HAY, NDL, REPL) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN99(NDL), GP_STR_IN99(REPL), GP_STR_IN99(NULL, 0), 0)
#define GP_REPLACE99_4(A, B, C, D) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(A)), A, GP_STR_IN99(B), GP_STR_IN99(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        GP_STR_IN99(NULL, 0) : GP_STR_IN99(D), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        (uintptr_t)(D) : 0)
#define GP_REPLACE99_5(ALC, HAY, NDL, REPL, START) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(HAY), GP_STR_IN99(NDL), GP_STR_IN99(REPL), START)
#define GP_REPLACE99(A, B, ...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_REPLACE99_5, GP_REPLACE99_4, GP_REPLACE99_3)(A, B, __VA_ARGS__)

GPString gp_replace_all99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d);
#define GP_REPLACE_ALL99_3(HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN99(NDL), GP_STR_IN99(REPL), GP_STR_IN99(NULL, 0))
#define GP_REPLACE_ALL99_4(ALC, HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(HAY), GP_STR_IN99(NDL), GP_STR_IN99(REPL))
#define GP_REPLACE_ALL99(A, B, ...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_REPLACE_ALL99_4, GP_REPLACE_ALL99_3)(A, B, __VA_ARGS__)

GPString gp_trim99(
    const size_t a_size, const void* a, GPStrIn b, const char* char_set, int flags);
#define GP_TRIM99_1(STR) gp_str_trim(STR, NULL, 'l' | 'r')
#define GP_TRIM99_2(A, B) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN99(NULL, 0) : GP_STR_IN99(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : NULL, \
    'l' | 'r')
#define GP_TRIM99_3(A, B, C) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN99(NULL, 0) : GP_STR_IN99(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : (char*)(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (intptr_t)(C) : 'l' | 'r')
#define GP_TRIM99_4(ALC, STR, CHARS, FLAGS) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(STR), CHARS, FLAGS)
#define GP_TRIM99(...) \
    GP_OVERLOAD4(__VA_ARGS__, GP_TRIM99_4, GP_TRIM99_3, GP_TRIM99_2, GP_TRIM99_1)(__VA_ARGS__)

GPString gp_to_upper99(const GPAllocator* alc, const GPString str);
#define GP_TO_UPPER1(A)        gp_str_to_upper(A)
#define GP_TO_UPPER2(ALC, STR) gp_to_upper99((GPAllocator*)(ALC), STR)
#define GP_TO_UPPER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_UPPER2, GP_TO_UPPER1)(__VA_ARGS__)

GPString gp_to_lower99(const GPAllocator* alc, const GPString str);
#define GP_TO_LOWER1(A)        gp_str_to_lower(A)
#define GP_TO_LOWER2(ALC, STR) gp_to_lower99((GPAllocator*)(ALC), STR)
#define GP_TO_LOWER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_LOWER2, GP_TO_LOWER1)(__VA_ARGS__)

GPString gp_to_valid99(
    const GPAllocator* alc, const GPString str, const char*const replacement);
#define GP_TO_VALID2(A, REPL)        gp_str_to_valid(A, REPL)
#define GP_TO_VALID3(ALC, STR, REPL) gp_to_valid99((GPAllocator*)(ALC), STR, REPL)
#define GP_TO_VALID(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_VALID3, GP_TO_VALID2)(A,__VA_ARGS__)

inline size_t gp_find_first99(const GPString haystack, GPStrIn needle)
{
    return gp_str_find_first(haystack, needle.data, needle.length, 0);
}
#define GP_FIND_FIRST2(HAY, NDL)                gp_find_first99(HAY, GP_STR_IN(NDL))
#define GP_FIND_FIRST3(HAY, NDL, NDLLEN)        gp_str_find_first(HAY, NDL, NDLLEN, 0)
#define GP_FIND_FIRST(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, gp_str_find_first, GP_FIND_FIRST3, GP_FIND_FIRST2)(A, __VA_ARGS__)

inline size_t gp_find_last99(const GPString haystack, GPStrIn needle)
{
    return gp_str_find_last(haystack, needle.data, needle.length);
}
#define GP_FIND_LAST2(HAY, NDL) gp_find_last99(HAY, GP_STR_IN(NDL))
#define GP_FIND_LAST(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_last, GP_FIND_LAST2)(A, __VA_ARGS__)

#define GP_FIND_FIRST_OF2(HAY, CHARS) gp_str_find_first_of(HAY, CHARS, 0)
#define GP_FIND_FIRST_OF(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_first_of, GP_FIND_FIRST_OF2)(A, __VA_ARGS__)

#define GP_FIND_FIRST_NOT_OF2(HAY, CHARS) gp_str_find_first_not_of(HAY, CHARS, 0)
#define GP_FIND_FIRST_NOT_OF(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_first_not_of, GP_FIND_FIRST_NOT_OF2)(A, __VA_ARGS__)

inline bool gp_equal_case99(const GPString a, GPStrIn b)
{
    return gp_str_equal_case(a, b.data, b.length);
}
#define GP_EQUAL_CASE2(A, B) gp_equal_case99(a, GP_STR_IN(B))
#define GP_EQUAL_CASE(A,...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_equal_case, GP_EQUAL_CASE2)(A, __VA_ARGS__)

size_t gp_codepoint_count99(GPStrIn s);
#define GP_CODEPOINT_COUNT(...) gp_codepoint_count99(GP_STR_IN(__VA_ARGS__))

bool gp_is_valid99(GPStrIn s, size_t*i);
#define GP_IS_VALID1(S)       gp_is_valid99(GP_STR_IN(S),    NULL)
#define GP_IS_VALID2(S, L)    gp_is_valid99(GP_STR_IN(S, L), NULL)
#define GP_IS_VALID3(S, L, I) gp_is_valid99(GP_STR_IN(S, L), I)
#define GP_IS_VALID(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_IS_VALID3, GP_IS_VALID2, GP_IS_VALID1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Strings and arrays

#define GP_IS_ALC99(A) (GP_SIZEOF_TYPEOF(*(A)) >= sizeof(GPAllocator))

void gp_reserve99(size_t elem_size, void* px, const size_t capacity);
#define GP_RESERVE99(A, CAPACITY) gp_reserve99(sizeof**(A), A, CAPACITY)

void* gp_copy99(size_t y_size, const void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size);
#define GP_COPY99_2(A, B) \
    gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY99_3(A, B, C) gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY99(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_COPY99_3, GP_COPY99_2)(A,__VA_ARGS__)

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end);
#define GP_SLICE_WITH_INPUT99(Y, X, START, END) \
    gp_slice99(GP_SIZEOF_TYPEOF(*(Y)), Y, GP_SIZEOF_TYPEOF(*(X)), X, START, END)
#define GP_SLICE_WOUT_INPUT99(Y, START, END) \
    ((void*){0} = gp_arr_slice(sizeof**(Y), *(void**)(Y), NULL, START, END))
#define GP_SLICE99(A, START, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_WITH_INPUT99, GP_SLICE_WOUT_INPUT99)(A, START, __VA_ARGS__)

void* gp_append99(
    const size_t a_size, const void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_APPEND99_2(A, B) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_APPEND99_3(A, B, C) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, GP_IS_ALC99(A) ? #B : NULL, GP_IS_ALC99(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC99(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_APPEND99_4(A, B, C, D) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_APPEND99_5(A, B, C, D, E) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_APPEND99(A, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND99_5, GP_APPEND99_4, GP_APPEND99_3, GP_APPEND99_2)(A, __VA_ARGS__)

void* gp_insert99(
    const size_t a_size, const void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_INSERT99_3(A, POS, B) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_INSERT99_4(A, POS, B, C) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, GP_IS_ALC99(A) ? #B : NULL, GP_IS_ALC99(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC99(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_INSERT99_5(A, POS, B, C, D) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_INSERT99_6(A, POS, B, C, D, E) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_INSERT99(A, POS, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT99_6, GP_INSERT99_5, GP_INSERT99_4, GP_INSERT99_3)(A, POS, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Arrays

#ifdef GP_TYPEOF
inline void* gp_push99(const size_t elem_size, void*_parr)
{
    uint8_t** parr = _parr;
    *parr = gp_arr_reserve(elem_size, *parr, gp_arr_length(*parr) + 1);
    return *parr + elem_size * ((GPArrayHeader*)*parr - 1)->length++;
}
#define GP_PUSH(ARR, ELEM) \
    (*(GP_TYPEOF(*(ARR)))gp_push99(sizeof(**(ARR) = (ELEM)), (ARR)) = (ELEM))
#else
#define GP_PUSH(ARR, ELEM) ( \
    ((GPArrayHeader*)*(ARR) - 1)->length++, (*(ARR))[gp_length(*(ARR)) - 1] = (ELEM))
#endif

#ifdef GP_TYPEOF
#define GP_POP(ARR) (*(GP_TYPEOF(*(ARR)))gp_arr_pop(GP_SIZEOF_TYPEOF(**(ARR)), *(ARR)))
#else
#define GP_POP(ARR) (gp_arr_pop(sizeof(**(ARR)), *(ARR)), (*(ARR))[gp_arr_length(*(ARR))])
#endif

#define GP_ERASE2(ARR, POS)        ((void*){0} = gp_arr_erase(sizeof**(ARR), *(ARR), POS, 1))
#define GP_ERASE3(ARR, POS, COUNT) ((void*){0} = gp_arr_erase(sizeof**(ARR), *(ARR), POS, COUNT))
#define GP_ERASE(A,...) GP_OVERLOAD2(__VA_ARGS__, GP_ERASE3, GP_ERASE2)(A,__VA_ARGS__)

GPArray(void) gp_map99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    void(*f)(void*,const void*));
#define GP_MAP99_2(ARR, F) \
    gp_arr_map(sizeof**((F)(*(ARR),*(ARR)),(ARR)), *(ARR), NULL, 0, (void(*)(void*,const void*))(F))
#define GP_MAP99_3(A, SRC, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), SRC)), GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_MAP99_4(A, SRC, SRC_LENGTH, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), *(SRC))), (void(*)(void*,const void*))(F))
#define GP_MAP99(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_MAP99_4, GP_MAP99_3, GP_MAP99_2)(A,__VA_ARGS__)

#ifdef GP_TYPEOF // better type safety and allow using integer accumulator
#define GP_FOLD(ARR, ACC, F) \
    (GP_TYPEOF(ACC))(uintptr_t)gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#define GP_FOLDR(ARR, ACC, F) \
    (GP_TYPEOF(ACC))(uintptr_t)gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#else
#define GP_FOLD(ARR, ACC, F)  gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#define GP_FOLDR(ARR, ACC, F) gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#endif

GPArray(void) gp_filter99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    bool(*f)(const void* element));
#define GP_FILTER99_2(ARR, F) ((void*){0} = \
    gp_arr_filter(sizeof**((bool){0} = (F)(*(ARR)), (ARR)), *(ARR), NULL, 0, (bool(*)(const void*))(F)))
#define GP_FILTER99_3(A, SRC, F) gp_filter99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(((bool){0} = (F)(SRC), SRC)), GP_SIZEOF_TYPEOF(*(SRC)), (bool(*)(const void*))(F))
#define GP_FILTER99_4(A, SRC, SRC_LENGTH, F) gp_filter99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(*((bool){0} = (F)(SRC), SRC)), (bool(*)(const void*))(F))
#define GP_FILTER99(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FILTER99_4, GP_FILTER99_3, GP_FILTER99_2)(A,__VA_ARGS__)

// ----------------------------------------------------------------------------
// Dictionarys

#ifdef GP_TYPEOF
inline void* gp_put99(GPHashMap* dict, GPStrIn key)
{
    return gp_hash_map_put(dict, key.data, key.length, NULL);
}
#define GP_PUT_ELEM(DICT, ELEM, ...) ( \
    *(GP_TYPEOF(*(DICT)))(gp_put99((GPHashMap*)*(DICT), GP_STR_IN99(__VA_ARGS__))) = (ELEM))
#else
#define GP_PUT_ELEM(DICT, ELEM, ...) do \
{ \
    void* _gp_dict = *(DICT); \
    GPStrIn _gp_key = GP_STR_IN99(__VA_ARGS__); \
     *(DICT) = gp_hash_map_put(_gp_dict, _gp_key.data, _gp_key.length, NULL); \
    **(DICT) = (ELEM); \
     *(DICT) = _gp_dict; \
} while(0)
#endif
#define GP_PUT3(DICT, KEY, ELEM)             GP_PUT_ELEM(DICT, ELEM, KEY)
#define GP_PUT4(DICT, KEY, KEY_LENGTH, ELEM) GP_PUT_ELEM(DICT, ELEM, KEY, KEY_LENGTH)
#define GP_PUT(A, B, ...) GP_OVERLOAD2(__VA_ARGS__, GP_PUT4, GP_PUT3)(A, B,__VA_ARGS__)

GP_NONNULL_ARGS(1)
inline void* gp_get99(void* map, GPStrIn key)
{
    return gp_hash_map_get(map, key.data, key.length);
}

#ifdef GP_TYPEOF
#define GP_GET(DICT, ...) ((GP_TYPEOF(DICT))gp_get99(DICT, GP_STR_IN99(__VA_ARGS__)))
#else
#define GP_GET(DICT, ...) gp_get99(DICT, GP_STR_IN99(__VA_ARGS__))
#endif

inline bool gp_remove99(GPHashMap* dict, GPStrIn key)
{
    return gp_hash_map_remove(dict, key.data, key.length);
}
#define GP_REMOVE(DICT, ...) gp_remove99((GPHashMap*)*(DICT), GP_STR_IN99(__VA_ARGS__))

// ----------------------------------------------------------------------------
// Allocators

#define GP_ALLOC(ALLOCATOR, SIZE) gp_mem_alloc(GP_ALC(ALLOCATOR), SIZE)

#define GP_ALLOC_TYPE_WITH_COUNT(ALLOCATOR, TYPE, COUNT) \
    gp_mem_alloc(GP_ALC(ALLOCATOR), (COUNT) * sizeof(TYPE))
#define GP_ALLOC_TYPE_WOUT_COUNT(ALLOCATOR, TYPE) \
    gp_mem_alloc(GP_ALC(ALLOCATOR), sizeof(TYPE))
#define GP_ALLOC_TYPE(ALC, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_ALLOC_TYPE_WITH_COUNT,GP_ALLOC_TYPE_WOUT_COUNT)(ALC, __VA_ARGS__)

#define GP_ALLOC_ZEROES(ALLOCATOR, SIZE) \
    gp_mem_alloc_zeroes(GP_ALC(ALLOCATOR), SIZE)

#define GP_DEALLOC(ALLOCATOR, BLOCK) \
    gp_mem_dealloc(GP_ALC(ALLOCATOR), (BLOCK))

#define GP_REALLOC(ALLOCATOR, ...) \
    gp_mem_realloc(GP_ALC(ALLOCATOR),__VA_ARGS__)

// ----------------------------------------------------------------------------
// File

inline GPString gp_file99(size_t a_size, void* a, const char* path, const char* mode)
{
    switch (a_size)
    {
        case sizeof(GPChar): // read a, write to path
            if (gp_str_file((GPString*)&a, path, mode) == 0)
                return a;
            break;

        case sizeof(GPString): // read from path, write to a
            if (gp_str_file(a, path, mode) == 0)
                return *(GPString*)a;
            break;

        GPString str;
        default: // read from path to a new string
            str = gp_str_new(a, 128, "");
            if (gp_str_file(&str, path, mode) == 0)
                return str;
            break;
    }
    return NULL;
}
#define GP_FILE99_3(A, ...) gp_file99(GP_SIZEOF_TYPEOF(*(A)), A, __VA_ARGS__)
#define GP_FILE99_2(PATH, ...) gp_file_open(PATH, __VA_ARGS__)
#define GP_FILE99(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE99_3, GP_FILE99_2)(A,__VA_ARGS__)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_GENERIC_INCLUDED
