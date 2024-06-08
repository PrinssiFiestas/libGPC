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

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// TODO implement what's not implemented

// Note: macros may take variadic arguments even when not necessary for better
// error messages. Also in some occasions, but not always, allows using
// compound literals as macros arguments.

#define GPHashMap(T) T*

// Constructors
#define gp_arr(...)                GP_ARR_NEW(__VA_ARGS__)
#define gp_str(...)                GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)

// Bytes and strings
#define gp_equal(...)              GP_EQUAL(__VA_ARGS__)
#define gp_count(...)              GP_COUNT(__VA_ARGS__)
#define gp_codepoint_length(...)   GP_CODEPOINT_LENGTH(__VA_ARGS__)
#define gp_codepoint_classify(...) GP_CODEPOINT_CLASSIFY(__VA_ARGS__)

// Strings
#define gp_repeat(...)             GP_REPEAT(__VA_ARGS__)
#define gp_replace(...)            GP_REPLACE(__VA_ARGS__)
#define gp_replace_all(...)        GP_REPLACE_ALL(__VA_ARGS__)
#define gp_trim(...)               GP_TRIM(__VA_ARGS__)
#define gp_to_upper(...)           GP_TO_UPPER(__VA_ARGS__)
#define gp_to_lower(...)           GP_TO_LOWER(__VA_ARGS__)
#define gp_to_valid(...)           GP_TO_VALID(__VA_ARGS__)
#define gp_find_first(...)         GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)          GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)      GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)  GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)         GP_EQUAL_CASE(__VA_ARGS__)
#define gp_codepoint_count(...)    GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)           GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
#define gp_length(...)             gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)           gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)         gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)          gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)            GP_RESERVE(__VA_ARGS__)
#define gp_copy(...)               GP_COPY(__VA_ARGS__)
#define gp_slice(...)              GP_SLICE(__VA_ARGS__)
#define gp_append(...)             GP_APPEND(__VA_ARGS__)
#define gp_insert(...)             GP_INSERT(__VA_ARGS__)

// Arrays
#define gp_map(...)                GP_MAP(__VA_ARGS__)
#define gp_fold(ARR, ACC, F)       (typeof(ACC))(uintptr_t)gp_arr_fold (sizeof*(ARR),ARR,(void*)(ACC),(void*)(F))
#define gp_foldr(ARR, ACC, F)      (typeof(ACC))(uintptr_t)gp_arr_foldr(sizeof*(ARR),ARR,(void*)(ACC),(void*)(F))
#define gp_filter(...)             GP_FILTER(__VA_ARGS__)

// Arrays and hash maps
#define gp_at(...)
#define gp_push(...)
#define gp_pop(...)
#define gp_remove(...)

// Memory
#define gp_alloc(...)        GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)   GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...) GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)      GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)      GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)         GP_FILE(__VA_ARGS__)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


// Currently C99 compliant, but later on C11 _Generic() selection should be used
// so any char* could be passed as string inputs insead of just literals. This
// would have better type safety too.

#ifdef __GNUC__
// Suppress suspicious usage of sizeof warning.
#define GP_SIZEOF_TYPEOF(X) sizeof(typeof(X))
#else
#define GP_SIZEOF_TYPEOF(X) sizeof(X)
#endif

// ----------------------------------------------------------------------------
// Constructors

static inline GPArray(void) gp_arr99(const GPAllocator* alc,
    const size_t elem_size, const void*const init, const size_t init_length)
{
    GPArray(void) out = gp_arr_new(alc, elem_size, init_length > 4 ? init_length : 4);
    ((GPArrayHeader*)out - 1)->length = init_length;
    return memcpy(out, init, elem_size * init_length);
}
#define GP_ARR_NEW(ALC, TYPE, ...) (TYPE*)gp_arr99( \
    (GPAllocator*)(ALC), \
    sizeof(TYPE), (TYPE[]){__VA_ARGS__}, sizeof((TYPE[]){__VA_ARGS__}) / sizeof(TYPE))

struct gp_str_maker { const GPAllocator* allocator; const char* init; };
GPString gp_str_make(struct gp_str_maker maker);
#define GP_STR_NEW(ALLOCATOR, ...) \
    gp_str_make((struct gp_str_maker){(GPAllocator*)(ALLOCATOR), __VA_ARGS__})

// ----------------------------------------------------------------------------
// Bytes and strings

typedef struct gp_str_in { const void* data; const size_t length; } GPStrIn;
static inline GPStrIn gp_str_in99(const void* data, const size_t length)
{
    return (GPStrIn) {
        .data   = data,
        .length = length != SIZE_MAX ? length : gp_arr_length(data)
    };
}
#define GP_STR_IN1(A) gp_str_in99( \
    (void*)(A), #A[0] == '"' ? GP_SIZEOF_TYPEOF(A) - sizeof "" : SIZE_MAX)
#define GP_STR_IN(...) GP_OVERLOAD2(__VA_ARGS__, gp_str_in99, GP_STR_IN1)(__VA_ARGS__)

static inline bool gp_equal99(const GPString a, GPStrIn b) {
    return gp_bytes_equal(a, gp_str_length(a), b.data, b.length);
}
#define GP_EQUAL2(A, B)               gp_equal99(A, GP_STR_IN(B))
#define GP_EQUAL3(A, B, B_LENGTH)     gp_str_equal(A, B, B_LENGTH)
#define GP_EQUAL4(A, A_LEN, B, B_LEN) gp_bytes_equal(A, A_LEN, B, B_LEN)
#define GP_EQUAL(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_EQUAL4, GP_EQUAL3, GP_EQUAL2)(A, __VA_ARGS__)

static inline size_t gp_count99(GPStrIn haystack, GPStrIn needle) {
    return gp_bytes_count(haystack.data, haystack.length, needle.data, needle.length);
}
#define GP_COUNT2(A, B)       gp_count99(GP_STR_IN(A), GP_STR_IN(B))
#define GP_COUNT3(A, B, C)    gp_count99(GP_STR_IN(A), GP_STR_IN(B, C))
#define GP_COUNT4(A, B, C, D) gp_count99(GP_STR_IN(A, B), GP_STR_IN(C, D))
#define GP_COUNT(A, ...) GP_OVERLOAD3(__VA_ARGS__, GP_COUNT4, GP_COUNT3, GP_COUNT2)(A, __VA_ARGS__)

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

GPString gp_repeat99(
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
#define GP_REPEAT(A, COUNT, ...) gp_repeat99(GP_SIZEOF_TYPEOF(*(A)), A, COUNT, GP_STR_IN(__VA_ARGS__))

static inline GPString gp_replace99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d,
    const size_t start)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_replace((GPString*)a, b.data, b.length, c.data, c.length, start);
        return *(GPString*)a;
    }
    GPString out = gp_str_new(a, b.length + c.length + d.length, "");
    const size_t pos = gp_bytes_find_first(b.data, b.length, c.data, c.length, start);
    if (pos == GP_NOT_FOUND) {
        memcpy(out, b.data, b.length);
        ((GPStringHeader*)out - 1)->length = b.length;
    } else {
        memcpy(out, b.data, pos);
        memcpy(out + pos, d.data, d.length);
        memcpy(out + pos + d.length, b.data + pos + c.length, b.length - c.length);
        ((GPStringHeader*)out - 1)->length = b.length + d.length - c.length;
    }
    return out;
}
#define GP_REPLACE3(HAY, NDL, REPL) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN(NDL), GP_STR_IN(REPL), GP_STR_IN(NULL, 0), 0)
#define GP_REPLACE4(A, B, C, D) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(A)), A, GP_STR_IN(B), GP_STR_IN(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        GP_STR_IN(NULL, 0) : GP_STR_IN(D), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        (uintptr_t)(D) : 0)
#define GP_REPLACE5(ALC, HAY, NDL, REPL, START) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN(HAY), GP_STR_IN(NDL), GP_STR_IN(REPL), START)
#define GP_REPLACE(A, B, ...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_REPLACE5, GP_REPLACE4, GP_REPLACE3)(A, B, __VA_ARGS__)

static inline GPString gp_replace_all99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_replace_all((GPString*)a, b.data, b.length, c.data, c.length);
        return *(GPString*)a;
    }
    // TODO don't copy and replace all, just copy what's needed
    GPString out = gp_str_new(a, b.length, "");
    gp_str_copy(&out, b.data, b.length);
    gp_str_replace_all(&out, c.data, c.length, d.data, d.length);
    return out;
}
#define GP_REPLACE_ALL3(HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN(NDL), GP_STR_IN(REPL), GP_STR_IN(NULL, 0))
#define GP_REPLACE_ALL4(ALC, HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN(HAY), GP_STR_IN(NDL), GP_STR_IN(REPL))
#define GP_REPLACE_ALL(A, B, ...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_REPLACE_ALL4, GP_REPLACE_ALL3)(A, B, __VA_ARGS__)

static inline GPString gp_trim99(
    const size_t a_size, const void* a, GPStrIn b, const char* char_set, int flags)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_trim((GPString*)a, char_set, flags);
        return *(GPString*)a;
    }
    GPString out = gp_str_new(a, b.length, "");
    // TODO don't copy and trim, just copy what's needed!
    gp_str_copy(&out, b.data, b.length);
    gp_str_trim(&out, char_set, flags);
    return out;
}
#define GP_TRIM1(STR) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(STR)), STR, GP_STR_IN(NULL, 0), NULL, 'l' | 'r')
#define GP_TRIM2(A, B) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN(NULL, 0) : GP_STR_IN(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : NULL, \
    'l' | 'r')
#define GP_TRIM3(A, B, C) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN(NULL, 0) : GP_STR_IN(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : (char*)(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (intptr_t)(C) : 'l' | 'r')
#define GP_TRIM4(ALC, STR, CHARS, FLAGS) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN(STR), CHARS, FLAGS)
#define GP_TRIM(...) \
    GP_OVERLOAD4(__VA_ARGS__, GP_TRIM4, GP_TRIM3, GP_TRIM2, GP_TRIM1)(__VA_ARGS__)

static inline GPString gp_to_upper99(const GPAllocator* alc, const GPString str)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_upper(&out);
    return out;
}
#define GP_TO_UPPER1(A)        gp_str_to_upper(A)
#define GP_TO_UPPER2(ALC, STR) gp_to_upper99((GPAllocator*)(ALC), STR)
#define GP_TO_UPPER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_UPPER2, GP_TO_UPPER1)(__VA_ARGS__)

static inline GPString gp_to_lower99(const GPAllocator* alc, const GPString str)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_lower(&out);
    return out;
}
#define GP_TO_LOWER1(A)        gp_str_to_lower(A)
#define GP_TO_LOWER2(ALC, STR) gp_to_lower99((GPAllocator*)(ALC), STR)
#define GP_TO_LOWER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_LOWER2, GP_TO_LOWER1)(__VA_ARGS__)

static inline GPString gp_to_valid99(
    const GPAllocator* alc, const GPString str, const char*const replacement)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_valid(&out, replacement);
    return out;
}
#define GP_TO_VALID2(A, REPL)        gp_str_to_valid(A, REPL)
#define GP_TO_VALID3(ALC, STR, REPL) gp_to_valid99((GPAllocator*)(ALC), STR, REPL)
#define GP_TO_VALID(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_VALID3, GP_TO_VALID2)(A,__VA_ARGS__)

static inline size_t gp_find_first99(const GPString haystack, GPStrIn needle)
{
    return gp_str_find_first(haystack, needle.data, needle.length, 0);
}
#define GP_FIND_FIRST2(HAY, NDL)                gp_find_first99(HAY, GP_STR_IN(NDL))
#define GP_FIND_FIRST3(HAY, NDL, NDLLEN)        gp_str_find_first(HAY, NDL, NDLLEN, 0)
#define GP_FIND_FIRST(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, gp_str_find_first, GP_FIND_FIRST3, GP_FIND_FIRST2)(A, __VA_ARGS__)

static inline size_t gp_find_last99(const GPString haystack, GPStrIn needle)
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

static inline bool gp_equal_case99(const GPString a, GPStrIn b)
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

#define GP_IS_ALC(A) (GP_SIZEOF_TYPEOF(*(A)) >= sizeof(GPAllocator))

void gp_reserve99(size_t elem_size, void* px, const size_t capacity);
#define GP_RESERVE(A, CAPACITY) gp_reserve99(sizeof**(A), A, CAPACITY)

void* gp_copy99(size_t y_size, void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size);
#define GP_COPY2(A, B) \
    gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY3(A, B, C) gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_COPY3, GP_COPY2)(A,__VA_ARGS__)

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end);
#define GP_SLICE_WITH_INPUT(Y, X, START, END) \
    gp_slice99(GP_SIZEOF_TYPEOF(*(Y)), Y, GP_SIZEOF_TYPEOF(*(X)), X, START, END)
#define GP_SLICE_WOUT_INPUT(Y, START, END) \
    ((void*){0} = gp_arr_slice(sizeof**(Y), *(void**)(Y), NULL, START, END))
#define GP_SLICE(A, START, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_WITH_INPUT, GP_SLICE_WOUT_INPUT)(A, START, __VA_ARGS__)

void* gp_append99(
    const size_t a_size, void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_APPEND2(A, B) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_APPEND3(A, B, C) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_APPEND4(A, B, C, D) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_APPEND5(A, B, C, D, E) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_APPEND(A, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND5, GP_APPEND4, GP_APPEND3, GP_APPEND2)(A, __VA_ARGS__)

void* gp_insert99(
    const size_t a_size, void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_INSERT3(A, POS, B) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_INSERT4(A, POS, B, C) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_INSERT5(A, POS, B, C, D) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_INSERT6(A, POS, B, C, D, E) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_INSERT(A, POS, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT6, GP_INSERT5, GP_INSERT4, GP_INSERT3)(A, POS, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Arrays

GPArray(void) gp_map99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    void(*f)(void*,const void*));
#define GP_MAP2(ARR, F) \
    gp_arr_map(sizeof**(ARR), *(ARR), NULL, 0, (void(*)(void*,const void*))(F))
#define GP_MAP3(A, SRC, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(SRC), GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_MAP4(A, SRC, SRC_LENGTH, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_MAP(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_MAP4, GP_MAP3, GP_MAP2)(A,__VA_ARGS__)

GPArray(void) gp_filter99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    bool(*f)(const void* element));
#define GP_FILTER2(ARR, F) \
    gp_arr_map(sizeof**(ARR), *(ARR), NULL, 0, (void(*)(void*,const void*))(F))
#define GP_FILTER3(A, SRC, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(SRC), GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_FILTER4(A, SRC, SRC_LENGTH, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_FILTER(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FILTER4, GP_FILTER3, GP_FILTER2)(A,__VA_ARGS__)

// ----------------------------------------------------------------------------
// Allocators

#define GP_ALLOC(ALLOCATOR, SIZE) gp_mem_alloc((GPAllocator*)(ALLOCATOR), SIZE)

#define GP_ALLOC_TYPE_WITH_COUNT(ALLOCATOR, TYPE, COUNT) \
    gp_mem_alloc((GPAllocator*)(ALLOCATOR), (COUNT) * sizeof(TYPE))
#define GP_ALLOC_TYPE_WOUT_COUNT(ALLOCATOR, TYPE) \
    gp_mem_alloc((GPAllocator*)(ALLOCATOR), sizeof(TYPE))
#define GP_ALLOC_TYPE(ALC, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_ALLOC_TYPE_WITH_COUNT,GP_ALLOC_TYPE_WOUT_COUNT)(ALC, __VA_ARGS__)

#define GP_ALLOC_ZEROES(ALLOCATOR, SIZE, COUNT) \
    gp_mem_alloc_zeroes((GPAllocator*)(ALLOCATOR), (SIZE) * sizeof(TYPE))

#define GP_DEALLOC(ALLOCATOR, BLOCK) \
    gp_mem_dealloc((GPAllocator*)(ALLOCATOR), (BLOCK))

#define GP_REALLOC(ALLOCATOR, ...) \
    gp_mem_realloc((GPAllocator*)(ALLOCATOR),__VA_ARGS__)

// ----------------------------------------------------------------------------
// File

static inline GPString gp_file99(size_t a_size, void* a, const char* path, const char* mode)
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
#define GP_FILE3(A, ...) gp_file99(GP_SIZEOF_TYPEOF(*(A)), A, __VA_ARGS__)

#define GP_FILE2(PATH, ...) gp_file_open(PATH, __VA_ARGS__)
#define GP_FILE(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE3, GP_FILE2)(A,__VA_ARGS__)

#endif // GP_GENERIC_INCLUDED
