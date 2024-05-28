// Script generated single header library.

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               memory.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Memory management and allocators
 */

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED

#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aligment of all pointers returned by any valid allocators
#ifndef GP_UTILS_INCLUDED
#if __STDC_VERSION__ >= 201112L
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

//
typedef struct gp_allocator
{
    void* (*alloc)  (const struct gp_allocator*, size_t block_size);
    void  (*dealloc)(const struct gp_allocator*, void*  block);
} GPAllocator;

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_MALLOC_SIZE(2)
inline void* gp_mem_alloc(
    const GPAllocator* allocator,
    size_t size)
{
    return allocator->alloc(allocator, size);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_MALLOC_SIZE(2)
inline void* gp_mem_alloc_zeroes(
    const GPAllocator* allocator,
    size_t size)
{
    void* memset(void*, int, size_t);
    return memset(gp_mem_alloc(allocator, size), 0, size);
}

GP_NONNULL_ARGS(1)
inline void gp_mem_dealloc(
    const GPAllocator* allocator,
    void* block)
{
    if (block != NULL)
        allocator->dealloc(allocator, block);
}

GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
void* gp_mem_realloc(
    const GPAllocator* allocator,
    void*  optional_old_block,
    size_t old_size,
    size_t new_size);

#define gp_alloc(allocator, type, count) \
    gp_mem_alloc((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_alloc_zeroes(allocator, type, count) \
    gp_mem_alloc_zeroes((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_dealloc(allocator, optional_block) \
    gp_mem_dealloc((GPAllocator*)(allocator), (optional_block))

#define gp_realloc(allocator, optional_block, old_capacity, new_capacity) \
    gp_mem_realloc( \
        (GPAllocator*)(allocator), \
        optional_block, \
        old_capacity, \
        new_capacity)

// ----------------------------------------------------------------------------
// Scope allocator

// Create thread local scope.
GPAllocator* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

// End scope and any inner scopes that have not been ended.
void gp_end(GPAllocator* optional_scope);

// Deferred functions are called in Last In First Out order in gp_end().
void gp_defer(GPAllocator* scope, void (*f)(void* arg), void* arg)
    GP_NONNULL_ARGS(1, 2);

// Get lastly created scope in callbacks. You should prefer to just pass scopes
// as arguments when possible.
GPAllocator* gp_last_scope(GPAllocator* return_this_if_no_scopes);

// ----------------------------------------------------------------------------
// Arena allocator

// Arena that does not run out of memory. This is achieved by creating new
// arenas when old one gets full.
typedef struct gp_arena GPArena;

// growth_coefficient determines how large each subsequent arena in arena list
// is relative to previous arena when the previous arena gets full.
GPArena gp_arena_new(size_t capacity) GP_NODISCARD;
void gp_arena_delete(GPArena* optional);
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Heap allocator

#ifdef NDEBUG
/** malloc() based allocator. */
extern const GPAllocator*const gp_heap;
#else // heap allocator can be overridden for debugging
/** malloc() based allocator. */
extern const GPAllocator* gp_heap;
#endif


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
struct gp_arena
{
    GPAllocator allocator;
    struct gp_arena_node* head; // also contains arenas memory block
    size_t capacity;
};

#endif // GP_MEMORY_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               io.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_IO_INCLUDED
#define GP_IO_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

#if _WIN32
typedef struct __stat64 GPStat;
#elif _GNU_SOURCE
typedef struct stat64 GPStat;
#else // 64-bit in 64-bit Linux
typedef struct stat GPStat;
#endif

GP_NONNULL_ARGS() GP_NODISCARD
inline int gp_stat(GPStat* s, const char* path)
{
    #if _WIN32
    return _stat64(path, s);
    #elif _GNU_SOURCE
    return stat64(path, s);
    #else
    return stat(path, s);
    #endif
}

#define/* size_t */gp_print(...) \
    GP_FILE_PRINT(stdout, __VA_ARGS__)

#define/* size_t */gp_println(...) \
    GP_FILE_PRINTLN(stdout, __VA_ARGS__)

#define/* size_t */gp_file_print(FILE_ptr, ...) \
    GP_FILE_PRINT(FILE_ptr, __VA_ARGS__)

#define/* size_t */gp_file_println(FILE_ptr, ...) \
    GP_FILE_PRINTLN(FILE_ptr, __VA_ARGS__)

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

size_t gp_file_print_internal(
    FILE* file,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_file_println_internal(
    FILE* file,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#define GP_FILE_PRINT(OUT, ...) \
    gp_file_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_FILE_PRINTLN(OUT, ...) \
    gp_file_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_IO_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               string.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type.
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
typedef struct gp_char { uint8_t c; } GPChar;
typedef struct gp_string_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPStringHeader;

typedef GPChar* GPString;

GPString gp_str_new(
    const       GPAllocator*,
    size_t      capacity,
    const char* init) GP_NONNULL_ARGS_AND_RETURN;

#define/* GPString */gp_str_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    const_char_ptr_init) (GPString) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPStringHeader header; char data[ (size_t_capacity) + sizeof"" ]; }) { \
{ \
    .length     = sizeof(const_char_ptr_init) - 1, \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, { const_char_ptr_init } }.data

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. Capacity should be
// initialized to be actual capacity -1 for null termination.
/*
    struct optional_name{ GPStringHeader header; GPChar data[2048]; }my_str_mem;
    my_str_mem.header = (GPStringHeader) {.capacity = 2048 - sizeof"" };
    GPString my_string = my_str_mem.data;
*/

// Passing strings on stack is safe too.
void gp_str_delete(GPString optional_string);

const char* gp_cstr(GPString) GP_NONNULL_ARGS_AND_RETURN;

size_t             gp_str_length    (GPString) GP_NONNULL_ARGS();
size_t             gp_str_capacity  (GPString) GP_NONNULL_ARGS();
void*              gp_str_allocation(GPString) GP_NONNULL_ARGS();
const GPAllocator* gp_str_allocator (GPString) GP_NONNULL_ARGS();

void gp_str_reserve(
    GPString* str,
    size_t       capacity) GP_NONNULL_ARGS();

void gp_str_copy(
    GPString*           dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_repeat(
    GPString*           dest,
    size_t              count,
    const void*restrict src,
    size_t              src_length) GP_NONNULL_ARGS();

void gp_str_slice(
    GPString*           dest,
    const void*restrict optional_src, // mutates dest if NULL
    size_t              src_start,
    size_t              src_end) GP_NONNULL_ARGS(1);

void gp_str_append(
    GPString*           dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_insert(
    GPString*           dest,
    size_t              pos,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

// Returns index to the first occurrence of needle in haystack.
size_t gp_str_replace(
    GPString*           haystack,
    const void*restrict needle,
    size_t              needle_length,
    const void*restrict replacement,
    size_t              replacement_length,
    size_t              start) GP_NONNULL_ARGS();

// Returns number of replacements made.
size_t gp_str_replace_all(
    GPString*           haystack,
    const void*restrict needle,
    size_t              needle_length,
    const void*restrict replacement,
    size_t              replacement_length) GP_NONNULL_ARGS();

#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(str_ptr_out, __VA_ARGS__)

#define/* size_t */gp_str_n_print(str_ptr_out, n, ...) \
    GP_STR_N_PRINT(str_ptr_out, n, __VA_ARGS__)

#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINTLN(str_ptr_out, __VA_ARGS__)

#define/* size_t */gp_str_n_println(str_ptr_out, n, ...) \
    GP_STR_N_PRINTLN(str_ptr_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
// Trims whitespace if char_set is NULL.
void gp_str_trim(
    GPString*   str,
    const char* optional_char_set,
    int         flags) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_upper(
    GPString* str) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_lower(
    GPString* str) GP_NONNULL_ARGS();

void gp_str_to_valid(
    GPString*   str,
    const char* replacement) GP_NONNULL_ARGS();

// Returns 0 on success.
// Returns -1 if file operations fail. Check errno for the specific error.
// Returns  1 if file size > SIZE_MAX in 32-bit systems.
int gp_str_from_path(
    GPString*   str,
    const char* file_path) GP_NONNULL_ARGS() GP_NODISCARD;

// ----------------------------------------------------------------------------
// String examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_str_find(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start) GP_NONNULL_ARGS();

size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

size_t gp_str_count(
    GPString    haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

bool gp_str_equal(
    GPString    s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

// Locale dependent!
bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

size_t gp_str_codepoint_count(
    GPString str) GP_NONNULL_ARGS();

bool gp_str_is_valid(
    GPString str,
    size_t*  optional_invalid_position) GP_NONNULL_ARGS(1);

size_t gp_str_codepoint_length(
    GPString str) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#ifdef _MSC_VER
// unnamed struct in parenthesis in gp_str_on_stack()
#pragma warning(disable : 4116)
#endif

size_t gp_str_print_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_print_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_println_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_println_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#define GP_STR_PRINT(OUT, ...) \
    gp_str_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINT(OUT, N, ...) \
    gp_str_n_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_PRINTLN(OUT, ...) \
    gp_str_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINTLN(OUT, N, ...) \
    gp_str_n_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_STRING_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               hashmap.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file hashmap.h
 * @brief Dictionary data structure
 */

#ifndef GPC_HASHMAP_INCLUDED
#define GPC_HASHMAP_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

typedef struct gp_map      GPMap;
typedef struct gp_hash_map GPHashMap;

// Optional attributes
typedef struct gp_map_initializer
{
    // If 0, elements are assumed to be pointers.
    size_t element_size;

    // Should be a power of 2. Defaults to 256
    size_t capacity;

    // If element_size != 0, argument is pointer to the element, else argument
    // is the actual pointer. In the latter case an example of a valid
    // destructor is free().
    void (*destructor)(void* element);
} GPMapInitializer;

// ------------------
// 128-bit uint

union gp_endianness_detector
{
    uint16_t u16;
    struct { uint8_t is_little; uint8_t is_big; } endianness;
};
extern const union gp_endianness_detector GP_INTEGER; // = {.u16 = 1 }

typedef union gp_uint128
{
    struct {
        uint64_t lo;
        uint64_t hi;
    } little_endian;

    struct {
        uint64_t hi;
        uint64_t lo;
    } big_endian;

    #if __GNUC__ && __SIZEOF_INT128__
    __uint128_t u128;
    #endif
} GPUint128;

GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_lo(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.lo : &u->big_endian.lo);
}
GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_hi(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.hi : &u->big_endian.hi);
}

// ------------------
// Hash map

GPHashMap* gp_hash_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional) GP_NONNULL_ARGS(1) GP_NONNULL_RETURN;

void gp_hash_map_delete(GPHashMap*);

void gp_hash_map_set(
    GPHashMap*,
    const void* key,
    size_t      key_size,
    const void* value) GP_NONNULL_ARGS();

// Returns NULL if not found
void* gp_hash_map_get(
    GPHashMap*,
    const void* key,
    size_t      key_size) GP_NONNULL_ARGS();

bool gp_hash_map_remove(
    GPHashMap*,
    const void* key,
    size_t      key_size) GP_NONNULL_ARGS();

// ------------------
// Non-hashed map

GPMap* gp_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional) GP_NONNULL_ARGS(1) GP_NONNULL_RETURN;

void gp_map_delete(GPMap*);

void gp_map_set(
    GPMap*,
    GPUint128   key,
    const void* value) GP_NONNULL_ARGS();

// Returns NULL if not found
void* gp_map_get(
    GPMap*,
    GPUint128 key) GP_NONNULL_ARGS();

// Returns false if not found
bool gp_map_remove(
    GPMap*,
    GPUint128 key) GP_NONNULL_ARGS();

// ------------------
// Hashing

uint32_t  gp_bytes_hash32 (const void* key, size_t key_size) GP_NONNULL_ARGS();
uint64_t  gp_bytes_hash64 (const void* key, size_t key_size) GP_NONNULL_ARGS();
GPUint128 gp_bytes_hash128(const void* key, size_t key_size) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#endif // GPC_HASHMAP_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               utils.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file string.h
 * @brief General purpose utilities
 */

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED 1

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <signal.h>

#ifdef _WIN32
#include <intrin.h>
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aligment of all pointers returned by any valid allocators
#ifndef GP_MEMORY_INCLUDED
#if __STDC_VERSION__ >= 201112L
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

/** Check and limit upper and lower bounds at once.
 * @p end will be limited to @p limit and @p start will be limited to @p end and
 * @p limit.
 * @return true if arguments are in bounds and @p end > @p start.
 */
bool gp_check_bounds(
    size_t* optional_start_non_inclusive,
    size_t* optional_end_inclusive,
    size_t  limit);

/** Round number up to the next power of 2.
 * Used to compute array sizes on reallocations, thus size_t.
 */
size_t   gp_next_power_of_2   (size_t);
uint32_t gp_next_power_of_2_32(uint32_t);
uint64_t gp_next_power_of_2_64(uint64_t);

inline uintptr_t gp_round_to_aligned(const uintptr_t x)
{
    return x + (GP_ALLOC_ALIGNMENT - 1) - ((x - 1) % GP_ALLOC_ALIGNMENT);
}

#define gp_min(x, y) gp_generic_min(x, y)
#define gp_max(x, y) gp_generic_max(x, y)

inline bool gp_fapproxf(float x, float y, float max_relative_diff) {
    return fabsf(x - y) <= max_relative_diff * fmaxf(x, y);
}
inline bool gp_fapprox(double x, double y, double max_relative_diff) {
    return fabs(x - y) <= max_relative_diff * fmax(x, y);
}
inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff){
    return fabsl(x - y) <= max_rel_diff * fmaxl(x, y);
}

/** Compare raw memory. */
inline bool gp_mem_equal(
    const void* lhs,
    const void* rhs,
    size_t lhs_size,
    size_t rhs_size)
{
    if (lhs_size != rhs_size)
        return false;
    int memcmp(const void*, const void*, size_t);
    return memcmp(lhs, rhs, lhs_size) == 0;
}

#define GP_BREAKPOINT            GP_BREAKPOINT_INTERNAL

// ----------------------------------------------------------------------------
// Random functions. Only work for 64 bit platforms for now. // TODO

typedef struct gp_random_state GPRandomState;

GPRandomState gp_new_random_state(uint64_t seed);
uint32_t gp_random      (GPRandomState*);
double   gp_frandom     (GPRandomState*);
int32_t  gp_random_range(GPRandomState*, int32_t min, int32_t max);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

struct gp_random_state
{
    uint64_t private_state;
    uint64_t private_inc;
};

int                gp_imin(int x, int y);
long               gp_lmin(long x, long y);
long long          gp_llmin(long long x, long long y);
unsigned           gp_umin(unsigned x, unsigned y);
unsigned long      gp_lumin(unsigned long x, unsigned long y);
unsigned long long gp_llumin(unsigned long long x, unsigned long long y);

int                gp_imax(int x, int y);
long               gp_lmax(long x, long y);
long long          gp_llmax(long long x, long long y);
unsigned           gp_umax(unsigned x, unsigned y);
unsigned long      gp_lumax(unsigned long x, unsigned long y);
unsigned long long gp_llumax(unsigned long long x, unsigned long long y);

// gp_min() and gp_max() implementations
#if defined(__GNUC__)

#define gp_generic_min(X, Y) ({ \
    typeof(X) _gp_min_X = (X); typeof(Y) _gp_min_Y = (Y); \
    _gp_min_X < _gp_min_Y ? _gp_min_X : _gp_min_Y; \
})

#define gp_generic_max(X, Y) ({ \
    typeof(X) _gp_max_X = (X); typeof(Y) _gp_max_Y = (Y); \
    _gp_max_X > _gp_max_Y ? _gp_max_X : _gp_max_Y; \
})

#elif __STDC_VERSION__ >= 201112L

#define gp_generic_min(X, Y) \
_Generic(X, \
    int:                gp_imin  (X, Y), \
    long:               gp_lmin  (X, Y), \
    long long:          gp_llmin (X, Y), \
    unsigned:           gp_umin  (X, Y), \
    unsigned long:      gp_lumin (X, Y), \
    unsigned long long: gp_llumin(X, Y), \
    float:              gp_fminf (X, Y), \
    double:             gp_fmin  (X, Y), \
    long double:        gp_fminl (X, Y))

#define gp_generic_max(X, Y) \
_Generic(X, \
    int:                gp_imax  (X, Y), \
    long:               gp_lmax  (X, Y), \
    long long:          gp_llmax (X, Y), \
    unsigned:           gp_umax  (X, Y), \
    unsigned long:      gp_lumax (X, Y), \
    unsigned long long: gp_llumax(X, Y), \
    float:              gp_fmaxf (X, Y), \
    double:             gp_fmax  (X, Y), \
    long double:        gp_fmaxl (X, Y))

#else // Non-GNU C99

// Not ideal but does the job
#define gp_generic_min(X, Y) ((X) < (Y) ? (X) : (Y))
#define gp_generic_max(X, Y) ((X) > (Y) ? (X) : (Y))

#endif // gp_min() and gp_max() implementations

// Set breakpoint
#ifdef _WIN32
#define GP_BREAKPOINT_INTERNAL __debugbreak()
#elif defined(SIGTRAP)
#define GP_BREAKPOINT_INTERNAL raise(SIGTRAP)
#else // no breakpoints for you
#define GP_BREAKPOINT_INTERNAL
#endif // breakpoint

#endif // GP_UTILS_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               array.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <stdint.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
typedef struct gp_array_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPArrayHeader;

#define GPArray(T) T*

#define GP_ARR_ATTRS(...) \
    GP_NONNULL_RETURN GP_NODISCARD GP_NONNULL_ARGS(__VA_ARGS__)

GPArray(void) gp_arr_new(
    const GPAllocator*,
    size_t element_size,
    size_t element_count) GP_ARR_ATTRS();

#define/* GPArray(T) */gp_arr_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    T, ...) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPArrayHeader header; T data[size_t_capacity]; }) { \
{ \
    .length     = sizeof((T[]){__VA_ARGS__})/sizeof(T), \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, {__VA_ARGS__} }.data

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. Example with int:
/*
    struct optional_name { GPArrayHeader header; int data[2048]; } my_array_mem;
    my_array_mem.header = (GPArrayHeader) {.capacity = 2048 };
    GPArray(int) my_array = my_array_mem.data;
*/

// Passing arrays on stack is safe too.
void gp_arr_delete(GPArray(void) optional);

size_t             gp_arr_length    (const GPArray(void)) GP_NONNULL_ARGS();
size_t             gp_arr_capacity  (const GPArray(void)) GP_NONNULL_ARGS();
void*              gp_arr_allocation(const GPArray(void)) GP_NONNULL_ARGS();
const GPAllocator* gp_arr_allocator (const GPArray(void)) GP_NONNULL_ARGS();

GPArray(void) gp_arr_reserve(
    size_t        element_size,
    GPArray(void) arr,
    size_t        capacity) GP_ARR_ATTRS();

GPArray(void) gp_arr_copy(
    size_t              element_size,
    GPArray(void)       dest,
    const void*restrict src,
    size_t              src_length) GP_ARR_ATTRS();

GPArray(void) gp_arr_slice(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              start_index,
    size_t              end_index) GP_ARR_ATTRS(2);

GPArray(void) gp_arr_push(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict element) GP_ARR_ATTRS();

void* gp_arr_pop(
    size_t        element_size,
    GPArray(void) arr) GP_NONNULL_ARGS_AND_RETURN;

GPArray(void) gp_arr_append(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict src,
    size_t              element_count) GP_ARR_ATTRS();

GPArray(void) gp_arr_insert(
    size_t              element_size,
    GPArray(void)       arr,
    size_t              pos,
    const void*restrict src,
    size_t              element_count) GP_ARR_ATTRS();

GPArray(void) gp_arr_remove(
    size_t        element_size,
    GPArray(void) arr,
    size_t        pos,
    size_t        count) GP_NONNULL_ARGS_AND_RETURN;

GPArray(void) gp_arr_map(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    void (*f)(void* out, const void* in)) GP_ARR_ATTRS(2, 5);

void* gp_arr_fold(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element)) GP_NONNULL_ARGS(2, 4);

void* gp_arr_foldr(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element)) GP_NONNULL_ARGS(2, 4);

GPArray(void) gp_arr_filter(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    bool (*f)(const void* element)) GP_ARR_ATTRS(2, 5);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifdef _MSC_VER
// unnamed struct in parenthesis in gp_arr_on_stack()
#pragma warning(disable : 4116)
#endif

#endif // GPC_ARRAY_H




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               terminal.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TERMINAL_INCLUDED
#define GP_TERMINAL_INCLUDED

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Use these macros to print coloured output to terminals that support ANSI
// escape codes. Printing any of these strings changes the output color.
// Example using string concatenation:
/*
    printf(
        GP_RED                "Printing in red! "
        GP_WHITE_BG GP_BLACK "Printing in black with white background! "
        GP_RESET_TERMINAL     "Remember to reset to default color!\n");
*/

#define GP_RESET_TERMINAL      "\033[0m"

// ----------------------------------------------------------------------------
// Foreground color

#define GP_BLACK               "\033[30m"
#define GP_RED                 "\033[31m"
#define GP_GREEN               "\033[32m"
#define GP_YELLOW              "\033[33m"
#define GP_BLUE                "\033[34m"
#define GP_MAGENTA             "\033[35m"
#define GP_CYAN                "\033[36m"
#define GP_WHITE               "\033[37m"

#define GP_BRIGHT_BLACK        "\033[90m"
#define GP_BRIGHT_RED          "\033[91m"
#define GP_BRIGHT_GREEN        "\033[92m"
#define GP_BRIGHT_YELLOW       "\033[93m"
#define GP_BRIGHT_BLUE         "\033[94m"
#define GP_BRIGHT_MAGENTA      "\033[95m"
#define GP_BRIGHT_CYAN         "\033[96m"
#define GP_BRIGHT_WHITE        "\033[97m"

#define GP_RGB(R, G, B)        "\033[38;2;" #R ";" #G ";" #B "m"

// ----------------------------------------------------------------------------
// Background color

#define GP_BLACK_BG            "\033[40m"
#define GP_RED_BG              "\033[41m"
#define GP_GREEN_BG            "\033[42m"
#define GP_YELLOW_BG           "\033[43m"
#define GP_BLUE_BG             "\033[44m"
#define GP_MAGENTA_BG          "\033[45m"
#define GP_CYAN_BG             "\033[46m"
#define GP_WHITE_BG            "\033[47m"

#define GP_BRIGHT_BLACK_BG     "\033[100m"
#define GP_BRIGHT_RED_BG       "\033[101m"
#define GP_BRIGHT_GREEN_BG     "\033[102m"
#define GP_BRIGHT_YELLOW_BG    "\033[103m"
#define GP_BRIGHT_BLUE_BG      "\033[104m"
#define GP_BRIGHT_MAGENTA_BG   "\033[105m"
#define GP_BRIGHT_CYAN_BG      "\033[106m"
#define GP_BRIGHT_WHITE_BG     "\033[107m"

#define GP_RGB_BG(R, G, B)     "\033[38;2;" #R ";" #G ";" #B "m"

// Swap foreground and background colors.
#define GP_INVERT_COLORS       "\033[7m"
#define GP_NO_INVERTED_COLORS  "\033[27m"

// ----------------------------------------------------------------------------
// Font

#define GP_RESET_FONT          "\033[10m"

#define GP_BOLD                "\033[1m"
#define GP_FAINT               "\033[2m"
#define GP_NORMAL_INTENSITY    "\033[22m" // Neither bold nor faint
#define GP_ITALIC              "\033[3m"  // Rarely supported
#define GP_GOTHIC              "\033[20m" // Rarely supported
#define GP_NO_ITALIC           "\033[23m" // Also disables gothic
#define GP_UNDERLINE           "\033[4m"
#define GP_DOUBLE_UNDERLINE    "\033[21m" // May disable bold instead
#define GP_NO_UNDERLINE        "\033[24m" // Also disables double underline
#define GP_SLOW_BLINK          "\033[5m"
#define GP_FAST_BLINK          "\033[6m"  // Rarely supported
#define GP_HIDE                "\033[8m"  // Rarely supported
#define GP_REVEAL              "\033[28m" // Unhide
#define GP_CROSSED_OUT         "\033[9m"

// Select alternative font from 0 to 9 where 0 is default font
#define GP_FONT(N)             "\033[1" #N "m"

// ----------------------------------------------------------------------------
// Cursor movement

// N = steps to move

#define GP_CURSOR_UP(N)            "\033[" #N "A"
#define GP_CURSOR_DOWN(N)          "\033[" #N "B"
#define GP_CURSOR_FORWARD(N)       "\033[" #N "C"
#define GP_CURSOR_BACK(N)          "\033[" #N "D"
#define GP_CURSOR_NEXT_LINE(N)     "\033[" #N "E"
#define GP_CURSOR_PREVIOUS_LINE(N) "\033[" #N "F"

// Moves cursor to row N
#define GP_CURSOR_ROW(N)           "\033[" #N "G"

// Moves cursor to row N column M
#define GP_CURSOR_POSITION(N, M)   "\033[" #N ";" #M "H"

#endif // GP_TERMINAL_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               bytes.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file bytes.h
 * Unsafe and ASCII only, but fast strings.
 */

#ifndef GP_BYTES_INCLUDED
#define GP_BYTES_INCLUDED

#include <stdbool.h>
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

size_t gp_bytes_slice(
    void*restrict       dest,
    const void*restrict optional_src, // mutates dest if NULL
    size_t              start,
    size_t              end) GP_NONNULL_ARGS(1);

size_t gp_bytes_repeat(
    void*restrict       dest,
    size_t              count,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_append(
    void*restrict       dest,
    size_t              dest_size,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_insert(
    void*restrict       dest,
    size_t              dest_size,
    size_t              pos,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_replace_range(
    void*restrict       dest,
    size_t              dest_size,
    size_t              start,
    size_t              end,
    const void*restrict replacement,
    size_t              replacement_length) GP_NONNULL_ARGS();

// Returns index to the first occurrence of needle in haystack.
size_t gp_bytes_replace(
    void*restrict       haystack,
    size_t              haystack_size,
    const void*restrict needle,
    size_t              needle_size,
    const void*restrict replacement,
    size_t              replacement_size,
    size_t*             optional_in_start_out_first_occurrence_position)
    GP_NONNULL_ARGS(1, 3, 5);

size_t gp_bytes_replace_all(
    void*restrict       haystack,
    size_t              haystack_size,
    const void*restrict needle,
    size_t              needle_size,
    const void*restrict replacement,
    size_t              replacement_size,
    size_t*             optional_replacement_count) GP_NONNULL_ARGS(1, 3, 5);

#define/* size_t */gp_bytes_print(bytes_out, ...) \
    GP_BYTES_PRINT(bytes_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_bytes_n_print(bytes_out, n, ...) \
    GP_BYTES_PRINT(bytes_out, n, __VA_ARGS__)

#define/* size_t */gp_bytes_println(bytes_out, ...) \
    GP_BYTES_PRINTLN(bytes_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_bytes_n_println(bytes_out, n, ...) \
    GP_BYTES_PRINTLN(bytes_out, n, __VA_ARGS__)

#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, and 'l' | 'r' for both. Trims whitespace if
// char_set is NULL.
size_t gp_bytes_trim(
    void*restrict       bytes,
    size_t              bytes_size,
    void**restrict      optional_out_ptr, // memmove() if NULL
    const char*restrict optional_char_set,
    int                 flags) GP_NONNULL_ARGS(1);

size_t gp_bytes_to_upper(
    void*  bytes,
    size_t bytes_size) GP_NONNULL_ARGS();

size_t gp_bytes_to_lower(
    void*  bytes,
    size_t bytes_size) GP_NONNULL_ARGS();

size_t gp_bytes_to_valid(
    void*restrict       bytes,
    size_t              bytes_size,
    const char*restrict replacement) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Bytes examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_bytes_find(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size,
    size_t      start) GP_NONNULL_ARGS();

size_t gp_bytes_find_last(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

size_t gp_bytes_count(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

bool gp_bytes_equal(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

bool gp_bytes_equal_case(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

bool gp_bytes_is_valid(
    const void* bytes,
    size_t      bytes_size,
    size_t*     optional_invalid_position) GP_NONNULL_ARGS(1);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


typedef struct gp_printable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum gp_type type;

    // Actual data is in gp_str_print_internal() variadic args.
} GPPrintable;

#if __STDC_VERSION__ >= 201112L
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

size_t gp_bytes_print_internal(
    void*restrict out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_bytes_println_internal(
    void*restrict out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#define GP_BYTES_PRINT(OUT, N, ...) \
    gp_bytes_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_BYTES_PRINTLN(OUT, N, ...) \
    gp_bytes_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_BYTES_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               assert.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file assert.h
 * @brief Unit testing
 */

#ifndef GP_ASSERT_INCLUDED
#define GP_ASSERT_INCLUDED 1

#include <stdbool.h>

#ifndef GP_USER_ASSERT_EXIT
void exit(int status);
#define GP_USER_ASSERT_EXIT (exit)
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and exits program.
#define gp_assert(/* bool condition, variables*/...) \
    ((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), GP_USER_ASSERT_EXIT(1), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define gp_expect(/* bool condition, variables*/...) \
    ((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), false))

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test. Calling with
// NULL when test is not running does nothing.
void gp_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite. Calling
// with NULL when suite is not running does nothing. Also ends last test.
void gp_suite(const char* name);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

// Optional explicit end of all testing and report results. If this
// function is not called explicitly, it will be called when main() returns.
void gp_end_testing(void);

#define GP_FAIL(...) \
    gp_fail_internal( \
        __FILE__, \
        __LINE__, \
        __func__, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)
//
void gp_fail_internal(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#endif // GP_ASSERT_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               overload.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_OVERLOAD_INCLUDED
#define GP_OVERLOAD_INCLUDED 1

#include <stdbool.h>
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Keep things sane
#define GP_MAX_ARGUMENTS 64

// Overloading functions and macro functions by the number of arguments can be
// done with OVERLOADN() macros. First arg to OVERLOADN() is always __VA_ARGS__
// which is followed by names of functions/macros to be overloaded in ascending
// order. The actual arguments also has to be given after OVERLOADN(). Zero
// arguments is not possible.
// Example for max 3 args below:
/*
void func1(int arg1);
#define MACRO2(arg1, arg2) somefunc(arg1, arg2)
int func3(char arg1, void* arg2, const char* arg3);

// Note 3 in the name of the macro.
#define func(...) OVERLOAD3(__VA_ARGS__, func3, MACRO2, func1)(__VA_ARGS__)

int main(void)
{
    // now func() can be called with 1-3 args.
    func(1);
    func(1, 2);
    func('1', (void*)2, "3");
}
*/

// Helper macros

#define GP_STRFY(A) #A
#define GP_STRFY_1ST_ARG(A, ...) #A
#define GP_1ST_ARG(A, ...) A
#define GP_COMMA(...) ,
#define GP_DUMP(...)
#define GP_EVAL(...) __VA_ARGS__

// Arguments list can be processed with GP_PROCESS_ALL_ARGS() macro. The first
// argument is a function or a macro that takes a single argument. This function
// processes the variadic argument list. The second argument determines a
// separator for the variadic argument list. It has to be a macro function that
// takes a variadic argument but just expands to the separator without doing
// anything to __VA_ARGS__ like GP_COMMA() Example uses below:
/*
    int add_one(int x) { return x + 1; }
    int array[] = { GP_PROCESS_ALL_ARGS(add_one, GP_COMMA, 3, 4, 5) };
    // The line above expands to
    int array[] = { add_one(3), add_one(4), add_one(5) };

    #define PLUS(...) +
    int sum = GP_PROCESS_ALL_ARGS(GP_EVAL, PLUS, 2, 3, 4, 5);
    // The line above expands to
    int sum = 2 + 3 + 4 + 5

    // Combining the above we can get sum of squares
    double square(double x) { return x*x; }
    double sum_of_squares = GP_LIST_ALL(square, PLUS, 3.14, .707);
    // expands to
    double sum_of_squares = square(3.14) + square(.707);
*/

// If __VA_OPT__() is needed with GP_PROCESS_ALL_ARGS(),
// GP_PROCESS_ALL_BUT_1ST() can be used instead. GP_PROCESS_ALL_BUT_1ST() processes every argument that is passed to it except the first one. __VA_OPT__() can be simulated by using the first argument as a required argument making all variadic arguments optional without needing __VA_OPT__(). Example below:
/*
    int sq(int x) { return x * x; }
    #define PLUS(...) +

    // First argument required! In this case it's the format string.
    #define PRINT_SUM_OF_SQ(...) printf(GP_PROCESS_ALL_BUT_1ST(sq, PLUS, __VA_ARGS__)

    PRINT_INCREMENTED("%i", 1, 2, 3);
    // expands to
    printf("%i", sq(1) + sq(2) + sq(3));
*/

// ----------------------------------------------------------------------------
// typeof() operator. GNUC and MSVC already covers mostly used compilers, but
// not all compilers are supported.

#if __STDC_VERSION__ >= 202311L || defined(__GNUC__) || defined(__TINYC__)
#define GP_TYPEOF(X) typeof(X)
#elif defined(_MSC_VER)
#define GP_TYPEOF(X) __typeof__(X)
#endif

// Use in variadic function arguments with GP_TYPE() macro
typedef enum gp_type
{
    GP_UNSIGNED_CHAR,
    GP_UNSIGNED_SHORT,
    GP_UNSIGNED,
    GP_UNSIGNED_LONG,
    GP_UNSIGNED_LONG_LONG,
    GP_BOOL,
    GP_SIGNED_CHAR,
    GP_CHAR,
    GP_SHORT,
    GP_INT,
    GP_LONG,
    GP_LONG_LONG,
    GP_FLOAT,
    GP_DOUBLE,
    GP_CHAR_PTR,
    GP_STRING,
    GP_PTR,
} GPType;

inline size_t gp_sizeof(const GPType T) {
    switch (T) {
        case GP_CHAR: case GP_SIGNED_CHAR: case GP_UNSIGNED_CHAR:
            return sizeof(char);
        case GP_SHORT: case GP_UNSIGNED_SHORT:
            return sizeof(short);
        case GP_BOOL:
            return sizeof(bool);
        case GP_INT: case GP_UNSIGNED:
            return sizeof(int);
        case GP_LONG: case GP_UNSIGNED_LONG:
            return sizeof(long);
        case GP_LONG_LONG: case GP_UNSIGNED_LONG_LONG:
            return sizeof(long long);
        case GP_FLOAT:
            return sizeof(float);
        case GP_DOUBLE:
            return sizeof(double);
        case GP_CHAR_PTR: case GP_STRING: case GP_PTR:
            return sizeof(char*);
    }
    return 0;
}

#define GP_TYPE(VAR)                              \
_Generic(VAR,                                     \
    bool:                  GP_BOOL,               \
    short:                 GP_SHORT,              \
    int:                   GP_INT,                \
    long:                  GP_LONG,               \
    long long:             GP_LONG_LONG,          \
    unsigned short:        GP_UNSIGNED_LONG,      \
    unsigned int:          GP_UNSIGNED,           \
    unsigned long:         GP_UNSIGNED_LONG,      \
    unsigned long long:    GP_UNSIGNED_LONG_LONG, \
    float:                 GP_FLOAT,              \
    double:                GP_DOUBLE,             \
    char:                  GP_CHAR,               \
    unsigned char:         GP_UNSIGNED_CHAR,      \
    signed char:           GP_SIGNED_CHAR,        \
    char*:                 GP_CHAR_PTR,           \
    const char*:           GP_CHAR_PTR,           \
    struct gp_char*:       GP_STRING,             \
    const struct gp_char*: GP_STRING,             \
    default:               GP_PTR)

inline bool gp_is_unsigned(const GPType T) { return T <= GP_UNSIGNED_LONG_LONG; }
inline bool gp_is_integer (const GPType T) { return T <= GP_LONG_LONG; }
inline bool gp_is_floating(const GPType T) { return GP_FLOAT <= T && T <= GP_DOUBLE; }
inline bool gp_is_pointer (const GPType T) { return GP_CHAR_PTR <= T && T <= GP_PTR; }

// Returns the number of arguments
#define GP_COUNT_ARGS(...) GP_OVERLOAD64(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,  \
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,  \
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#if __STDC_VERSION__ <= 199901L
// Unique struct/union name
#define GP_MAKE_UNIQUE(A, B) A##B
#define GP_C99_UNIQUE_STRUCT(LINE) GP_MAKE_UNIQUE(_gp_uniqs__, LINE)
#else
// C11 allows structs and unions to be unnamed
#define GP_C99_UNIQUE_STRUCT(_)
#endif

// ----------------------------------------------------------------------------
// Gory script generated internals below. You have been warned.

#define GP_PROCESS_ALL_ARGS(FUNC, SEPARATOR, ...) GP_OVERLOAD64(__VA_ARGS__, 	\
GP_PROC64, GP_PROC63, GP_PROC62, GP_PROC61, GP_PROC60, GP_PROC59, GP_PROC58, GP_PROC57, \
GP_PROC56, GP_PROC55, GP_PROC54, GP_PROC53, GP_PROC52, GP_PROC51, GP_PROC50, GP_PROC49, \
GP_PROC48, GP_PROC47, GP_PROC46, GP_PROC45, GP_PROC44, GP_PROC43, GP_PROC42, GP_PROC41, \
GP_PROC40, GP_PROC39, GP_PROC38, GP_PROC37, GP_PROC36, GP_PROC35, GP_PROC34, GP_PROC33, \
GP_PROC32, GP_PROC31, GP_PROC30, GP_PROC29, GP_PROC28, GP_PROC27, GP_PROC26, GP_PROC25, \
GP_PROC24, GP_PROC23, GP_PROC22, GP_PROC21, GP_PROC20, GP_PROC19, GP_PROC18, GP_PROC17, \
GP_PROC16, GP_PROC15, GP_PROC14, GP_PROC13, GP_PROC12, GP_PROC11, GP_PROC10, GP_PROC9, 	\
GP_PROC8, GP_PROC7, GP_PROC6, GP_PROC5, GP_PROC4, GP_PROC3, GP_PROC2, GP_PROC1)	\
(FUNC, SEPARATOR, __VA_ARGS__)

#define GP_PROCESS_ALL_BUT_1ST(FUNC, SEPARATOR, ...) GP_OVERLOAD64(__VA_ARGS__, 	\
GP_PROC64_1, GP_PROC63_1, GP_PROC62_1, GP_PROC61_1, GP_PROC60_1, GP_PROC59_1, GP_PROC58_1, \
GP_PROC57_1, GP_PROC56_1, GP_PROC55_1, GP_PROC54_1, GP_PROC53_1, GP_PROC52_1, GP_PROC51_1, \
GP_PROC50_1, GP_PROC49_1, GP_PROC48_1, GP_PROC47_1, GP_PROC46_1, GP_PROC45_1, GP_PROC44_1, \
GP_PROC43_1, GP_PROC42_1, GP_PROC41_1, GP_PROC40_1, GP_PROC39_1, GP_PROC38_1, GP_PROC37_1, \
GP_PROC36_1, GP_PROC35_1, GP_PROC34_1, GP_PROC33_1, GP_PROC32_1, GP_PROC31_1, GP_PROC30_1, \
GP_PROC29_1, GP_PROC28_1, GP_PROC27_1, GP_PROC26_1, GP_PROC25_1, GP_PROC24_1, GP_PROC23_1, \
GP_PROC22_1, GP_PROC21_1, GP_PROC20_1, GP_PROC19_1, GP_PROC18_1, GP_PROC17_1, GP_PROC16_1, \
GP_PROC15_1, GP_PROC14_1, GP_PROC13_1, GP_PROC12_1, GP_PROC11_1, GP_PROC10_1, GP_PROC9_1, \
GP_PROC8_1, GP_PROC7_1, GP_PROC6_1, GP_PROC5_1, GP_PROC4_1, GP_PROC3_1, GP_PROC2_1, 	\
GP_PROC1_1)(FUNC, SEPARATOR, __VA_ARGS__)

#define GP_PROC1(F, SEP, A) F(A)
#define GP_PROC2(F, SEP, A, ...) F(A) SEP(A) GP_PROC1(F, SEP, __VA_ARGS__)
#define GP_PROC3(F, SEP, A, ...) F(A) SEP(A) GP_PROC2(F, SEP, __VA_ARGS__)
#define GP_PROC4(F, SEP, A, ...) F(A) SEP(A) GP_PROC3(F, SEP, __VA_ARGS__)
#define GP_PROC5(F, SEP, A, ...) F(A) SEP(A) GP_PROC4(F, SEP, __VA_ARGS__)
#define GP_PROC6(F, SEP, A, ...) F(A) SEP(A) GP_PROC5(F, SEP, __VA_ARGS__)
#define GP_PROC7(F, SEP, A, ...) F(A) SEP(A) GP_PROC6(F, SEP, __VA_ARGS__)
#define GP_PROC8(F, SEP, A, ...) F(A) SEP(A) GP_PROC7(F, SEP, __VA_ARGS__)
#define GP_PROC9(F, SEP, A, ...) F(A) SEP(A) GP_PROC8(F, SEP, __VA_ARGS__)
#define GP_PROC10(F, SEP, A, ...) F(A) SEP(A) GP_PROC9(F, SEP, __VA_ARGS__)
#define GP_PROC11(F, SEP, A, ...) F(A) SEP(A) GP_PROC10(F, SEP, __VA_ARGS__)
#define GP_PROC12(F, SEP, A, ...) F(A) SEP(A) GP_PROC11(F, SEP, __VA_ARGS__)
#define GP_PROC13(F, SEP, A, ...) F(A) SEP(A) GP_PROC12(F, SEP, __VA_ARGS__)
#define GP_PROC14(F, SEP, A, ...) F(A) SEP(A) GP_PROC13(F, SEP, __VA_ARGS__)
#define GP_PROC15(F, SEP, A, ...) F(A) SEP(A) GP_PROC14(F, SEP, __VA_ARGS__)
#define GP_PROC16(F, SEP, A, ...) F(A) SEP(A) GP_PROC15(F, SEP, __VA_ARGS__)
#define GP_PROC17(F, SEP, A, ...) F(A) SEP(A) GP_PROC16(F, SEP, __VA_ARGS__)
#define GP_PROC18(F, SEP, A, ...) F(A) SEP(A) GP_PROC17(F, SEP, __VA_ARGS__)
#define GP_PROC19(F, SEP, A, ...) F(A) SEP(A) GP_PROC18(F, SEP, __VA_ARGS__)
#define GP_PROC20(F, SEP, A, ...) F(A) SEP(A) GP_PROC19(F, SEP, __VA_ARGS__)
#define GP_PROC21(F, SEP, A, ...) F(A) SEP(A) GP_PROC20(F, SEP, __VA_ARGS__)
#define GP_PROC22(F, SEP, A, ...) F(A) SEP(A) GP_PROC21(F, SEP, __VA_ARGS__)
#define GP_PROC23(F, SEP, A, ...) F(A) SEP(A) GP_PROC22(F, SEP, __VA_ARGS__)
#define GP_PROC24(F, SEP, A, ...) F(A) SEP(A) GP_PROC23(F, SEP, __VA_ARGS__)
#define GP_PROC25(F, SEP, A, ...) F(A) SEP(A) GP_PROC24(F, SEP, __VA_ARGS__)
#define GP_PROC26(F, SEP, A, ...) F(A) SEP(A) GP_PROC25(F, SEP, __VA_ARGS__)
#define GP_PROC27(F, SEP, A, ...) F(A) SEP(A) GP_PROC26(F, SEP, __VA_ARGS__)
#define GP_PROC28(F, SEP, A, ...) F(A) SEP(A) GP_PROC27(F, SEP, __VA_ARGS__)
#define GP_PROC29(F, SEP, A, ...) F(A) SEP(A) GP_PROC28(F, SEP, __VA_ARGS__)
#define GP_PROC30(F, SEP, A, ...) F(A) SEP(A) GP_PROC29(F, SEP, __VA_ARGS__)
#define GP_PROC31(F, SEP, A, ...) F(A) SEP(A) GP_PROC30(F, SEP, __VA_ARGS__)
#define GP_PROC32(F, SEP, A, ...) F(A) SEP(A) GP_PROC31(F, SEP, __VA_ARGS__)
#define GP_PROC33(F, SEP, A, ...) F(A) SEP(A) GP_PROC32(F, SEP, __VA_ARGS__)
#define GP_PROC34(F, SEP, A, ...) F(A) SEP(A) GP_PROC33(F, SEP, __VA_ARGS__)
#define GP_PROC35(F, SEP, A, ...) F(A) SEP(A) GP_PROC34(F, SEP, __VA_ARGS__)
#define GP_PROC36(F, SEP, A, ...) F(A) SEP(A) GP_PROC35(F, SEP, __VA_ARGS__)
#define GP_PROC37(F, SEP, A, ...) F(A) SEP(A) GP_PROC36(F, SEP, __VA_ARGS__)
#define GP_PROC38(F, SEP, A, ...) F(A) SEP(A) GP_PROC37(F, SEP, __VA_ARGS__)
#define GP_PROC39(F, SEP, A, ...) F(A) SEP(A) GP_PROC38(F, SEP, __VA_ARGS__)
#define GP_PROC40(F, SEP, A, ...) F(A) SEP(A) GP_PROC39(F, SEP, __VA_ARGS__)
#define GP_PROC41(F, SEP, A, ...) F(A) SEP(A) GP_PROC40(F, SEP, __VA_ARGS__)
#define GP_PROC42(F, SEP, A, ...) F(A) SEP(A) GP_PROC41(F, SEP, __VA_ARGS__)
#define GP_PROC43(F, SEP, A, ...) F(A) SEP(A) GP_PROC42(F, SEP, __VA_ARGS__)
#define GP_PROC44(F, SEP, A, ...) F(A) SEP(A) GP_PROC43(F, SEP, __VA_ARGS__)
#define GP_PROC45(F, SEP, A, ...) F(A) SEP(A) GP_PROC44(F, SEP, __VA_ARGS__)
#define GP_PROC46(F, SEP, A, ...) F(A) SEP(A) GP_PROC45(F, SEP, __VA_ARGS__)
#define GP_PROC47(F, SEP, A, ...) F(A) SEP(A) GP_PROC46(F, SEP, __VA_ARGS__)
#define GP_PROC48(F, SEP, A, ...) F(A) SEP(A) GP_PROC47(F, SEP, __VA_ARGS__)
#define GP_PROC49(F, SEP, A, ...) F(A) SEP(A) GP_PROC48(F, SEP, __VA_ARGS__)
#define GP_PROC50(F, SEP, A, ...) F(A) SEP(A) GP_PROC49(F, SEP, __VA_ARGS__)
#define GP_PROC51(F, SEP, A, ...) F(A) SEP(A) GP_PROC50(F, SEP, __VA_ARGS__)
#define GP_PROC52(F, SEP, A, ...) F(A) SEP(A) GP_PROC51(F, SEP, __VA_ARGS__)
#define GP_PROC53(F, SEP, A, ...) F(A) SEP(A) GP_PROC52(F, SEP, __VA_ARGS__)
#define GP_PROC54(F, SEP, A, ...) F(A) SEP(A) GP_PROC53(F, SEP, __VA_ARGS__)
#define GP_PROC55(F, SEP, A, ...) F(A) SEP(A) GP_PROC54(F, SEP, __VA_ARGS__)
#define GP_PROC56(F, SEP, A, ...) F(A) SEP(A) GP_PROC55(F, SEP, __VA_ARGS__)
#define GP_PROC57(F, SEP, A, ...) F(A) SEP(A) GP_PROC56(F, SEP, __VA_ARGS__)
#define GP_PROC58(F, SEP, A, ...) F(A) SEP(A) GP_PROC57(F, SEP, __VA_ARGS__)
#define GP_PROC59(F, SEP, A, ...) F(A) SEP(A) GP_PROC58(F, SEP, __VA_ARGS__)
#define GP_PROC60(F, SEP, A, ...) F(A) SEP(A) GP_PROC59(F, SEP, __VA_ARGS__)
#define GP_PROC61(F, SEP, A, ...) F(A) SEP(A) GP_PROC60(F, SEP, __VA_ARGS__)
#define GP_PROC62(F, SEP, A, ...) F(A) SEP(A) GP_PROC61(F, SEP, __VA_ARGS__)
#define GP_PROC63(F, SEP, A, ...) F(A) SEP(A) GP_PROC62(F, SEP, __VA_ARGS__)
#define GP_PROC64(F, SEP, A, ...) F(A) SEP(A) GP_PROC63(F, SEP, __VA_ARGS__)

#define GP_PROC1_1(F, SEP, A) A
#define GP_PROC2_1(F, SEP, A, ...) A, GP_PROC1(F, SEP, __VA_ARGS__)
#define GP_PROC3_1(F, SEP, A, ...) A, GP_PROC2(F, SEP, __VA_ARGS__)
#define GP_PROC4_1(F, SEP, A, ...) A, GP_PROC3(F, SEP, __VA_ARGS__)
#define GP_PROC5_1(F, SEP, A, ...) A, GP_PROC4(F, SEP, __VA_ARGS__)
#define GP_PROC6_1(F, SEP, A, ...) A, GP_PROC5(F, SEP, __VA_ARGS__)
#define GP_PROC7_1(F, SEP, A, ...) A, GP_PROC6(F, SEP, __VA_ARGS__)
#define GP_PROC8_1(F, SEP, A, ...) A, GP_PROC7(F, SEP, __VA_ARGS__)
#define GP_PROC9_1(F, SEP, A, ...) A, GP_PROC8(F, SEP, __VA_ARGS__)
#define GP_PROC10_1(F, SEP, A, ...) A, GP_PROC9(F, SEP, __VA_ARGS__)
#define GP_PROC11_1(F, SEP, A, ...) A, GP_PROC10(F, SEP, __VA_ARGS__)
#define GP_PROC12_1(F, SEP, A, ...) A, GP_PROC11(F, SEP, __VA_ARGS__)
#define GP_PROC13_1(F, SEP, A, ...) A, GP_PROC12(F, SEP, __VA_ARGS__)
#define GP_PROC14_1(F, SEP, A, ...) A, GP_PROC13(F, SEP, __VA_ARGS__)
#define GP_PROC15_1(F, SEP, A, ...) A, GP_PROC14(F, SEP, __VA_ARGS__)
#define GP_PROC16_1(F, SEP, A, ...) A, GP_PROC15(F, SEP, __VA_ARGS__)
#define GP_PROC17_1(F, SEP, A, ...) A, GP_PROC16(F, SEP, __VA_ARGS__)
#define GP_PROC18_1(F, SEP, A, ...) A, GP_PROC17(F, SEP, __VA_ARGS__)
#define GP_PROC19_1(F, SEP, A, ...) A, GP_PROC18(F, SEP, __VA_ARGS__)
#define GP_PROC20_1(F, SEP, A, ...) A, GP_PROC19(F, SEP, __VA_ARGS__)
#define GP_PROC21_1(F, SEP, A, ...) A, GP_PROC20(F, SEP, __VA_ARGS__)
#define GP_PROC22_1(F, SEP, A, ...) A, GP_PROC21(F, SEP, __VA_ARGS__)
#define GP_PROC23_1(F, SEP, A, ...) A, GP_PROC22(F, SEP, __VA_ARGS__)
#define GP_PROC24_1(F, SEP, A, ...) A, GP_PROC23(F, SEP, __VA_ARGS__)
#define GP_PROC25_1(F, SEP, A, ...) A, GP_PROC24(F, SEP, __VA_ARGS__)
#define GP_PROC26_1(F, SEP, A, ...) A, GP_PROC25(F, SEP, __VA_ARGS__)
#define GP_PROC27_1(F, SEP, A, ...) A, GP_PROC26(F, SEP, __VA_ARGS__)
#define GP_PROC28_1(F, SEP, A, ...) A, GP_PROC27(F, SEP, __VA_ARGS__)
#define GP_PROC29_1(F, SEP, A, ...) A, GP_PROC28(F, SEP, __VA_ARGS__)
#define GP_PROC30_1(F, SEP, A, ...) A, GP_PROC29(F, SEP, __VA_ARGS__)
#define GP_PROC31_1(F, SEP, A, ...) A, GP_PROC30(F, SEP, __VA_ARGS__)
#define GP_PROC32_1(F, SEP, A, ...) A, GP_PROC31(F, SEP, __VA_ARGS__)
#define GP_PROC33_1(F, SEP, A, ...) A, GP_PROC32(F, SEP, __VA_ARGS__)
#define GP_PROC34_1(F, SEP, A, ...) A, GP_PROC33(F, SEP, __VA_ARGS__)
#define GP_PROC35_1(F, SEP, A, ...) A, GP_PROC34(F, SEP, __VA_ARGS__)
#define GP_PROC36_1(F, SEP, A, ...) A, GP_PROC35(F, SEP, __VA_ARGS__)
#define GP_PROC37_1(F, SEP, A, ...) A, GP_PROC36(F, SEP, __VA_ARGS__)
#define GP_PROC38_1(F, SEP, A, ...) A, GP_PROC37(F, SEP, __VA_ARGS__)
#define GP_PROC39_1(F, SEP, A, ...) A, GP_PROC38(F, SEP, __VA_ARGS__)
#define GP_PROC40_1(F, SEP, A, ...) A, GP_PROC39(F, SEP, __VA_ARGS__)
#define GP_PROC41_1(F, SEP, A, ...) A, GP_PROC40(F, SEP, __VA_ARGS__)
#define GP_PROC42_1(F, SEP, A, ...) A, GP_PROC41(F, SEP, __VA_ARGS__)
#define GP_PROC43_1(F, SEP, A, ...) A, GP_PROC42(F, SEP, __VA_ARGS__)
#define GP_PROC44_1(F, SEP, A, ...) A, GP_PROC43(F, SEP, __VA_ARGS__)
#define GP_PROC45_1(F, SEP, A, ...) A, GP_PROC44(F, SEP, __VA_ARGS__)
#define GP_PROC46_1(F, SEP, A, ...) A, GP_PROC45(F, SEP, __VA_ARGS__)
#define GP_PROC47_1(F, SEP, A, ...) A, GP_PROC46(F, SEP, __VA_ARGS__)
#define GP_PROC48_1(F, SEP, A, ...) A, GP_PROC47(F, SEP, __VA_ARGS__)
#define GP_PROC49_1(F, SEP, A, ...) A, GP_PROC48(F, SEP, __VA_ARGS__)
#define GP_PROC50_1(F, SEP, A, ...) A, GP_PROC49(F, SEP, __VA_ARGS__)
#define GP_PROC51_1(F, SEP, A, ...) A, GP_PROC50(F, SEP, __VA_ARGS__)
#define GP_PROC52_1(F, SEP, A, ...) A, GP_PROC51(F, SEP, __VA_ARGS__)
#define GP_PROC53_1(F, SEP, A, ...) A, GP_PROC52(F, SEP, __VA_ARGS__)
#define GP_PROC54_1(F, SEP, A, ...) A, GP_PROC53(F, SEP, __VA_ARGS__)
#define GP_PROC55_1(F, SEP, A, ...) A, GP_PROC54(F, SEP, __VA_ARGS__)
#define GP_PROC56_1(F, SEP, A, ...) A, GP_PROC55(F, SEP, __VA_ARGS__)
#define GP_PROC57_1(F, SEP, A, ...) A, GP_PROC56(F, SEP, __VA_ARGS__)
#define GP_PROC58_1(F, SEP, A, ...) A, GP_PROC57(F, SEP, __VA_ARGS__)
#define GP_PROC59_1(F, SEP, A, ...) A, GP_PROC58(F, SEP, __VA_ARGS__)
#define GP_PROC60_1(F, SEP, A, ...) A, GP_PROC59(F, SEP, __VA_ARGS__)
#define GP_PROC61_1(F, SEP, A, ...) A, GP_PROC60(F, SEP, __VA_ARGS__)
#define GP_PROC62_1(F, SEP, A, ...) A, GP_PROC61(F, SEP, __VA_ARGS__)
#define GP_PROC63_1(F, SEP, A, ...) A, GP_PROC62(F, SEP, __VA_ARGS__)
#define GP_PROC64_1(F, SEP, A, ...) A, GP_PROC63(F, SEP, __VA_ARGS__)

#define GP_OVERLOAD1(_0, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD2(_0, _1, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD3(_0, _1, _2, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD4(_0, _1, _2, _3, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD5(_0, _1, _2, _3, _4, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD6(_0, _1, _2, _3, _4, _5, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD7(_0, _1, _2, _3, _4, _5, _6, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD8(_0, _1, _2, _3, _4, _5, _6, _7, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD9(_0, _1, _2, _3, _4, _5, _6, _7, _8, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, RESOLVED, 	\
...) RESOLVED
#define GP_OVERLOAD14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, RESOLVED, ...) RESOLVED

#endif // GP_OVERLOAD_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               attributes.h
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ATTRIBUTES_INCLUDED
#define GP_ATTRIBUTES_INCLUDED

// ----------------------------------------------------------------------------
// Nodiscard

#ifdef __GNUC__
#define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER)
#define GP_NODISCARD _Check_return_
#else
#define GP_NODISCARD
#endif

// ----------------------------------------------------------------------------
// Nonnull

#ifdef __GNUC__
#define GP_NONNULL_ARGS(...) __attribute__((nonnull (__VA_ARGS__)))
#define GP_NONNULL_RETURN    __attribute__((returns_nonnull))
#define GP_NONNULL_ARGS_AND_RETURN \
    __attribute__((nonnull)) __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#define GP_NONNULL_ARGS(...)
#define GP_NONNULL_RETURN _Ret_notnull_
#define GP_NONNULL_ARGS_AND_RETURN _Ret_notnull_
#else
#define GP_NONNULL_ARGS(...)
#define GP_NONNULL_RETURN
#define GP_NONNULL_ARGS_AND_RETURN
#endif

// ----------------------------------------------------------------------------
// Malloc-like functions

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

#ifdef __GNUC__
#define GP_MALLOC_SIZE(...) __attribute__((alloc_size (__VA_ARGS__)))
#else
#define GP_MALLOC_SIZE(...)
#endif

// ----------------------------------------------------------------------------
// Static array index

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) && \
    !defined(__TINYC__) && \
    !defined(__MSP430__) && \
    !defined(__COMPCERT__)
// Use to specify an array argument with at least some number of valid elements,
// e.g. "void foo(int arr[GPC_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.
#define GP_STATIC static
#else
#define GP_STATIC
#endif

// ----------------------------------------------------------------------------
// Printf format string type checking

#if defined(__GNUC__)
// Type checking for format strings
#define GP_PRINTF(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
    __attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))
#else
#define GP_PRINTF(...)
#endif

// ----------------------------------------------------------------------------
// Disable sanitizer

#ifdef __GNUC__
#define GP_NO_SANITIZE __attribute__((no_sanitize("address", "leak", "undefined")))
#else
#define GP_NO_SANITIZE
#endif

#endif // GP_ATTRIBUTES_INCLUDED




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//
//                    IMPLEMENTATION
//
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#ifdef GPC_IMPLEMENTATION

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               utils.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)


extern inline uintptr_t gp_round_to_aligned(uintptr_t);
extern inline bool gp_fapproxf(float x, float y, float max_relative_diff);
extern inline bool gp_fapprox(double x, double y, double max_relative_diff);
extern inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff);
extern inline bool gp_mem_equal(const void* rhs, const void* lhs, size_t, size_t);

size_t gp_next_power_of_2(size_t x)
{
    return sizeof x == sizeof(uint32_t) ?
        gp_next_power_of_2_32(x) : gp_next_power_of_2_64(x);
}

uint32_t gp_next_power_of_2_32(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

uint64_t gp_next_power_of_2_64(uint64_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

bool gp_check_bounds(size_t* start, size_t* end, size_t limit)
{
    bool clipped = false;
    end = end != NULL ? end : &(size_t){ limit };
    if (*end > limit) {
        *end = limit;
        clipped = true;
    }
    if (start != NULL && *start >= *end) {
        *start  = *end - (limit != 0);
        clipped = true;
    }
    return ! clipped;
}



// Random stuff

//static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

GPRandomState gp_new_random_state(uint64_t seed)
{
    GPRandomState state;
    pcg32_srandom_r((pcg32_random_t*)&state, seed, 0xf35d3918378e53c4ULL);
    return state;
}

uint32_t gp_random(GPRandomState* state)
{
    return pcg32_random_r((pcg32_random_t*)state);
}

double gp_frandom(GPRandomState* state)
{
    return ldexp(pcg32_random_r((pcg32_random_t*)state), -32);
}

int32_t gp_random_range(GPRandomState* state, int32_t min, int32_t max)
{
    if (max - min > 0)
        return  (int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)( max - min + 1)) + min;
    else
        return -(int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)(-max + min - 1)) + min;
}




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               bytes.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <printf/conversions.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <limits.h>

static void* gp_memmem(
    const void* haystack, const size_t hlen, const void* needle, const size_t nlen)
{
    #if defined(_GNU_SOURCE) && defined(__linux__)
    return memmem(haystack, hlen, needle, nlen);
    #endif
    if (hlen == 0 || nlen == 0)
        return NULL;

    const char n0 = *(char*)needle;
    for (void* p = memchr(haystack, n0, hlen); p != NULL;)
    {
        if (p + nlen > haystack + hlen)
            return NULL;
        if (memcmp(p, needle, nlen) == 0)
            return p;

        p++;
        p = memchr(p, n0, hlen - (p - haystack));
    }
    return NULL;
}

size_t gp_bytes_find(
    const void*  haystack,
    const size_t haystack_size,
    const void*  needle,
    const size_t needle_size,
    const size_t start)
{
    const void* result = gp_memmem(
        haystack + start, haystack_size - start, needle, needle_size);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* gp_memchr_r(const char* ptr_r, const char ch, size_t count)
{
    const char* position = NULL;
    while (--ptr_r, --count != (size_t)-1) // <=> count >= 0
    {
        if (*ptr_r == ch) {
            position = ptr_r;
            break;
        }
    }
    return position;
}

size_t gp_bytes_find_last(
    const void*  _haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_length)
{
    const char* haystack = (const char*)_haystack;

    if (needle_length > haystack_length || needle_length==0 || haystack_length==0)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = gp_memchr_r(data, *(char*)needle, to_be_searched)))
    {
        if (memcmp(data, needle, needle_length) == 0)
        {
            position = (size_t)(data - haystack);
            break;
        }
        data--;
        const char* haystack_end = haystack + haystack_length;
        to_be_searched = haystack_length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gp_bytes_count(
    const void*  haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_size)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_bytes_find(haystack, haystack_length, needle, needle_size, i))
        != GP_NOT_FOUND)
    {
        count++;
        i++;
    }
    return count;
}

bool gp_bytes_equal(
    const void*  s1,
    const size_t s1_size,
    const void*  s2,
    const size_t s2_size)
{
    if (s1_size != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

bool gp_bytes_equal_case(
    const void* _s1,
    const size_t s1_size,
    const void* _s2,
    const size_t s2_size)
{
    if (s1_size != s2_size)
        return false;

    const char* s1 = _s1;
    const char* s2 = _s2;
    for (size_t i = 0; i < s1_size; i++)
    {
        const char c1 = s1[i] + ('A' <= s1[i] && s1[i] <= 'Z') * ('a' - 'A');
        const char c2 = s2[i] + ('A' <= s2[i] && s2[i] <= 'Z') * ('a' - 'A');
        if (c1 != c2)
            return false;
    }
    return true;
}

bool gp_bytes_is_valid(
    const void* _str,
    const size_t n,
    size_t* invalid_index)
{
    const uint8_t* str = _str;
    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; i++) {
        if (str[i] & 0x80) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
    }
    for (; i < n - remaining; i += 8) {
        uint64_t x;
        memcpy(&x, str + i, sizeof x);
        if (x & 0x8080808080808080) // invalid detected
            break; // find the index for the invalid in the next loop
    }
    for (; i < n; i++) {
        if (str[i] & 0x80) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
    }
    return true;
}

size_t gp_bytes_slice(
    void*restrict dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    if (src != NULL)
        memcpy(dest, src + start, end - start);
    else
        memmove(dest, (uint8_t*)dest + start, end - start);
    return end - start;
}

size_t gp_bytes_repeat(
    void*restrict dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    if (mem_length == 1) {
        memset(dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(dest + i * mem_length, mem, mem_length);
    }
    return n * mem_length;
}

size_t gp_bytes_append(
    void*restrict dest,
    const size_t dest_length,
    const void* src,
    const size_t src_length)
{
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    return dest_length + src_length;
}

size_t gp_bytes_insert(
    void*restrict dest,
    const size_t dest_length,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    return dest_length + n;
}

size_t gp_bytes_replace_range(
    void*restrict me,
    const size_t me_length,
    const size_t start,
    const size_t end,
    const void*restrict replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}

size_t gp_bytes_replace(
    void*restrict haystack,
    const size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* in_start_out_pos)
{
    size_t start = in_start_out_pos != NULL ? *in_start_out_pos : 0;
    if ((start = gp_bytes_find(haystack, haystack_length, needle, needle_length, start))
        == GP_NOT_FOUND) {
        return GP_NOT_FOUND;
    }

    if (in_start_out_pos != NULL)
        *in_start_out_pos = start;

    const size_t end = start + needle_length;
    return gp_bytes_replace_range(
        haystack,
        haystack_length,
        start,
        end,
        replacement,
        replacement_length);
}

size_t gp_bytes_replace_all(
    void*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* optional_replacement_count)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_bytes_find(haystack, haystack_length, needle, needle_length, start))
        != GP_NOT_FOUND)
    {
        haystack_length = gp_bytes_replace_range(
            haystack,
            haystack_length,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;
    return haystack_length;
}

size_t gp_bytes_print_internal(
    void*restrict out,
    const size_t n,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += gp_bytes_print_objects(
            n >= length ? n - length : 0,
            (uint8_t*)out + length,
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    return length;
}

size_t gp_bytes_println_internal(
    void*restrict out,
    const size_t n,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += gp_bytes_print_objects(
            n >= length ? n - length : 0,
            (uint8_t*)out + length,
            &args,
            &i,
            objs[i]);

        if (n > length)
            ((char*)out)[length++] = ' ';
    }
    va_end(_args);
    va_end(args.list);

    if (n > (length - !!length)) // overwrite last space
        ((char*)out)[length - 1] = '\n';

    return length;
}

size_t gp_bytes_trim(
    void*restrict _str,
    size_t length,
    void**restrict optional_out_ptr,
    const char*restrict optional_char_set,
    int flags)
{
    char* str = _str;
    const bool left  = flags & 0x04;
    const bool right = flags & 0x02;

    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_ASCII_WHITESPACE;

    if (left)
    {
        char last = str[length - 1];
        str[length - 1] = '\0';
        size_t prefix_length = strspn(str, char_set);
        str[length - 1] = last;

        if (prefix_length == length - 1 && strchr(char_set, last) != NULL)
            prefix_length++;

        length -= prefix_length;

        if (optional_out_ptr != NULL)
            *optional_out_ptr = str + prefix_length;
        else
            memmove(str, str + prefix_length, length);
    }

    if (right && length > 0)
    {
        while (strchr(char_set, ((char*)str)[length - 1]) != NULL) {
            length--;
            if (length == 0)
                break;
        }
    }
    return length;
}

size_t gp_bytes_to_upper(
    void* _bytes,
    size_t bytes_size)
{
    char* bytes = _bytes;
    for (size_t i = 0; i < bytes_size; i++)
    {
        if ('a' <= bytes[i] && bytes[i] <= 'z')
            bytes[i] -= 'a' - 'A';
    }
    return bytes_size;
}

size_t gp_bytes_to_lower(
    void* _bytes,
    size_t bytes_size)
{
    char* bytes = _bytes;
    for (size_t i = 0; i < bytes_size; i++)
    {
        if ('A' <= bytes[i] && bytes[i] <= 'Z')
            bytes[i] += 'a' - 'A';
    }
    return bytes_size;
}

static size_t gp_bytes_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const uint8_t* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        if (haystack[i] >= 0x80)
            return i;
    }
    return GP_NOT_FOUND;
}

static size_t gp_bytes_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const uint8_t* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        if (haystack[i] < 0x80)
            return i;
    }
    return length;
}

size_t gp_bytes_to_valid(
    void*restrict str,
    size_t length,
    const char* replacement)
{
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_bytes_find_invalid(str, start, length)) != GP_NOT_FOUND)
    {
        length = gp_bytes_replace_range(
            str,
            length,
            start,
            gp_bytes_find_valid(str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }
    return length;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               gpbuild.c_
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef __unix__
#include <unistd.h>
#endif

#include <stdio.h>

int main(void)
{
    execlp("gcc", "gcc", "--version", NULL);
    perror(NULL);
    puts("Not gonna haååen");
}




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               string.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <wctype.h>
#include <printf/printf.h>
#include <printf/conversions.h>
#include <sys/types.h>
#include <sys/stat.h>

GPString gp_str_new(
    const GPAllocator* allocator,
    size_t capacity,
    const char* init)
{
    GPStringHeader* me = gp_mem_alloc(allocator, sizeof*me + capacity + sizeof"");
    *me = (GPStringHeader) {
        .capacity   = capacity,
        .allocator  = allocator,
        .allocation = me };
    return memcpy(me + 1, init, strlen(init));
}

void gp_str_delete(GPString me)
{
    if (me != NULL && gp_str_allocation(me) != NULL)
        gp_dealloc(gp_str_allocator(me), gp_str_allocation(me));
}

static GPStringHeader* gp_str_header(const GPString str)
{
    return (GPStringHeader*)str - 1;
}

size_t             gp_str_length    (GPString s) { return gp_str_header(s)->length;    }
size_t             gp_str_capacity  (GPString s) { return gp_str_header(s)->capacity;  }
void*              gp_str_allocation(GPString s) { return gp_str_header(s)->allocation;}
const GPAllocator* gp_str_allocator (GPString s) { return gp_str_header(s)->allocator; }

size_t gp_str_find(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start)
{
    return gp_bytes_find(haystack, gp_str_length(haystack), needle, needle_size, start);
}

size_t gp_str_find_last(
    GPString haystack,
    const void* needle,
    size_t needle_length)
{
    return gp_bytes_find_last(haystack, gp_str_length(haystack), needle, needle_length);
}

size_t gp_str_count(
    GPString haystack,
    const void* needle,
    size_t     needle_size)
{
    return gp_bytes_count(haystack, gp_str_length(haystack), needle, needle_size);
}

bool gp_str_equal(
    GPString  s1,
    const void* s2,
    size_t      s2_size)
{
    if (gp_str_length(s1) != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

static size_t gp_bytes_codepoint_count(
    const void* _str,
    const size_t n)
{
    size_t count = 0;
    const char* str = _str;
    static const size_t valid_leading_nibble[] = {
        1,1,1,1, 1,1,1,1, 0,0,0,0, 1,1,1,1
    };
    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; i++)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    for (; i < n - remaining; i += 8)
    {
        // Read 8 bytes to be processed in parallel
        uint64_t x;
        memcpy(&x, str + i, sizeof x);

        // Extract bytes that start with 0b10
        const uint64_t a =   x & 0x8080808080808080llu;
        const uint64_t b = (~x & 0x4040404040404040llu) << 1;

        // Each byte in c is either 0 or 0b10000000
        uint64_t c = a & b;

        uint32_t bit_count;
        #ifdef __clang__ // only Clang seems to benefit from popcount()
        bit_count = __builtin_popcountll(c);
        #else
        //https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        uint32_t v0 = c & 0xffffffffllu;
        uint32_t v1 = c >> 32;

        v0 = v0 - (v0 >> 1);
        v0 = (v0 & 0x33333333) + ((v0 >> 2) & 0x33333333);
        bit_count = (((v0 + (v0 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;

        v1 = v1 - (v1 >> 1);
        v1 = (v1 & 0x33333333) + ((v1 >> 2) & 0x33333333);
        bit_count += (((v1 + (v1 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
        #endif

        count += 8 - bit_count;
    }
    for (; i < n; i++)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    return count;
}

static size_t gp_bytes_codepoint_length(
    const void* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[*(uint8_t*)str >> 3];
}

bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size)
{
    size_t s1_length = gp_bytes_codepoint_count(s1, gp_str_length(s1));
    size_t s2_length = gp_bytes_codepoint_count(s2, s2_size);
    if (s1_length != s2_length)
        return false;

    mbstate_t state1 = {0};
    mbstate_t state2 = {0};
    wchar_t wc1;
    wchar_t wc2;
    for (size_t i = 0; i < s1_length; i++)
    {
        size_t wc1_length = mbrtowc(&wc1, (char*)s1, sizeof(wchar_t), &state1);
        size_t wc2_length = mbrtowc(&wc2, (char*)s2, sizeof(wchar_t), &state2);
        if (sizeof(wchar_t) < sizeof(uint32_t)/* Windows probably */&&
            (wc1_length == (size_t)-2) != (wc2_length == (size_t)-2))
        { // one fits to wchar_t and other doesn't so most likely different
            return false;
        }
        else if (sizeof(wchar_t) < sizeof(uint32_t) &&
                 wc1_length == (size_t)-2) // char wider than sizeof(wchar_t)
        {                                  // so just compare raw bytes
            size_t s1_codepoint_size = gp_bytes_codepoint_length(s1);
            size_t s2_codepoint_size = gp_bytes_codepoint_length(s2);
            if (s1_codepoint_size != s2_codepoint_size ||
                memcmp(s1, s2, s1_codepoint_size) != 0)
            {
                return false;
            }
            s1 += s1_codepoint_size;
            s2 += s2_codepoint_size;
        }
        else
        {
            wc1 = towlower(wc1);
            wc2 = towlower(wc2);
            if (wc1 != wc2)
                return false;

            s1 += wc1_length;
            s2 += wc2_length;
        }
    }
    return true;
}

size_t gp_str_codepoint_count(
    GPString str)
{
    return gp_bytes_codepoint_count(str, gp_str_length(str));
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
static bool gp_valid_codepoint(
    const uint32_t c)
{
    if (c <= 0x7Fu)
        return true;

    if (0xC280u <= c && c <= 0xDFBFu)
       return ((c & 0xE0C0u) == 0xC080u);

    if (0xEDA080u <= c && c <= 0xEDBFBFu)
       return 0; // Reject UTF-16 surrogates

    if (0xE0A080u <= c && c <= 0xEFBFBFu)
       return ((c & 0xF0C0C0u) == 0xE08080u);

    if (0xF0908080u <= c && c <= 0xF48FBFBFu)
       return ((c & 0xF8C0C0C0u) == 0xF0808080u);

    return false;
}

bool gp_str_is_valid(
    GPString _str,
    size_t* invalid_index)
{
    const char* str = (const char*)_str;
    const size_t length = gp_str_length(_str);
    for (size_t i = 0; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(str + i);
        if (cp_length == 0 || i + cp_length > length) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)str[i + j];
        if ( ! gp_valid_codepoint(codepoint)) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
        i += cp_length;
    }
    return true;
}

size_t gp_str_codepoint_length(
    GPString str)
{
    return gp_bytes_codepoint_length(str);
}

const char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0';
    return (const char*)str;
}

void gp_str_reserve(
    GPString* str,
    size_t capacity)
{
    *str = gp_arr_reserve(sizeof**str, *str, capacity + sizeof"");
    gp_str_header(*str)->capacity -= sizeof"";
}

void gp_str_copy(
    GPString* dest,
    const void*restrict src,
    size_t n)
{
    gp_str_reserve(dest, n);
    memcpy(*dest, src, n);
    gp_str_header(*dest)->length = n;
}

void gp_str_repeat(
    GPString* dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    gp_str_reserve(dest, n * mem_length);
    if (mem_length == 1) {
        memset(*dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(*dest + i * mem_length, mem, mem_length);
    }
    gp_str_header(*dest)->length = n * mem_length;
}

void gp_str_slice(
    GPString* dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    if (src != NULL) {
        gp_str_reserve(dest, end - start);
        memcpy(*dest, src + start, end - start);
        gp_str_header(*dest)->length = end - start;
    } else {
        memmove(*dest, *dest + start,  end - start);
        gp_str_header(*dest)->length = end - start;
    }
}

void gp_str_append(
    GPString* dest,
    const void* src,
    size_t src_length)
{
    gp_str_reserve(dest, gp_str_length(*dest) + src_length);
    memcpy(*dest + gp_str_length(*dest), src, src_length + sizeof"");
    gp_str_header(*dest)->length += src_length;
}

void gp_str_insert(
    GPString* dest,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    gp_str_reserve(dest, gp_str_length(*dest) + n);
    memmove(*dest + pos + n, *dest + pos, gp_str_length(*dest) - pos);
    memcpy(*dest + pos, src, n);
    gp_str_header(*dest)->length += n;
}

size_t gp_str_replace(
    GPString* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t start)
{
    if ((start = gp_str_find(*haystack, needle, needle_length, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;

    gp_str_reserve(haystack,
        gp_str_length(*haystack) + replacement_length - needle_length);

    const size_t end = start + needle_length;
    gp_str_header(*haystack)->length = gp_bytes_replace_range(
        *haystack,
        gp_str_length(*haystack),
        start,
        end,
        replacement,
        replacement_length);

    return start;
}

size_t gp_str_replace_all(
    GPString* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_str_find(*haystack, needle, needle_length, start)) != GP_NOT_FOUND)
    {
        gp_str_reserve(haystack,
            gp_str_length(*haystack) + replacement_length - needle_length);

        gp_str_header(*haystack)->length = gp_bytes_replace_range(
            *haystack,
            gp_str_length(*haystack),
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    return replacement_count;
}

static size_t gp_str_print_object_size(GPPrintable object, pf_va_list _args)
{
    va_list args;
    va_copy(args, _args.list);

    size_t length = 0;
    if (object.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args, char*);
        length = pf_vsnprintf(
            NULL,
            0,
            fmt,
            args);
    } else {
        switch (object.type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                length = 1;
                break;

            case GP_BOOL:
                length = strlen("false");
                break;

            char* p;
            size_t p_len;
            case GP_CHAR_PTR:
                p = va_arg(args, char*);
                p_len = strlen(p);
                length = p_len;
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args, GPString);
                length = gp_str_length(s);
                break;

            default:
                length = gp_max_digits_in(object.type);
        }
    }
    va_end(args);
    return length;
}

size_t gp_str_print_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    // Avoid many small allocations by estimating a sufficient buffer size. This
    // estimation is currently completely arbitrary.
    if (gp_str_allocator(*out) != NULL)
        gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        if (gp_str_allocator(*out) != NULL)
            gp_str_reserve(out, gp_str_length(*out) + gp_str_print_object_size(objs[i], args));
        gp_str_header(*out)->length += gp_bytes_print_objects(
            (size_t)-1,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    return gp_str_header(*out)->length;
}

size_t gp_str_n_print_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    gp_str_reserve(out, n);
    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_header(*out)->length += gp_bytes_print_objects(
            n >= gp_str_length(*out) ? n - gp_str_length(*out) : 0,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    const size_t out_length = gp_str_length(*out);
    if (out_length > n)
        gp_str_header(*out)->length = n;
    return out_length;
}

size_t gp_str_println_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    // Avoid many small allocations by estimating a sufficient buffer size. This
    // estimation is currently completely arbitrary.
    if (gp_str_allocator(*out) != NULL)
        gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        if (gp_str_allocator(*out) != NULL)
            gp_str_reserve(out,
                gp_str_length(*out) + strlen(" ") + gp_str_print_object_size(objs[i], args));

        gp_str_header(*out)->length += gp_bytes_print_objects(
            (size_t)-1,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);

        (*out)[gp_str_header(*out)->length++].c = ' ';
    }
    va_end(_args);
    va_end(args.list);

    (*out)[gp_str_length(*out) - 1].c = '\n';
    return gp_str_header(*out)->length;
}

size_t gp_str_n_println_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    gp_str_reserve(out, n);
    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_header(*out)->length += gp_bytes_print_objects(
            n >= gp_str_length(*out) ? n - gp_str_length(*out) : 0,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);

        if (n > gp_str_length(*out))
            (*out)[gp_str_header(*out)->length++].c = ' ';
    }
    va_end(_args);
    va_end(args.list);

    if (n > (gp_str_length(*out) - !!gp_str_length(*out))) // overwrite last space
        (*out)[gp_str_length(*out) - 1].c = '\n';

    const size_t out_length = gp_str_length(*out);
    if (out_length > n)
        gp_str_header(*out)->length = n;
    return out_length;
}

void gp_str_trim(
    GPString* str,
    const char*restrict optional_char_set,
    int flags)
{
    const bool ascii = flags & 0x01;
    if (ascii) {
        gp_str_header(*str)->length = gp_bytes_trim(
            *str, gp_str_length(*str), NULL, optional_char_set, flags);
        return;
    }
    // else utf8

    size_t      length   = gp_str_length(*str);
    const bool  left     = flags & 0x04;
    const bool  right    = flags & 0x02;
    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_WHITESPACE;

    if (left)
    {
        size_t prefix_length = 0;
        while (true)
        {
            char codepoint[8] = "";
            size_t size = gp_bytes_codepoint_length(*str + prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;

        memmove(*str, *str + prefix_length, length);
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_bytes_codepoint_length(*str + i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    gp_str_header(*str)->length = length;
}

static void gp_str_to_something(
    GPString* str,
    wint_t(*const towsomething)(wint_t))
{
    size_t length = gp_str_length(*str);

    wchar_t  stack_buf[1 << 10];
    size_t   buf_cap = sizeof stack_buf / sizeof*stack_buf;
    wchar_t* buf = stack_buf;
    if (length + sizeof"" > buf_cap) {
        buf_cap = length + sizeof"";
        buf = gp_mem_alloc(gp_heap, buf_cap * sizeof*buf);
    }
    const char* src = gp_cstr(*str);
    size_t buf_length = mbsrtowcs(buf, &src, buf_cap, &(mbstate_t){0});

    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    const wchar_t* pbuf = (const wchar_t*)buf;
    gp_str_reserve(str, wcsrtombs(NULL, &pbuf, 0, &(mbstate_t){0}));

    gp_str_header(*str)->length = wcsrtombs((char*)*str,
        &pbuf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        gp_mem_dealloc(gp_heap, buf);
}

void gp_str_to_upper(
    GPString* str)
{
    gp_str_to_something(str, towupper);
}

void gp_str_to_lower(
    GPString* str)
{
    gp_str_to_something(str, towlower);
}

static size_t gp_str_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 0 || i + cp_length > length)
            return i;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
        if ( ! gp_valid_codepoint(codepoint))
            return i;

        i += cp_length;
    }
    return GP_NOT_FOUND;
}

static size_t gp_str_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 1)
            return i;
        if (cp_length == 0)
            continue;

        if (cp_length + i < length) {
            uint32_t codepoint = 0;
            for (size_t j = 0; j < cp_length; j++)
                codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
            if (gp_valid_codepoint(codepoint))
                return i;
        } // else maybe there's ascii in last bytes so continue
    }
    return length;
}

void gp_str_to_valid(
    GPString* str,
    const char* replacement)
{
          size_t length = gp_str_length(*str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_str_find_invalid(*str, start, length)) != GP_NOT_FOUND)
    {
        const size_t end = gp_str_find_valid(*str, start, length);
        gp_str_reserve(str,
            gp_str_length(*str) + replacement_length - (end - start));

        length = gp_bytes_replace_range(
            *str,
            length,
            start,
            end,
            replacement,
            replacement_length);

        start += replacement_length;
    }
    gp_str_header(*str)->length = length;
}

int gp_str_case_compare(
    const GPString _s1,
    const GPString _s2)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;

    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[sizeof stack_buf1 / sizeof*stack_buf1];
    size_t  buf1_cap = sizeof stack_buf1 / sizeof*stack_buf1;
    size_t  buf2_cap = buf1_cap;
    wchar_t*buf1     = stack_buf1;
    wchar_t*buf2     = stack_buf2;

    GPArena arena;
    const GPAllocator* scope = NULL;
    const size_t max_length = gp_max(gp_str_length(_s1), gp_str_length(_s2));
    if (max_length + 1 >= buf1_cap)
    {
        arena = gp_arena_new(2 * max_length * sizeof*buf1 +/*internals*/64);
        scope = (const GPAllocator*)&arena;
    }
    if (gp_str_length(_s1) + 1 >= buf1_cap) {
        buf1_cap = gp_str_length(_s1) + 1;
        buf1 = gp_mem_alloc(scope, buf1_cap * sizeof(wchar_t));
    }
    if (gp_str_length(_s2) + 1 >= buf2_cap) {
        buf2_cap = gp_str_length(_s2) + 1;
        buf2 = gp_mem_alloc(scope, buf2_cap * sizeof(wchar_t));
    }
    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    int result = wcscoll(buf1, buf2);
    gp_arena_delete((GPArena*)scope);
    return result;
}

int gp_str_from_path(
    GPString*   str,
    const char* file_path)
{
    #if _WIN32
    struct __stat64 s;
    if (_stat64(file_path, &s) != 0)
    #elif _GNU_SOURCE
    struct stat64 s;
    if (stat64(file_path, &s) != 0)
    #else
    struct stat s;
    if (stat(file_path, &s) != 0)
    #endif
        return -1;

    if ((uint64_t)s.st_size > SIZE_MAX)
        return 1;

    FILE* f = fopen(file_path, "r");
    if (f == NULL)
        return -1;

    gp_str_reserve(str, s.st_size);
    if (fread(*str, sizeof**str, s.st_size, f) != (size_t)s.st_size) {
        fclose(f);
        return -1;
    }
    gp_str_header(*str)->length = s.st_size;

    fclose(f);
    return 0;
}




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               d2s.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.
//
// -DRYU_ONLY_64_BIT_OPS Avoid using uint128_t or 64-bit intrinsics. Slower,
//     depending on your compiler.
//
// -DRYU_OPTIMIZE_SIZE Use smaller lookup tables. Instead of storing every
//     required power of 5, only store every 26th entry, and compute
//     intermediate values with a multiplication. This reduces the lookup table
//     size by about 10x (only one case, and only double) at the cost of some
//     performance. Currently requires MSVC intrinsics.


#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// Include either the small or the full lookup tables depending on the mode.
#if defined(RYU_OPTIMIZE_SIZE)
#else
#endif

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

static inline uint32_t decimalLength17(const uint64_t v) {
  // This is slightly faster than a loop.
  // The average output length is 16.38 digits, so we check high-to-low.
  // Function precondition: v is not an 18, 19, or 20-digit number.
  // (17 digits are sufficient for round-tripping.)
  assert(v < 100000000000000000L);
  if (v >= 10000000000000000L) { return 17; }
  if (v >= 1000000000000000L) { return 16; }
  if (v >= 100000000000000L) { return 15; }
  if (v >= 10000000000000L) { return 14; }
  if (v >= 1000000000000L) { return 13; }
  if (v >= 100000000000L) { return 12; }
  if (v >= 10000000000L) { return 11; }
  if (v >= 1000000000L) { return 10; }
  if (v >= 100000000L) { return 9; }
  if (v >= 10000000L) { return 8; }
  if (v >= 1000000L) { return 7; }
  if (v >= 100000L) { return 6; }
  if (v >= 10000L) { return 5; }
  if (v >= 1000L) { return 4; }
  if (v >= 100L) { return 3; }
  if (v >= 10L) { return 2; }
  return 1;
}

// A floating decimal representing m * 10^e.
typedef struct floating_decimal_64 {
  uint64_t mantissa;
  // Decimal exponent's range is -324 to 308
  // inclusive, and can fit in a short if needed.
  int32_t exponent;
} floating_decimal_64;

static inline floating_decimal_64 d2d(const uint64_t ieeeMantissa, const uint32_t ieeeExponent) {
  int32_t e2;
  uint64_t m2;
  if (ieeeExponent == 0) {
    // We subtract 2 so that the bounds computation has 2 additional bits.
    e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = ieeeMantissa;
  } else {
    e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  }
  const bool even = (m2 & 1) == 0;
  const bool acceptBounds = even;

  // Step 2: Determine the interval of valid decimal representations.
  const uint64_t mv = 4 * m2;
  // Implicit bool -> int conversion. True is 1, false is 0.
  const uint32_t mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
  // We would compute mp and mm like this:
  // uint64_t mp = 4 * m2 + 2;
  // uint64_t mm = mv - 1 - mmShift;

  // Step 3: Convert to a decimal power base using 128-bit arithmetic.
  uint64_t vr, vp, vm;
  int32_t e10;
  bool vmIsTrailingZeros = false;
  bool vrIsTrailingZeros = false;
  if (e2 >= 0) {
    // I tried special-casing q == 0, but there was no effect on performance.
    // This expression is slightly faster than max(0, log10Pow2(e2) - 1).
    const uint32_t q = log10Pow2(e2) - (e2 > 3);
    e10 = (int32_t) q;
    const int32_t k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((int32_t) q) - 1;
    const int32_t i = -e2 + (int32_t) q + k;
    #if defined(RYU_OPTIMIZE_SIZE)
      uint64_t pow5[2];
      double_computeInvPow5(q, pow5);
      vr = mulShiftAll64(m2, pow5, i, &vp, &vm, mmShift);
    #else
      vr = mulShiftAll64(m2, DOUBLE_POW5_INV_SPLIT[q], i, &vp, &vm, mmShift);
    #endif

    if (q <= 21) {
      // This should use q <= 22, but I think 21 is also safe. Smaller values
      // may still be safe, but it's more difficult to reason about them.
      // Only one of mp, mv, and mm can be a multiple of 5, if any.
      const uint32_t mvMod5 = ((uint32_t) mv) - 5 * ((uint32_t) div5(mv));
      if (mvMod5 == 0) {
        vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
      } else if (acceptBounds) {
        // Same as min(e2 + (~mm & 1), pow5Factor(mm)) >= q
        // <=> e2 + (~mm & 1) >= q && pow5Factor(mm) >= q
        // <=> true && pow5Factor(mm) >= q, since e2 >= q.
        vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
      } else {
        // Same as min(e2 + 1, pow5Factor(mp)) >= q.
        vp -= multipleOfPowerOf5(mv + 2, q);
      }
    }
  } else {
    // This expression is slightly faster than max(0, log10Pow5(-e2) - 1).
    const uint32_t q = log10Pow5(-e2) - (-e2 > 1);
    e10 = (int32_t) q + e2;
    const int32_t i = -e2 - (int32_t) q;
    const int32_t k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
    const int32_t j = (int32_t) q - k;
    #if defined(RYU_OPTIMIZE_SIZE)
      uint64_t pow5[2];
      double_computePow5(i, pow5);
      vr = mulShiftAll64(m2, pow5, j, &vp, &vm, mmShift);
    #else
      vr = mulShiftAll64(m2, DOUBLE_POW5_SPLIT[i], j, &vp, &vm, mmShift);
    #endif

    if (q <= 1) {
      // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
      // mv = 4 * m2, so it always has at least two trailing 0 bits.
      vrIsTrailingZeros = true;
      if (acceptBounds) {
        // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
        vmIsTrailingZeros = mmShift == 1;
      } else {
        // mp = mv + 2, so it always has at least one trailing 0 bit.
        --vp;
      }
    } else if (q < 63) { // TODO(ulfjack): Use a tighter bound here.
      // We want to know if the full product has at least q trailing zeros.
      // We need to compute min(p2(mv), p5(mv) - e2) >= q
      // <=> p2(mv) >= q && p5(mv) - e2 >= q
      // <=> p2(mv) >= q (because -e2 >= q)
      vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
    }
  }

  // Step 4: Find the shortest decimal representation in the interval of valid representations.
  int32_t removed = 0;
  uint8_t lastRemovedDigit = 0;
  uint64_t output;
  // On average, we remove ~2 digits.
  if (vmIsTrailingZeros || vrIsTrailingZeros) {
    // General case, which happens rarely (~0.7%).
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      vmIsTrailingZeros &= vmMod10 == 0;
      vrIsTrailingZeros &= lastRemovedDigit == 0;
      lastRemovedDigit = (uint8_t) vrMod10;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }

    if (vmIsTrailingZeros) {
      for (;;) {
        const uint64_t vmDiv10 = div10(vm);
        const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
        if (vmMod10 != 0) {
          break;
        }
        const uint64_t vpDiv10 = div10(vp);
        const uint64_t vrDiv10 = div10(vr);
        const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t) vrMod10;
        vr = vrDiv10;
        vp = vpDiv10;
        vm = vmDiv10;
        ++removed;
      }
    }
    if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
      // Round even if the exact number is .....50..0.
      lastRemovedDigit = 4;
    }
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~99.3%). Percentages below are relative to this.
    bool roundUp = false;
    const uint64_t vpDiv100 = div100(vp);
    const uint64_t vmDiv100 = div100(vm);
    if (vpDiv100 > vmDiv100) { // Optimization: remove two digits at a time (~86.2%).
      const uint64_t vrDiv100 = div100(vr);
      const uint32_t vrMod100 = ((uint32_t) vr) - 100 * ((uint32_t) vrDiv100);
      roundUp = vrMod100 >= 50;
      vr = vrDiv100;
      vp = vpDiv100;
      vm = vmDiv100;
      removed += 2;
    }
    // Loop iterations below (approximately), without optimization above:
    // 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
    // Loop iterations below (approximately), with optimization above:
    // 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      roundUp = vrMod10 >= 5;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + (vr == vm || roundUp);
  }
  const int32_t exp = e10 + removed;

  floating_decimal_64 fd;
  fd.exponent = exp;
  fd.mantissa = output;
  return fd;
}

static inline int to_chars(const floating_decimal_64 v, const bool sign, char* const result) {
  // Step 5: Print the decimal representation.
  int index = 0;
  if (sign) {
    result[index++] = '-';
  }

  uint64_t output = v.mantissa;
  const uint32_t olength = decimalLength17(output);


  // Print the decimal digits.
  // The following code is equivalent to:
  // for (uint32_t i = 0; i < olength - 1; ++i) {
  //   const uint32_t c = output % 10; output /= 10;
  //   result[index + olength - i] = (char) ('0' + c);
  // }
  // result[index] = '0' + output % 10;

  uint32_t i = 0;
  // We prefer 32-bit operations, even on 64-bit platforms.
  // We have at most 17 digits, and uint32_t can store 9 digits.
  // If output doesn't fit into uint32_t, we cut off 8 digits,
  // so the rest will fit into uint32_t.
  if ((output >> 32) != 0) {
    // Expensive 64-bit division.
    const uint64_t q = div1e8(output);
    uint32_t output2 = ((uint32_t) output) - 100000000 * ((uint32_t) q);
    output = q;

    const uint32_t c = output2 % 10000;
    output2 /= 10000;
    const uint32_t d = output2 % 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    const uint32_t d0 = (d % 100) << 1;
    const uint32_t d1 = (d / 100) << 1;
    memcpy(result + index + olength - 1, DIGIT_TABLE + c0, 2);
    memcpy(result + index + olength - 3, DIGIT_TABLE + c1, 2);
    memcpy(result + index + olength - 5, DIGIT_TABLE + d0, 2);
    memcpy(result + index + olength - 7, DIGIT_TABLE + d1, 2);
    i += 8;
  }
  uint32_t output2 = (uint32_t) output;
  while (output2 >= 10000) {
    #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
      const uint32_t c = output2 - 10000 * (output2 / 10000);
    #else
      const uint32_t c = output2 % 10000;
    #endif
    output2 /= 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    memcpy(result + index + olength - i - 1, DIGIT_TABLE + c0, 2);
    memcpy(result + index + olength - i - 3, DIGIT_TABLE + c1, 2);
    i += 4;
  }
  if (output2 >= 100) {
    const uint32_t c = (output2 % 100) << 1;
    output2 /= 100;
    memcpy(result + index + olength - i - 1, DIGIT_TABLE + c, 2);
    i += 2;
  }
  if (output2 >= 10) {
    const uint32_t c = output2 << 1;
    // We can't use memcpy here: the decimal dot goes between these two digits.
    result[index + olength - i] = DIGIT_TABLE[c + 1];
    result[index] = DIGIT_TABLE[c];
  } else {
    result[index] = (char) ('0' + output2);
  }

  // Print decimal point if needed.
  if (olength > 1) {
    result[index + 1] = '.';
    index += olength + 1;
  } else {
    ++index;
  }

  // Print the exponent.
  result[index++] = 'E';
  int32_t exp = v.exponent + (int32_t) olength - 1;
  if (exp < 0) {
    result[index++] = '-';
    exp = -exp;
  }

  if (exp >= 100) {
    const int32_t c = exp % 10;
    memcpy(result + index, DIGIT_TABLE + 2 * (exp / 10), 2);
    result[index + 2] = (char) ('0' + c);
    index += 3;
  } else if (exp >= 10) {
    memcpy(result + index, DIGIT_TABLE + 2 * exp, 2);
    index += 2;
  } else {
    result[index++] = (char) ('0' + exp);
  }

  return index;
}

static inline bool d2d_small_int(const uint64_t ieeeMantissa, const uint32_t ieeeExponent,
  floating_decimal_64* const v) {
  const uint64_t m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  const int32_t e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;

  if (e2 > 0) {
    // f = m2 * 2^e2 >= 2^53 is an integer.
    // Ignore this case for now.
    return false;
  }

  if (e2 < -52) {
    // f < 1.
    return false;
  }

  // Since 2^52 <= m2 < 2^53 and 0 <= -e2 <= 52: 1 <= f = m2 / 2^-e2 < 2^53.
  // Test if the lower -e2 bits of the significand are 0, i.e. whether the fraction is 0.
  const uint64_t mask = (1ull << -e2) - 1;
  const uint64_t fraction = m2 & mask;
  if (fraction != 0) {
    return false;
  }

  // f is an integer in the range [1, 2^53).
  // Note: mantissa might contain trailing (decimal) 0's.
  // Note: since 2^53 < 10^16, there is no need to adjust decimalLength17().
  v->mantissa = m2 >> -e2;
  v->exponent = 0;
  return true;
}

int d2s_buffered_n(double f, char* result) {
  // Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
  const uint64_t bits = double_to_bits(f);

  // Decode bits into sign, mantissa, and exponent.
  const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
  const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t ieeeExponent = (uint32_t) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
  // Case distinction; exit early for the easy cases.
  if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u) || (ieeeExponent == 0 && ieeeMantissa == 0)) {
    return copy_special_str(result, ieeeSign, ieeeExponent, ieeeMantissa);
  }

  floating_decimal_64 v;
  const bool isSmallInt = d2d_small_int(ieeeMantissa, ieeeExponent, &v);
  if (isSmallInt) {
    // For small integers in the range [1, 2^53), v.mantissa might contain trailing (decimal) zeros.
    // For scientific notation we need to move these zeros into the exponent.
    // (This is not needed for fixed-point notation, so it might be beneficial to trim
    // trailing zeros in to_chars only if needed - once fixed-point notation output is implemented.)
    for (;;) {
      const uint64_t q = div10(v.mantissa);
      const uint32_t r = ((uint32_t) v.mantissa) - 10 * ((uint32_t) q);
      if (r != 0) {
        break;
      }
      v.mantissa = q;
      ++v.exponent;
    }
  } else {
    v = d2d(ieeeMantissa, ieeeExponent);
  }

  return to_chars(v, ieeeSign, result);
}

void d2s_buffered(double f, char* result) {
  const int index = d2s_buffered_n(f, result);

  // Terminate the string.
  result[index] = '\0';
}

#if 0 // Not used so shut up analyzer
char* d2s(double f) {
  char* const result = (char*) malloc(25);
  d2s_buffered(f, result);
  return result;
}
#endif




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               overload.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md


extern inline size_t gp_sizeof     (const GPType T);
extern inline bool   gp_is_unsigned(const GPType T);
extern inline bool   gp_is_integer (const GPType T);
extern inline bool   gp_is_floating(const GPType T);
extern inline bool   gp_is_pointer (const GPType T);





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               printf.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/printf.h>
#include <printf/format_scanning.h>
#include <printf/conversions.h>


#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>

struct MiscData
{
    bool has_sign;
    bool has_0x;
    bool is_nan_or_inf;
};

static uintmax_t get_uint(pf_va_list args[static 1], const PFFormatSpecifier fmt)
{
    if (fmt.conversion_format == 'p')
        return va_arg(args->list, uintptr_t);

    switch (fmt.length_modifier)
    {
        case 'j':
            return va_arg(args->list, uintmax_t);

        case 'l' * 2:
            return va_arg(args->list, unsigned long long);

        case 'l':
            return va_arg(args->list, unsigned long);

        case 'h':
            return (unsigned short)va_arg(args->list, unsigned);

        case 'h' * 2:
            return (unsigned char)va_arg(args->list, unsigned);

        case 'z':
            return (size_t)va_arg(args->list, size_t);

        case 'B': // byte
            return (uint8_t)va_arg(args->list, unsigned);

        case 'W': // word
            return (uint16_t)va_arg(args->list, unsigned);

        case 'D': // double word
            return (uint32_t)va_arg(args->list, uint32_t);

        case 'Q': // quad word
            return (uint64_t)va_arg(args->list, uint64_t);

        default:
            return va_arg(args->list, unsigned);
    }
}

static void string_padding(
    struct pf_string out[static 1],
    const PFFormatSpecifier fmt,
    const void* string,
    const size_t length)
{
    const unsigned field_width = fmt.field.width > length ?
        fmt.field.width : length;
    const unsigned diff = field_width - length;
    if (fmt.flag.dash) // left justified
    { // first string, then pad
        pf_concat(out, string, length);
        pf_pad(out, ' ', diff);
    }
    else // first pad, then string
    {
        pf_pad(out, ' ', diff);
        pf_concat(out, string, length);
    }

}

static unsigned write_s(
    struct pf_string out[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const char* cstr = va_arg(args->list, const char*);

    size_t cstr_len = 0;
    if (fmt.precision.option == PF_NONE) // should be null-terminated
        cstr_len = strlen(cstr);
    else // who knows if null-terminated
        while (cstr_len < fmt.precision.width && cstr[cstr_len] != '\0')
            cstr_len++;

    string_padding(out, fmt, cstr, cstr_len);
    return out->length - original_length;
}

static unsigned write_S(
    struct pf_string out[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const GPString str = va_arg(args->list, GPString);
    size_t length = gp_str_length(str);
    if (fmt.precision.option != PF_NONE)
        length = pf_min(length, fmt.precision.width);

    string_padding(out, fmt, str, length);
    return out->length - original_length;
}

static void write_leading_zeroes(
    struct pf_string out[static 1],
    const unsigned written_by_utoa,
    const PFFormatSpecifier fmt)
{
    if (fmt.precision.option != PF_NONE)
    {
        const unsigned diff =
            fmt.precision.width <= written_by_utoa ? 0 :
            fmt.precision.width - written_by_utoa;
        memmove(
            out->data + out->length + diff,
            out->data + out->length,
            pf_limit(*out, written_by_utoa));
        memset(out->data + out->length, '0', pf_limit(*out, diff));
        out->length += written_by_utoa + diff;
    }
    else
    {
        out->length += written_by_utoa;
    }
}

static unsigned write_i(
    struct pf_string out[static 1],
    struct MiscData md[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    intmax_t i;
    switch (fmt.length_modifier)
    {
        case 'j':
            i = va_arg(args->list, intmax_t);
            break;

        case 'l' * 2:
            i = va_arg(args->list, long long);
            break;

        case 'l':
            i = va_arg(args->list, long);
            break;

        case 'h':
            i = (short)va_arg(args->list, int);
            break;

        case 'h' * 2: // signed char is NOT char!
            i = (signed char)va_arg(args->list, int);
            break;

        case 't':
            i = (ptrdiff_t)va_arg(args->list, ptrdiff_t);
            break;

        case 'B': // byte
            i = (int8_t)va_arg(args->list, int);
            break;

        case 'W': // word
            i = (int16_t)va_arg(args->list, int);
            break;

        case 'D': // double word
            i = (int32_t)va_arg(args->list, int32_t);
            break;

        case 'Q': // quad word
            i = (int64_t)va_arg(args->list, int64_t);
            break;

        default:
            i = va_arg(args->list, int);
    }

    const size_t original_length = out->length;

    const char sign = i < 0 ? '-' : fmt.flag.plus ? '+' : fmt.flag.space ? ' ' : 0;
    if (sign)
    {
        pf_push_char(out, sign);
        md->has_sign = true;
    }

    const unsigned max_written = pf_utoa(
        pf_capacity_left(*out), out->data + out->length, imaxabs(i));

    write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned write_o(
    struct pf_string out[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = get_uint(args, fmt);

    bool zero_written = false;
    if (fmt.flag.hash && u > 0)
    {
        pf_push_char(out, '0');
        zero_written = true;
    }

    const unsigned max_written = pf_otoa(
        pf_capacity_left(*out), out->data + out->length, u);

    // zero_written tells pad_zeroes() to add 1 less '0'
    write_leading_zeroes(out, zero_written + max_written, fmt);
    // compensate for added zero_written to write_leading_zeroes()
    out->length -= zero_written;

    return out->length - original_length;
}

static unsigned write_x(
    struct pf_string out[static 1],
    struct MiscData md[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = get_uint(args, fmt);

    if (fmt.flag.hash && u > 0)
    {
        pf_concat(out, "0x", strlen("0x"));
        md->has_0x = true;
    }

    const unsigned max_written = pf_xtoa(
        pf_capacity_left(*out), out->data + out->length, u);

    write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned write_X(
    struct pf_string out[static 1],
    struct MiscData md[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = get_uint(args, fmt);

    if (fmt.flag.hash && u > 0)
    {
        pf_concat(out, "0X", strlen("0X"));
        md->has_0x = true;
    }

    const unsigned max_written = pf_Xtoa(
        pf_capacity_left(*out), out->data + out->length, u);

    write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned write_u(
    struct pf_string out[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = get_uint(args, fmt);
    const unsigned max_written = pf_utoa(
        pf_capacity_left(*out), out->data + out->length, u);
    write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned write_p(
    struct pf_string out[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = get_uint(args, fmt);

    if (u > 0)
    {
        pf_concat(out, "0x", strlen("0x"));
        const unsigned max_written = pf_xtoa(
            pf_capacity_left(*out), out->data + out->length, u);
        write_leading_zeroes(out, max_written, fmt);
    }
    else
    {
        pf_concat(out, "(nil)", strlen("(nil)"));
    }
    return out->length - original_length;
}

static unsigned write_f(
    struct pf_string out[static 1],
    struct MiscData md[static 1],
    pf_va_list args[static 1],
    const PFFormatSpecifier fmt)
{
    const double f = va_arg(args->list, double);
    const unsigned written_by_conversion = pf_strfromd(
        out->data + out->length, out->capacity, fmt, f);
    out->length += written_by_conversion;

    md->has_sign = signbit(f) || fmt.flag.plus || fmt.flag.space;
    md->is_nan_or_inf = isnan(f) || isinf(f);

    return written_by_conversion;
}

static unsigned add_padding(
    struct pf_string out[static 1],
    const unsigned written,
    const struct MiscData md,
    const PFFormatSpecifier fmt)
{
    size_t start = out->length - written;
    const unsigned diff = fmt.field.width - written;

    const bool is_int_with_precision =
        strchr("diouxX", fmt.conversion_format) && fmt.precision.option != PF_NONE;
    const bool ignore_zero = is_int_with_precision || md.is_nan_or_inf;

    if (fmt.flag.dash) // left justified, append padding
    {
        pf_pad(out, ' ', diff);
    }
    else if (fmt.flag.zero && ! ignore_zero) // fill in zeroes
    { // 0-padding minding "0x" or sign prefix
        const unsigned offset = md.has_sign + 2 * md.has_0x;
        pf_insert_pad(out, start + offset, '0', diff);
    }
    else // fill in spaces
    {
        pf_insert_pad(out, start, ' ', diff);
    }

    return diff;
}



// ---------------------------------------------------------------------------
//
//
//
// IMPLEMENTATIONS OF PUBLIC FUNCTIONS
//
//
//
// ---------------------------------------------------------------------------



// ------------------------------
// String functtions

int pf_vsnprintf_consuming(
    char*restrict out_buf,
    const size_t max_size,
    const char format[restrict static 1],
    pf_va_list* args)
{
    struct pf_string out = { out_buf ? out_buf : "", .capacity = max_size };

    while (1)
    {
        const PFFormatSpecifier fmt = pf_scan_format_string(format, args);
        if (fmt.string == NULL)
            break;

        pf_concat(&out, format, fmt.string - format);

        // Jump over format specifier for next iteration
        format = fmt.string + fmt.string_length;

        unsigned written_by_conversion = 0;
        struct MiscData misc = {};

        switch (fmt.conversion_format)
        {
            case 'c':
                pf_push_char(&out, (char)va_arg(args->list, int));
                written_by_conversion = 1;
                break;

            case 's':
                written_by_conversion += write_s(
                    &out, args, fmt);
                break;

            case 'S':
                written_by_conversion += write_S(
                    &out, args, fmt);
                break;

            case 'd':
            case 'i':
                written_by_conversion += write_i(
                    &out, &misc, args, fmt);
                break;

            case 'o':
                written_by_conversion += write_o(
                    &out, args, fmt);
                break;

            case 'x':
                written_by_conversion += write_x(
                    &out, &misc, args, fmt);
                break;

            case 'X':
                written_by_conversion += write_X(
                    &out, &misc, args, fmt);
                break;

            case 'u':
                written_by_conversion += write_u(
                    &out, args, fmt);
                break;

            case 'p':
                written_by_conversion += write_p(
                    &out, args, fmt);
                break;

            case 'f': case 'F':
            case 'e': case 'E':
            case 'g': case 'G':
                written_by_conversion += write_f(
                    &out, &misc, args, fmt);
                break;

            case '%':
                pf_push_char(&out, '%');
                break;
        }

        if (written_by_conversion < fmt.field.width)
            add_padding(
                &out,
                written_by_conversion,
                misc,
                fmt);
    }

    // Write what's left in format string
    pf_concat(&out, format, strlen(format));

    return out.length;
}

int pf_vsnprintf(
    char* restrict out_buf,
    const size_t max_size,
    const char format[restrict static 1],
    va_list _args)
{
    pf_va_list args;
    va_copy(args.list, _args);
    int result = pf_vsnprintf_consuming(out_buf, max_size, format, &args);
    va_end(args.list);
    return result;
}

int pf_vsprintf(
    char buf[restrict static 1], const char fmt[restrict static 1], va_list args)
{
    return pf_vsnprintf(buf, SIZE_MAX, fmt, args);
}

__attribute__((format (printf, 2, 3)))
int pf_sprintf(char buf[restrict static 1], const char fmt[restrict static 1], ...)
{
    va_list args;
    va_start(args, fmt);
    int written = pf_vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);
    return written;
}

__attribute__((format (printf, 3, 4)))
int pf_snprintf(
    char* restrict buf, const size_t n, const char fmt[restrict static 1], ...)
{
    va_list args;
    va_start(args, fmt);
    int written = pf_vsnprintf(buf, n, fmt, args);
    va_end(args);
    return written;
}

// ------------------------------
// IO functtions

#define PAGE_SIZE 4096
#define BUF_SIZE (PAGE_SIZE + sizeof(""))

int pf_vfprintf(
    FILE stream[restrict static 1], const char fmt[restrict static 1], va_list args)
{
    char buf[BUF_SIZE];
    char* pbuf = buf;
    va_list args_copy;
    va_copy(args_copy, args);

    const int out_length = pf_vsnprintf(buf, BUF_SIZE, fmt, args);
    if (out_length >= (int)BUF_SIZE) // try again
    {
        pbuf = malloc(out_length + sizeof(""));
        pf_vsprintf(pbuf, fmt, args_copy);
    }
    fwrite(pbuf, sizeof(char), out_length, stream);

    if (pbuf != buf)
        free(pbuf);
    va_end(args_copy);
    return out_length;
}

int pf_vprintf(
    const char fmt[restrict static 1], va_list args)
{
    return pf_vfprintf(stdout, fmt, args);
}

__attribute__((format (printf, 1, 2)))
int pf_printf(
    const char fmt[restrict static 1], ...)
{
    va_list args;
    va_start(args, fmt);
    int n = pf_vfprintf(stdout, fmt, args);
    va_end(args);
    return n;
}

__attribute__((format (printf, 2, 3)))
int pf_fprintf(
    FILE stream[restrict static 1], const char fmt[restrict static 1], ...)
{
    va_list args;
    va_start(args, fmt);
    int n = pf_vfprintf(stream, fmt, args);
    va_end(args);
    return n;
}






// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               hashmap.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>

const union gp_endianness_detector GP_INTEGER = {.u16 = 1 };

extern inline uint64_t* gp_u128_lo(const GPUint128*t);
extern inline uint64_t* gp_u128_hi(const GPUint128*t);
#if !(__GNUC__ && __SIZEOF_INT128__) // unused static function
static void gp_mult64to128(
    uint64_t u, uint64_t v, uint64_t* h, uint64_t* l)
{
    uint64_t u1 = (u & 0xffffffff);
    uint64_t v1 = (v & 0xffffffff);
    uint64_t t = (u1 * v1);
    uint64_t w3 = (t & 0xffffffff);
    uint64_t k = (t >> 32);

    u >>= 32;
    t = (u * v1) + k;
    k = (t & 0xffffffff);
    uint64_t w1 = (t >> 32);

    v >>= 32;
    t = (u1 * v) + k;
    k = (t >> 32);

    *h = (u * v) + w1 + k;
    *l = (t << 32) + w3;
}
#endif
static void gp_mult128(const GPUint128 N, const GPUint128 M, GPUint128*const Ans)
{
    #if __GNUC__ && __SIZEOF_INT128__
    Ans->u128 = N.u128 * M.u128;
    #else
    gp_mult64to128(*gp_u128_lo(&N), *gp_u128_lo(&M), gp_u128_hi(Ans), gp_u128_lo(Ans));
    *gp_u128_hi(Ans) += *gp_u128_hi(&N) * *gp_u128_lo(&M) + *gp_u128_lo(&N) * *gp_u128_hi(&M);
    #endif
}

uint32_t gp_bytes_hash32(const void* str, const size_t str_size)
{
    const uint32_t FNV_prime        = 0x01000193;
    const uint32_t FNV_offset_basis = 0x811c9dc5;
    const uint8_t* ustr = str;

    uint32_t hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        hash ^= ustr[i];
        hash *= FNV_prime;
    }
    return hash;
}

uint64_t gp_bytes_hash64(const void* str, const size_t str_size)
{
    const uint64_t FNV_prime        = 0x00000100000001B3;
    const uint64_t FNV_offset_basis = 0xcbf29ce484222325;
    const uint8_t* ustr = str;

    uint64_t hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        hash ^= ustr[i];
        hash *= FNV_prime;
    }
    return hash;
}

GPUint128 gp_bytes_hash128(const void* str, const size_t str_size)
{
    GPUint128 FNV_prime            = {0};
    GPUint128 FNV_offset_basis     = {0};
    *gp_u128_hi(&FNV_prime)        = 0x0000000001000000;
    *gp_u128_lo(&FNV_prime)        = 0x000000000000013B;
    *gp_u128_hi(&FNV_offset_basis) = 0x6c62272e07bb0142;
    *gp_u128_lo(&FNV_offset_basis) = 0x62b821756295c58d;
    const uint8_t* ustr = str;

    GPUint128 hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        *gp_u128_lo(&hash) ^= ustr[i];
        gp_mult128(hash, FNV_prime, &hash);
    }
    return hash;
}

// ----------------------------------------------------------------------------

struct gp_map
{
    const size_t length; // number of slots
    const size_t element_size; // if 0, elements is in GPSlot
    const GPAllocator*const allocator;
    void (*const destructor)(void* element); // may be NULL
};

struct gp_hash_map
{
    struct gp_map map;
};

#define GP_EMPTY  ((uintptr_t) 0)
#define GP_IN_USE ((uintptr_t)-1)
typedef struct gp_slot
{
    GPUint128 key;
    union {
        uintptr_t slot;
        void*     slots;
    };
    const void* element;
} GPSlot;

// GPMap in memory:
// |GPMap|Slot 0|Slot 1|...|Slot n|Element 0|Element 1|...|Element n|
//
// Subsequent slots in memory in case of collissions:
// |New slot 0|...|New slot n/2|New element 1|...|New element n/2|
// ^
// Slot i info points here where i is the index of the colliding slot.
//
// If GPMap.element_not_pointer, element is in Slot array, not element array.

static void gp_no_op_destructor(void*_) { (void)_; }

GPMap* gp_map_new(const GPAllocator* allocator, const GPMapInitializer*_init)
{
    static const size_t DEFAULT_CAP = 1 << 8; // somewhat arbitrary atm
    static const GPMapInitializer defaults = {
        .element_size = sizeof(void*),
        .capacity     = DEFAULT_CAP,
    };
    const GPMapInitializer* init = _init == NULL ? &defaults : _init;

    const size_t length = init->capacity == 0 ?
        DEFAULT_CAP
      : gp_next_power_of_2(init->capacity) >> 1;

    const GPMap init_map = {
        .length       = length,
        .element_size = init->element_size,
        .allocator    = allocator,
        .destructor   = init->destructor == NULL ?
            gp_no_op_destructor
          : init->destructor
    };
    GPMap* block = gp_mem_alloc_zeroes(allocator,
        sizeof init_map + length * sizeof(GPSlot) + length * init->element_size);
    return memcpy(block, &init_map, sizeof init_map);
}

static inline size_t gp_next_length(const size_t length)
{
    return length/2 < 4 ? 4 : length/2;
}
static inline GPUint128 gp_shift_key(const GPUint128 key, const size_t length)
{
    #if __GNUC__ && __SIZEOF_INT128__
    if      (sizeof length == sizeof(unsigned))
        return (GPUint128){.u128 =
            key.u128 >> (sizeof(int)  * CHAR_BIT -__builtin_clz  (length) - 1)};
    else if (sizeof length == sizeof(long))
        return (GPUint128){.u128 =
            key.u128 >> (sizeof(long) * CHAR_BIT -__builtin_clzl (length) - 1)};
    return
        (GPUint128){.u128 =
        key.u128 >> (sizeof(long long)* CHAR_BIT -__builtin_clzll(length) - 1)};
    #else

    // Find bit width of length which is assumed to be a power of 2.
    // https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
    static const uint64_t b[] = {
        0xAAAAAAAAAAAAAAAA, 0xCCCCCCCCCCCCCCCC, 0xF0F0F0F0F0F0F0F0,
        0xFF00FF00FF00FF00, 0xFFFF0000FFFF0000, 0xFFFFFFFF00000000
    };
    uint64_t
    bitw  =  ((uint64_t)length & b[0]) != 0;
    bitw |= (((uint64_t)length & b[5]) != 0) << 5;
    bitw |= (((uint64_t)length & b[4]) != 0) << 4;
    bitw |= (((uint64_t)length & b[3]) != 0) << 3;
    bitw |= (((uint64_t)length & b[2]) != 0) << 2;
    bitw |= (((uint64_t)length & b[1]) != 0) << 1;

    // 128-bit bit shift right
    GPUint128 new_key = {0};
    *gp_u128_hi(&new_key) = *gp_u128_hi(&key) >> bitw;
    *gp_u128_lo(&new_key) =(*gp_u128_lo(&key) >> bitw) | (*gp_u128_hi(&key)<<(64-bitw));

    return new_key;
    #endif
}

void gp_map_delete_elems(
    GPMap*const  map,
    GPSlot*const slots,
    const size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (slots[i].slot == GP_IN_USE)
        {
            map->destructor(map->element_size == 0 ?
                (void*)slots[i].element
              : (uint8_t*)(slots + length) + i * map->element_size);
        }
        else if (slots[i].slot != GP_EMPTY)
        {
            gp_map_delete_elems(map, slots[i].slots, gp_next_length(length));
        }
    }
    if (slots != (GPSlot*)(map + 1))
        gp_mem_dealloc(map->allocator, slots);
    else
        gp_mem_dealloc(map->allocator, map);
}

void gp_map_delete(GPMap* map)
{
    gp_map_delete_elems(map, (GPSlot*)(map + 1), map->length);
}

static void gp_map_set_elem(
    const GPAllocator*const allocator,
    GPSlot*const            slots,
    const size_t            length,
    const GPUint128         key,
    const void*const        elem,
    const size_t            elem_size)
{
    uint8_t* values = (uint8_t*)(slots + length);
    const size_t i  = *gp_u128_lo(&key) & (length - 1);

    if (slots[i].slot == GP_EMPTY)
    {
        if (elem_size != 0)
            memcpy(values + i * elem_size, elem, elem_size);
        else
            slots[i].element = elem;
        slots[i].slot = GP_IN_USE;
        slots[i].key  = key;
        return;
    }
    const size_t next_length = gp_next_length(length);
    if (slots[i].slot == GP_IN_USE)
    {
        GPSlot* new_slots = gp_mem_alloc_zeroes(allocator,
            next_length * sizeof*new_slots + next_length * elem_size);

        gp_map_set_elem(
            allocator,
            new_slots,
            next_length,
            gp_shift_key(slots[i].key, length),
            elem_size != 0 ? values + i * elem_size : slots[i].element,
            elem_size);

        slots[i].slots = new_slots;
    }
    gp_map_set_elem(
        allocator,
        slots[i].slots,
        next_length,
        gp_shift_key(key, length),
        elem,
        elem_size);
}

void gp_map_set(
    GPMap* map,
    GPUint128 key,
    const void* value)
{
    gp_map_set_elem(
        map->allocator,
        (GPSlot*)(map + 1),
        map->length,
        key,
        value,
        map->element_size);
}

static void* gp_map_get_elem(
    const GPSlot*const slots,
    const size_t length,
    const GPUint128 key,
    const size_t elem_size)
{
    uint8_t* values = (uint8_t*)(slots + length);
    const size_t i  = *gp_u128_lo(&key) & (length - 1);
    if (slots[i].slot == GP_IN_USE)
        return elem_size != 0 ? values + i * elem_size : (void*)slots[i].element;
    else if (slots[i].slot == GP_EMPTY)
        return NULL;

    return gp_map_get_elem(
        slots[i].slots, gp_next_length(length), gp_shift_key(key, length), elem_size);
}

void* gp_map_get(GPMap* map, GPUint128 key)
{
    return gp_map_get_elem(
        (GPSlot*)(map + 1),
        map->length,
        key,
        map->element_size);
}

static bool gp_map_remove_elem(
    GPSlot*const slots,
    const size_t length,
    const GPUint128 key,
    const size_t elem_size,
    void (*const destructor)(void*))
{
    const size_t i  = *gp_u128_lo(&key) & (length - 1);
    if (slots[i].slot == GP_IN_USE) {
        slots[i].slot = GP_EMPTY;
        destructor(elem_size == 0 ?
            (void*)slots[i].element
          : (uint8_t*)(slots + length) + i * elem_size);
        return true;
    }
    else if (slots[i].slot == GP_EMPTY) {
        return false;
    }
    return gp_map_remove_elem(
        slots, gp_next_length(length), gp_shift_key(key, length), elem_size, destructor);
}

bool gp_map_remove(GPMap* map, GPUint128 key)
{
    return gp_map_remove_elem(
        (GPSlot*)(map + 1),
        map->length,
        key,
        map->element_size,
        map->destructor);
}

GPHashMap* gp_hash_map_new(const GPAllocator* alc, const GPMapInitializer* init)
{
    return (GPHashMap*)gp_map_new(alc, init);
}

void gp_hash_map_delete(GPHashMap* map) { gp_map_delete((GPMap*)map); }

void gp_hash_map_set(
    GPHashMap*  map,
    const void* key,
    size_t      key_size,
    const void* value)
{
    gp_map_set((GPMap*)map, gp_bytes_hash128(key, key_size), value);
}

// Returns NULL if not found
void* gp_hash_map_get(
    GPHashMap*  map,
    const void* key,
    size_t      key_size)
{
    return gp_map_get((GPMap*)map, gp_bytes_hash128(key, key_size));
}

bool gp_hash_map_remove(
    GPHashMap*  map,
    const void* key,
    size_t      key_size)
{
    return gp_map_remove((GPMap*)map, gp_bytes_hash128(key, key_size));
}






// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               io.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <printf/printf.h>

extern inline int gp_stat(GPStat* s, const char* path);

static size_t gp_print_va_arg(
    FILE* out,
    pf_va_list*restrict const args,
    const GPType type)
{
    size_t length = 0;
    switch (type)
    {
        case GP_CHAR:
        case GP_SIGNED_CHAR:
        case GP_UNSIGNED_CHAR:
            length = 1;
            fputc(va_arg(args->list, int), out);
            break;

        case GP_UNSIGNED_SHORT:
        case GP_UNSIGNED:
            length = fprintf(out, "%u", va_arg(args->list, unsigned));
            break;

        case GP_UNSIGNED_LONG:
            length = fprintf(out, "%lu", va_arg(args->list, unsigned long));
            break;

        case GP_UNSIGNED_LONG_LONG:
            length = fprintf(out, "%llu", va_arg(args->list, unsigned long long));
            break;

        case GP_BOOL:
            if (va_arg(args->list, int)) {
                length = strlen("true");
                fputs("true", out);
            } else {
                length = strlen("false");
                fputs("false", out);
            } break;

        case GP_SHORT:
        case GP_INT:
            length = fprintf(out, "%i", va_arg(args->list, int));
            break;

        case GP_LONG:
            length = fprintf(out, "%li", va_arg(args->list, long));
            break;

        case GP_LONG_LONG:
            length = fprintf(out, "%lli", va_arg(args->list, long long));
            break;

        case GP_FLOAT:
        case GP_DOUBLE:
            length = fprintf(out, "%g", va_arg(args->list, double));
            break;

        case GP_CHAR_PTR:
            length = fprintf(out, "%s", va_arg(args->list, char*));
            break;

        GPString s;
        case GP_STRING:
            s = va_arg(args->list, GPString);
            length = gp_arr_length(s);
            fwrite(s, 1, length, out);
            break;

        case GP_PTR:
            length = fprintf(out, "%p", va_arg(args->list, void*));
            break;
    }
    return length;
}

static size_t gp_print_objects(
    FILE* out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj)
{
    size_t length = 0;
    if (obj.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args->list, char*);
        *i += gp_count_fmt_specs(fmt);

        length += pf_vfprintf (out,     fmt, args->list);

        // Dummy consumption. TODO this is useless work, write a dedicated dummy
        // consumer.
        pf_vsnprintf_consuming(NULL, 0, fmt, args);
    } else {
        length += gp_print_va_arg(out, args, obj.type);
    }
    return length;
}

size_t gp_file_print_internal(
    FILE* out,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += gp_print_objects(
            out,
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    return length;
}

size_t gp_file_println_internal(
    FILE* out,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += strlen(" ") + gp_print_objects(
            out,
            &args,
            &i,
            objs[i]);

        if (i < arg_count - 1)
            fputs(" ",  out);
    }
    fputs("\n", out);
    va_end(_args);
    va_end(args.list);

    return length;
}




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               format_scanning.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/format_scanning.h>
#include <string.h>

PFFormatSpecifier
pf_scan_format_string(
    const char fmt_string[static 1],
    pf_va_list* va_args)
{
    PFFormatSpecifier fmt = { fmt_string };

    fmt.string = strchr(fmt.string, '%');
    if (fmt.string == NULL)
    {
        return fmt;
    }
    if (fmt.string[1] == '%')
    {
        fmt.string_length = 2;
        fmt.conversion_format = '%';
    }

    // Iterator
    const char* c = fmt.string + strlen("%");

    // Find all flags if any
    for (const char* flag; (flag = strchr("-+ #0", *c)); c++)
    {
        switch (*flag)
        {
            case '-': fmt.flag.dash  = 1; break;
            case '+': fmt.flag.plus  = 1; break;
            case ' ': fmt.flag.space = 1; break;
            case '#': fmt.flag.hash  = 1; break;
            case '0': fmt.flag.zero  = 1; break;
        }
    }

    // Find field width
    {
        if (*c == '*')
        {
            fmt.field.asterisk = true;

            int width = 0;
            if (va_args != NULL && (width = va_arg(va_args->list, int)) >= 0)
            {
                fmt.field.asterisk = false; // prevent recalling va_arg()
                fmt.field.width = width;
            }
            else if (width < 0)
            {
                fmt.field.asterisk = false;
            }
            c++;
        }
        else if ('1' <= *c && *c <= '9') // can't be 0. Leading 0 is a flag.
        {
            const char* num = c;
            unsigned digits = 0;
            do {
                digits++;
                c++;
            } while ('0' <= *c && *c <= '9');

            unsigned digit = 1;
            while (digits)
            {
                fmt.field.width += (num[digits - 1] - '0') * digit;
                digit *= 10;
                digits--;
            }
        }
    }

    // Find precision
    if (*c == '.')
    {
        c++; // ignore '.'

        if (*c == '*')
        {
            fmt.precision.option = PF_ASTERISK;

            int width = 0;
            if (va_args != NULL && (width = va_arg(va_args->list, int)) >= 0)
            {
                fmt.precision.option = PF_SOME;
                fmt.precision.width = width;
            }
            else if (width < 0)
            {
                fmt.precision.option = PF_NONE;
            }

            c++;
        }
        else
        {
            fmt.precision.option = PF_SOME;
            const char* num = c;
            unsigned digits = 0;

            while ('0' <= *c && *c <= '9')
            {
                digits++;
                c++;
            }

            unsigned digit = 1;
            while (digits)
            {
                fmt.precision.width += (num[digits - 1] - '0') * digit;
                digit *= 10;
                digits--;
            }
        }
    }

    // Find length modifier
    const char* modifier = strchr("hljztLBWDQ", *c);
    if (modifier != NULL)
    {
        fmt.length_modifier = *modifier;
        c++;
        if (*modifier == 'h' && *c == 'h') {
            fmt.length_modifier += 'h';
            c++;
        }
        if (*modifier == 'l' && *c == 'l') {
            fmt.length_modifier += 'l';
            c++;
        }
    }

    fmt.conversion_format = *c;
    c++; // get to the end of string
    fmt.string_length = c - fmt.string;

    return fmt;
}




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               pcg_basic.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

/*
 * This code is derived from the full C implementation, which is in turn
 * derived from the canonical C++ PCG implementation. The C++ version
 * has many additional features and is preferable if you can use C++ in
 * your project.
 */


// state for global RNGs

static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

void pcg32_srandom(uint64_t seed, uint64_t seq)
{
    pcg32_srandom_r(&pcg32_global, seed, seq);
}

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_random()
{
    return pcg32_random_r(&pcg32_global);
}


// pcg32_boundedrand(bound):
// pcg32_boundedrand_r(rng, bound):
//     Generate a uniformly distributed number, r, where 0 <= r < bound

uint32_t pcg32_boundedrand_r(pcg32_random_t* rng, uint32_t bound)
{
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}


uint32_t pcg32_boundedrand(uint32_t bound)
{
    return pcg32_boundedrand_r(&pcg32_global, bound);
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               array.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>

size_t gp_arr_length(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->length;
}

size_t gp_arr_capacity(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->capacity;
}

void* gp_arr_allocation(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->allocation;
}

const GPAllocator* gp_arr_allocator(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->allocator;
}

GPArray(void) gp_arr_new(
    const GPAllocator* allocator,
    const size_t element_size,
    const size_t element_count)
{
    const size_t size = element_size * element_count;
    GPArrayHeader* me = gp_mem_alloc(allocator, sizeof(*me) + size);
    *me = (GPArrayHeader) { 0, element_count, allocator, me };
    return me + 1;
}

void gp_arr_delete(GPArray(void) arr)
{
    if (arr != NULL && gp_arr_allocator(arr) != NULL)
        gp_mem_dealloc(gp_arr_allocator(arr), gp_arr_allocation(arr));
}

GPArray(void) gp_arr_reserve(
    const size_t element_size,
    GPArray(void) arr,
    size_t        capacity)
{
    if (capacity > gp_arr_capacity(arr))
    {
        capacity = gp_next_power_of_2(capacity);
        if (gp_arr_allocator(arr)->dealloc == gp_arena_dealloc &&
            gp_arr_allocation(arr) != NULL)
        { // gp_mem_realloc() knows how to just extend block in arena
            GPArrayHeader* new_block = gp_mem_realloc(
                gp_arr_allocator(arr),
                gp_arr_allocation(arr),
                sizeof*new_block + gp_arr_capacity(arr) * element_size,
                sizeof*new_block + capacity             * element_size);
            new_block->capacity   = capacity;
            new_block->allocation = new_block;
            return new_block + 1;
        } // else not arena or must copy contens from stack
        GPArrayHeader* new_block = gp_mem_alloc(
            gp_arr_allocator(arr),
            sizeof*new_block + capacity * element_size);

        memcpy(new_block, (GPArrayHeader*)arr - 1,
            sizeof*new_block + gp_arr_length(arr) * element_size);

        new_block->capacity   = capacity;
        new_block->allocation = new_block;

        gp_mem_dealloc(gp_arr_allocator(arr), gp_arr_allocation(arr));
        arr = new_block + 1;
    }
    return arr;
}

GPArray(void) gp_arr_copy(
    const size_t        element_size,
    GPArray(void)       dest,
    const void*restrict src,
    const size_t        src_length)
{
    dest = gp_arr_reserve(element_size, dest, src_length);
    memcpy(dest, src, src_length * element_size);
    ((GPArrayHeader*)dest - 1)->length = src_length;
    return dest;
}

GPArray(void) gp_arr_slice(
    const size_t elem_size,
    GPArray(void)       dest,
    const void*restrict const src,
    const size_t i_start,
    const size_t i_end)
{
    size_t length = i_end - i_start;

    if (src == NULL) {
        memmove(dest, (uint8_t*)dest + i_start * elem_size, length * elem_size);
    } else {
        dest = gp_arr_reserve(elem_size, dest, length);
        memcpy(dest, (uint8_t*)src + i_start * elem_size, length * elem_size);
    }
    ((GPArrayHeader*)dest - 1)->length = length;
    return dest;
}

GPArray(void) gp_arr_push(
    const size_t element_size,
    GPArray(void)       arr,
    const void*restrict element)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(element_size, arr, length + 1);
    memcpy((uint8_t*)arr + length * element_size, element, element_size);
    ((GPArrayHeader*)arr - 1)->length++;
    return arr;
}

void* gp_arr_pop(
    const size_t element_size,
    GPArray(void) arr)
{
    return (uint8_t*)arr + --((GPArrayHeader*)arr - 1)->length * element_size;
}

GPArray(void) gp_arr_append(
    const size_t element_size,
    GPArray(void)       arr,
    const void*restrict src,
    const size_t n)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(element_size, arr, length + n);
    memcpy((uint8_t*)arr + length * element_size, src, n * element_size);
    ((GPArrayHeader*)arr - 1)->length += n;
    return arr;
}

GPArray(void) gp_arr_insert(
    const size_t elem_size,
    GPArray(void) arr,
    const size_t pos,
    const void*restrict src,
    const size_t n)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(elem_size, arr, length + n);

    memmove(
        (uint8_t*)arr + (pos + n) * elem_size,
        (uint8_t*)arr +  pos      * elem_size,
        (length - pos)            * elem_size);
    memcpy(
        (uint8_t*)arr +  pos * elem_size, src, n * elem_size);

    ((GPArrayHeader*)arr - 1)->length += n;
    return arr;
}

GPArray(void) gp_arr_remove(
    const size_t  elem_size,
    GPArray(void) arr,
    const size_t  pos,
    const size_t  count)
{
    size_t* length = &((GPArrayHeader*)arr - 1)->length;
    const size_t tail_length = *length - (pos + count);
    memmove(
        (uint8_t*)arr +  pos          * elem_size,
        (uint8_t*)arr + (pos + count) * elem_size,
        tail_length                   * elem_size);
    *length -= count;
    return arr;
}

GPArray(void) gp_arr_map(
    const size_t elem_size,
    GPArray(void) arr,
    const void*restrict optional_src, // mutates arr if NULL
    const size_t src_length,
    void (*const f)(void* out, const void* in))
{
    if (optional_src == NULL) {
        for (size_t i = 0; i < gp_arr_length(arr); i++)
            f((uint8_t*)arr + i * elem_size, (uint8_t*)arr + i * elem_size);
    } else {
        arr = gp_arr_reserve(elem_size, arr, src_length);
        for (size_t i = 0; i < src_length; i++)
            f((uint8_t*)arr + i * elem_size, (uint8_t*)optional_src + i * elem_size);
        ((GPArrayHeader*)arr - 1)->length = src_length;
    }
    return arr;
}

void* gp_arr_fold(
    const size_t elem_size,
    const GPArray(void) arr,
    void* accumulator,
    void* (*const f)(void* accumulator, const void* element))
{
    for (size_t i = 0; i < gp_arr_length(arr); i++)
        accumulator = f(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

void* gp_arr_foldr(
    const size_t elem_size,
    const GPArray(void) arr,
    void* accumulator,
    void* (*const f)(void* accumulator, const void* element))
{
    for (size_t i = gp_arr_length(arr) - 1; i != (size_t)-1; i--)
        accumulator = f(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

static GPArray(void) gp_arr_filter_aliasing(
    const size_t elem_size,
    GPArray(void)restrict const arr,
    const size_t length,
    bool (*const f)(const void* x))
{
    size_t i = 0;
    ((GPArrayHeader*)arr - 1)->length = 0;

    for (; i < length; i++) // skip copying first matching elements
    {
        if (f((uint8_t*)arr + i * elem_size)) {
            ((GPArrayHeader*)arr - 1)->length++;
        } else {
            i++; // after this i > length(arr) so arr[i] and arr[length(arr)]
                 // will not alias
            break;
        }
    }
    for (; i < length; i++)
    {
        if (f((uint8_t*)arr + i * elem_size))
            memcpy(
                (uint8_t*)arr + ((GPArrayHeader*)arr - 1)->length++ * elem_size,
                (uint8_t*)arr + i * elem_size,
                elem_size);
    }
    return arr;
}

static GPArray(void) gp_arr_filter_non_aliasing(
    const size_t elem_size,
    GPArray(void)restrict arr,
    const void*restrict src,
    const size_t src_length,
    bool (*const f)(const void* x))
{
    arr = gp_arr_reserve(elem_size, arr, src_length);
    ((GPArrayHeader*)arr - 1)->length = 0;

    for (size_t i = 0; i < src_length; i++)
    {
        if (f((uint8_t*)src + i * elem_size))
            memcpy(
                (uint8_t*)arr + ((GPArrayHeader*)arr - 1)->length++ * elem_size,
                (uint8_t*)src + i * elem_size,
                elem_size);
    }
    return arr;
}

GPArray(void) gp_arr_filter(
    const size_t elem_size,
    GPArray(void) arr,
    const void*restrict optional_src,
    const size_t src_length,
    bool (*const f)(const void* x))
{
    if (optional_src == NULL)
        return gp_arr_filter_aliasing(
            elem_size,
            arr,
            gp_arr_length(arr),
            f);
    else
        return gp_arr_filter_non_aliasing(
            elem_size,
            arr,
            optional_src,
            src_length,
            f);
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               common.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <printf/printf.h>
#include <stdint.h>
#include <wchar.h>

extern inline void   gp_arena_dealloc(const GPAllocator*, void*);
extern inline size_t gp_max_digits_in  (const GPType T);
extern inline size_t gp_count_fmt_specs(const char* fmt);

size_t gp_convert_va_arg(
    const size_t limit,
    void*restrict const out,
    pf_va_list*restrict const args,
    const GPType type)
{
    size_t length = 0;
    switch (type)
    {
        case GP_CHAR:
        case GP_SIGNED_CHAR:
        case GP_UNSIGNED_CHAR:
            length++;
            if (limit > 0)
                *(uint8_t*)out = (char)va_arg(args->list, int);
            break;

        case GP_UNSIGNED_SHORT:
        case GP_UNSIGNED:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned));
            break;

        case GP_UNSIGNED_LONG:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned long));
            break;

        case GP_UNSIGNED_LONG_LONG:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned long long));
            break;

        case GP_BOOL:
            if (va_arg(args->list, int)) {
                length += strlen("true");
                memcpy(out, "true", gp_min(4llu, limit));
            } else {
                length += strlen("false");
                memcpy(out, "false", gp_min(5llu, limit));
            } break;

        case GP_SHORT:
        case GP_INT:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, int));
            break;

        case GP_LONG:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, long int));
            break;

        case GP_LONG_LONG:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, long long int));
            break;

        case GP_FLOAT:
        case GP_DOUBLE:
            length += pf_gtoa(
                limit,
                out,
                va_arg(args->list, double));
            break;

        char* p;
        size_t p_len;
        case GP_CHAR_PTR:
            p = va_arg(args->list, char*);
            p_len = strlen(p);
            memcpy(out, p, gp_min(p_len, limit));
            length += p_len;
            break;

        GPString s;
        case GP_STRING:
            s = va_arg(args->list, GPString);
            memcpy(out, s, gp_min(gp_arr_length(s), limit));
            length += gp_arr_length(s);
            break;

        case GP_PTR:
            p = va_arg(args->list, void*);
            if (p != NULL) {
                memcpy(out, "0x", gp_min(2llu, limit));
                length += strlen("0x") + pf_xtoa(
                    limit > 2 ? limit - 2 : 0, (char*)out + strlen("0x"), (uintptr_t)p);
            } else {
                length += strlen("(nil)");
                memcpy(out, "(nil)", gp_min(strlen("(nil)"), limit));
            } break;
    }
    return length;
}

size_t gp_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj)
{
    size_t length = 0;
    if (obj.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args->list, char*);
        *i += gp_count_fmt_specs(fmt);

        length += pf_vsnprintf_consuming(
            out,
            limit,
            fmt,
            args);
    } else {
        length += gp_convert_va_arg(limit, out, args, obj.type);
    }
    return length;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               assert.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <printf/printf.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef _WIN32
#include <libloaderapi.h> // GetModuleFileNameA()
#endif

static GP_MAYBE_THREAD_LOCAL const char* gp_current_test  = NULL;
static GP_MAYBE_THREAD_LOCAL const char* gp_current_suite = NULL;
static GP_MAYBE_THREAD_LOCAL bool gp_test_failed  = false;
static GP_MAYBE_THREAD_LOCAL bool gp_suite_failed = false;
static GP_MAYBE_ATOMIC uint32_t gp_test_count    = 0;
static GP_MAYBE_ATOMIC uint32_t gp_suite_count   = 0;
static GP_MAYBE_ATOMIC uint32_t gp_tests_failed  = 0;
static GP_MAYBE_ATOMIC uint32_t gp_suites_failed = 0;
static GP_MAYBE_ATOMIC uint32_t gp_initialized_testing = false;
#define GP_FAILED_STR GP_RED          "[FAILED]" GP_RESET_TERMINAL
#define GP_PASSED_STR GP_BRIGHT_GREEN "[PASSED]" GP_RESET_TERMINAL
static const char* prog_name = "";

// ----------------------------------------------------------------------------
// Implementations for gp_suite(), gp_test(), and relevant

void gp_end_testing(void)
{
    if (gp_test_count + gp_suite_count == 0)
        return;

    gp_test(NULL);
    gp_suite(NULL);

    printf("Finished testing%s%s\n", *prog_name ? " in " : ".", prog_name);
    printf("A total of %u tests ran in %u suites\n", gp_test_count, gp_suite_count);

    if (gp_tests_failed || gp_suites_failed)
        fprintf(stderr,
            GP_RED "%u tests failed and %u suites failed!" GP_RESET_TERMINAL "\n",
            gp_tests_failed, gp_suites_failed);
    else
        printf(GP_BRIGHT_GREEN "Passed all tests!" GP_RESET_TERMINAL "\n");

    puts("---------------------------------------------------------------");

    if (gp_tests_failed || gp_suites_failed)
        exit(EXIT_FAILURE);

    // Prevent redundant reporting at exit. Also user may want to restart tests.
    gp_test_count    = 0;
    gp_suite_count   = 0;
    gp_tests_failed  = 0;
    gp_suites_failed = 0;
    gp_initialized_testing = false;
}

static void gp_init_testing(void)
{
    if ( ! gp_initialized_testing)
    {
        gp_initialized_testing = true;

        #if (__GNUC__ && __linux__) || BSD
        extern const char* __progname;
        prog_name = __progname;
        #elif _WIN32
        static char prog_name_buf[MAX_PATH] = "";
        size_t length = GetModuleFileNameA(NULL, prog_name_buf, MAX_PATH);

        bool valid_ascii = 0 < length && length < MAX_PATH;
        for (size_t i = 0; i < length && valid_ascii; i++)
            valid_ascii = ~prog_name_buf[i] & 0x80;
        if (valid_ascii) {
            const char* trimmed = strrchr(prog_name_buf, '\\');
            prog_name = trimmed ? trimmed + strlen("\\") : prog_name_buf;
        }
        #endif

        puts("---------------------------------------------------------------");
        printf("Starting tests%s%s\n\n", *prog_name ? " in " : "", prog_name);
        atexit(gp_end_testing);
    }
}

void gp_test(const char* name)
{
    gp_init_testing();

    // End current test
    if (gp_current_test != NULL)
    {
        const char* indent = gp_current_suite == NULL ? "" : "\t";
        if (gp_test_failed) {
            gp_tests_failed++;
            fprintf(stderr,
            "%s" GP_FAILED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        } else {
            printf(
            "%s" GP_PASSED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        }

        gp_current_test = NULL;
    }

    // Start new test
    if (name != NULL)
    {
        // No starting message cluttering output

        gp_current_test = name;
        gp_test_failed = false;
        gp_test_count++;
    }
}

void gp_suite(const char* name)
{
    gp_init_testing();
    gp_test(NULL); // End current test

    // End current suite
    if (gp_current_suite != NULL)
    {
        if (gp_suite_failed) {
            gp_suites_failed++;
            fprintf(stderr, GP_FAILED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        } else {
            printf(GP_PASSED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        }
        gp_current_suite = NULL;
    }

    // Start new suite
    if (name != NULL)
    {
        printf("Starting suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n", name);

        gp_current_suite = name;
        gp_suite_failed = false;
        gp_suite_count++;
    }
}

// ----------------------------------------------------------------------------
// Implementations for gp_assert() and gp_expect()

void gp_fail_internal(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    if (gp_current_test != NULL)
    {
        gp_test_failed = true;
        func = gp_current_test;
    }
    if (gp_current_suite != NULL)
    {
        gp_suite_failed = true;
        if (gp_current_test == NULL)
            func = gp_current_suite;
    }

    const char* condition = objs[0].identifier;
    if (gp_sizeof(objs[0].type) == sizeof(uint64_t))
        (void)va_arg(args.list, uint64_t);
    else
        (void)va_arg(args.list, uint32_t);

    const char* indent = gp_current_test != NULL ? "\t" : "";
    fprintf(stderr,
        "%s%s " GP_WHITE_BG GP_BLACK "line %i" GP_RESET_TERMINAL
        " in " GP_CYAN "%s" GP_RESET_TERMINAL "\n"
        "%sCondition " GP_RED "%s " GP_FAILED_STR "\n",
        indent, file, line, func, indent, condition);

    char* buf = NULL;
    size_t buf_capacity = 0;
    for (size_t i = 1; i < arg_count; i++)
    {
        fputs(indent, stderr);
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            size_t fmt_spec_count = 0;
            char* fmt_spec = NULL;
            const char* l_braces = "([{<";
            const char* r_braces = ")]}>";
            const char* brace = strchr(l_braces, fmt[0]);

            for (const char* c = fmt; (c = strchr(c, '%')) != NULL; c++)
            {
                if (c[1] == '%') {
                    c++;
                } else {
                    fmt_spec_count++;
                    fmt_spec = strpbrk(c, "csdioxXufFeEgGp");
                    if (fmt_spec == NULL) {
                        fprintf(stderr, "Invalid format specifier \"%s\".", fmt);
                        continue;
                    }
                }
            }
            size_t printed = 0;
            if (fmt_spec_count == 0) // user comment
            {
                fprintf(stderr, "%s\n", fmt);
                continue;
            }
            else if (fmt_spec_count == 1)
            {
                fprintf(stderr,
                    GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ",
                    objs[i + 1/*0 is fmt so next one*/].identifier);

                // Color and opening quote if string or char
                if (*fmt_spec == 'c') // character
                    fprintf(stderr, GP_YELLOW);
                else if (*fmt_spec == 's') // string
                    fprintf(stderr, GP_BRIGHT_RED);
                else if (strchr("dibBouxX", *fmt_spec)) // integer
                    fprintf(stderr, GP_BRIGHT_BLUE);
                else if (strchr("fFeEgG", *fmt_spec)) // floating point
                    fprintf(stderr, GP_BRIGHT_MAGENTA);
                else if (*fmt_spec == 'p') // pointer
                    fprintf(stderr, GP_BLUE);
            }
            else
            {
                if (brace != NULL) {
                    fputc(*brace, stderr);
                    printed++;
                    if (fmt[1] == ' ') {
                        fputc(' ', stderr);
                        printed++;
                    }
                }
                for (size_t j = 0; j < fmt_spec_count - 1; j++)
                    printed += fprintf(stderr,"%s, ",objs[i + 1 + j].identifier);
                printed += fprintf(stderr, "%s", objs[i + fmt_spec_count].identifier);

                if (brace != NULL) {
                    if (fmt[1] == ' ') {
                        printed++;
                        fputc(' ', stderr);
                    }
                    size_t brace_i = brace - l_braces;
                    fputc(r_braces[brace_i], stderr);
                    printed++;
                }
                fprintf(stderr, GP_RESET_TERMINAL " = " GP_BRIGHT_CYAN);
                printed += strlen(" = ");
            }

            size_t required_capacity = pf_vsnprintf(NULL, 0, fmt, args.list)+1;
            if (required_capacity >= buf_capacity) {
                buf = realloc(
                    buf, buf_capacity = gp_next_power_of_2(required_capacity));
            }
            if (printed + required_capacity > 120)
                fprintf(stderr, "\n\t");

            pf_vsnprintf_consuming(buf, buf_capacity, fmt, &args);
            fprintf(stderr, "%s", buf);

            fprintf(stderr, GP_RESET_TERMINAL "\n");

            i += fmt_spec_count;
            continue;
        } // end if string literal

        fprintf(stderr,
            GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ", objs[i].identifier);

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                fprintf(stderr,
                    GP_YELLOW "\'%c\'", (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                fprintf(stderr, GP_BRIGHT_BLUE "%u", va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                fprintf(stderr,
                    GP_BRIGHT_BLUE "%lu", va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                fprintf(stderr,
                    GP_BRIGHT_BLUE "%llu", va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                fprintf(stderr, va_arg(args.list, int) ? "true" : "false");
                break;

            case GP_SHORT:
            case GP_INT:
                fprintf(stderr, GP_BRIGHT_BLUE "%i", va_arg(args.list, int));
                break;

            case GP_LONG:
                fprintf(stderr, GP_BRIGHT_BLUE "%li", va_arg(args.list, long));
                break;

            case GP_LONG_LONG:
                fprintf(stderr, GP_BRIGHT_BLUE "%lli", va_arg(args.list, long long));
                break;

            double f;
            case GP_FLOAT:
            case GP_DOUBLE:
                f = va_arg(args.list, double);
                fprintf(stderr, GP_BRIGHT_MAGENTA "%g", f);
                if (f - (int64_t)f == f/* whole number */&&
                    (int64_t)f < 100000) { // not printed using %e style
                    fprintf(stderr, ".0");
                } break;

            const char* char_ptr;
            case GP_CHAR_PTR:
                char_ptr = va_arg(args.list, char*);
                if (char_ptr != NULL)
                    fprintf(stderr, GP_BRIGHT_RED "\"%s\"", char_ptr);
                else
                    fprintf(stderr, GP_BRIGHT_RED "(null)");
                break;

            GPString str;
            case GP_STRING:
                str = va_arg(args.list, GPString);
                if (str != NULL)
                    fprintf(stderr, GP_BRIGHT_RED "\"%.*s\"",
                        (int)gp_str_length(str), (char*)str);
                else
                    fprintf(stderr, GP_BRIGHT_RED "(null)");
                break;

            case GP_PTR:
                fprintf(stderr, GP_BLUE "%p", va_arg(args.list, void*));
                break;
        }
        fprintf(stderr, GP_RESET_TERMINAL "\n");
    } // end for args
    fputs("\n", stderr);

    free(buf);
    va_end(_args);
    va_end(args.list);
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               memory.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GP_TESTS
#endif

extern inline void* gp_mem_alloc       (const GPAllocator*,size_t);
extern inline void* gp_mem_alloc_zeroes(const GPAllocator*,size_t);
extern inline void  gp_mem_dealloc     (const GPAllocator*,void*);

static void* gp_heap_alloc(const GPAllocator* unused, size_t block_size)
{
    (void)unused;
    void* mem = malloc(block_size);
    if (mem == NULL) {
        GP_BREAKPOINT;
        perror("malloc() failed");
        abort();
    }
    return mem;
}

static void gp_heap_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    free(block);
}

static const GPAllocator gp_mallocator = {
    .alloc   = gp_heap_alloc,
    .dealloc = gp_heap_dealloc
};
#ifdef NDEBUG
const GPAllocator*const gp_heap = &gp_mallocator;
#else
const GPAllocator*      gp_heap = &gp_mallocator;
#endif

// ----------------------------------------------------------------------------

// Instances of these live in the beginning of the arenas memory block so the
// first object is in &node + 1;
typedef struct gp_arena_node
{
    void* position;
    struct gp_arena_node* tail;
} GPArenaNode;

static void* gp_arena_alloc(const GPAllocator* allocator, const size_t _size)
{
    GPArena* arena = (GPArena*)allocator;
    const size_t size = gp_round_to_aligned(_size);
    GPArenaNode* head = arena->head;

    void* block = head->position;
    if ((uint8_t*)block + size > (uint8_t*)(head + 1) + arena->capacity)
    { // out of memory, create new arena
        const size_t new_cap = arena->capacity;
        GPArenaNode* new_node = gp_mem_alloc(gp_heap,
            sizeof(GPArenaNode) + gp_max(new_cap, size));
        new_node->tail     = head;

        block = new_node->position = new_node + 1;
        new_node->position = (uint8_t*)(new_node->position) + size;
        arena->head = new_node;
    }
    else
    {
        head->position = (uint8_t*)block + size;
    }
    return block;
}

GPArena gp_arena_new(const size_t capacity)
{
    const size_t cap = gp_round_to_aligned(capacity);
    GPArenaNode* node = gp_mem_alloc(gp_heap, sizeof(GPArenaNode) + cap);
    node->position = node + 1;
    node->tail     = NULL;
    return (GPArena) {
        .allocator = { gp_arena_alloc, gp_arena_dealloc },
        .head      = node,
        .capacity  = cap
    };
}

static bool gp_in_this_node(GPArenaNode* node, const size_t capacity, void* _pos)
{
    uint8_t* pos = _pos;
    uint8_t* block_start = (uint8_t*)(node + 1);
    return block_start <= pos && pos <= block_start + capacity;
}

static void gp_arena_node_delete(GPArena* arena)
{
    GPArenaNode* old_head = arena->head;
    arena->head = arena->head->tail;
    gp_mem_dealloc(gp_heap, old_head);
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_in_this_node(arena->head, arena->capacity, new_pos))
        gp_arena_node_delete(arena);
    arena->head->position = new_pos;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;
    while (arena->head != NULL) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(gp_heap, old_head);
    }
}

// ----------------------------------------------------------------------------

void* gp_mem_realloc(
    const GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size)
{
    GPArena* arena = (GPArena*)allocator;
    if (allocator->dealloc == gp_arena_dealloc &&
        (char*)old_block + gp_round_to_aligned(old_size) == (char*)arena->head->position)
    { // extend block instead of reallocating and copying
        arena->head->position = old_block;
        void* new_block = gp_arena_alloc(allocator, new_size);
        if (new_block != old_block) // arena ran out of space and reallocated
            memcpy(new_block, old_block, old_size);
        return new_block;
    }
    void* new_block = gp_mem_alloc(allocator, new_size);
    if (old_block != NULL)
        memcpy(new_block, old_block, old_size);
    gp_mem_dealloc(allocator, old_block);
    return new_block;
}

// ----------------------------------------------------------------------------
// Scope allocator

#ifndef GP_MIN_DEFAULT_SCOPE_SIZE
#define GP_MIN_DEFAULT_SCOPE_SIZE 1024
#endif

typedef struct gp_defer_object
{
    void (*f)(void* arg);
    void* arg;
} GPDeferObject;

typedef struct gp_defer_stack
{
    GPDeferObject* stack;
    uint32_t length;
    uint32_t capacity;
} GPDeferStack;

typedef struct gp_scope
{
    GPArena arena;
    struct gp_scope* parent;
    GPDeferStack* defer_stack;
} GPScope;

static GPThreadKey  gp_scope_factory_key;
static GPThreadOnce gp_scope_factory_key_once = GP_THREAD_ONCE_INIT;

static void gp_end_scopes(GPScope* scope, GPScope*const last_to_be_ended)
{
    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; i--) {
            scope->defer_stack->stack[i].f(scope->defer_stack->stack[i].arg);
        }
    }
    GPScope* previous = scope->parent;
    gp_arena_delete((GPArena*)scope);
    if (previous != NULL && scope != last_to_be_ended)
        gp_end_scopes(previous, last_to_be_ended);
}

// scope_factory lives in it's own arena so returns &scope_factory if there is
// no scopes.
static GPScope* gp_last_scope_of(GPArena* scope_factory)
{
    return (GPScope*) ((uint8_t*)(scope_factory->head->position) -
       gp_round_to_aligned(sizeof(GPScope)));
}

GPAllocator* gp_last_scope(GPAllocator* fallback)
{
    GPArena* factory = gp_thread_local_get(gp_scope_factory_key);
    GPScope* scope = NULL;
    if (factory == NULL || (scope = gp_last_scope_of(factory)) == (GPScope*)factory)
        return fallback;
    return (GPAllocator*)scope;
}

static void gp_delete_scope_factory(void*_factory)
{
    GPArena* factory = _factory;
    GPScope* remaining = gp_last_scope_of(factory);
    if (remaining != (GPScope*)factory)
        gp_end_scopes(remaining, NULL);

    gp_mem_dealloc(gp_heap, factory->head);
}

// Make Valgrind shut up.
static void gp_delete_main_thread_scope_factory(void)
{
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (scope_factory != NULL)
        gp_delete_scope_factory(scope_factory);
}
static void gp_make_scope_factory_key(void)
{
    atexit(gp_delete_main_thread_scope_factory);
    gp_thread_key_create(&gp_scope_factory_key, gp_delete_scope_factory);
}

#if __STDC_VERSION__ >= 201112L  && \
    !defined(__STDC_NO_ATOMICS__) && \
    ATOMIC_LLONG_LOCK_FREE == 2 // always lock-free
// Keeping track of average scope size allows scope allocator to estimate
// optimal scope arena size when creating scopes.
static _Atomic uint64_t gp_total_scope_sizes = 0;
static _Atomic size_t   gp_total_scope_count = 0;
#define GP_ATOMIC_OP(OP) OP
#else
#define GP_ATOMIC_OP(OP)
#endif

static size_t gp_scope_average_memory_usage(void)
{
    return GP_ATOMIC_OP(gp_total_scope_sizes/gp_total_scope_count) - 0;
}

static void* gp_scope_alloc(const GPAllocator* scope, size_t _size)
{
    const size_t size = gp_round_to_aligned(_size);
    GP_ATOMIC_OP(gp_total_scope_sizes += size);
    return gp_arena_alloc(scope, size);
}

#ifdef __GNUC__
#define GP_UNLIKELY(COND) __builtin_expect(!!(COND), 0)
#else
#define GP_UNLIKELY(COND) (COND)
#endif

GPAllocator* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_scope_factory_key_once, gp_make_scope_factory_key);

    // scope_factory should only allocate gp_round_to_aligned(sizeof(GPScope))
    // sized objects for consistent pointer arithmetic.
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (GP_UNLIKELY(scope_factory == NULL)) // initialize scope factory
    {
        const size_t nested_scopes = 64; // before reallocation
        GPArena scope_factory_data = gp_arena_new(
            (nested_scopes + 1/*self*/) * gp_round_to_aligned(sizeof(GPScope)));

        // Extend lifetime
        GPArena* scope_factory_mem = gp_arena_alloc(
            (GPAllocator*)&scope_factory_data,
            sizeof(GPScope)); // gets rounded in gp_arena_alloc()
        memcpy(scope_factory_mem, &scope_factory_data, sizeof*scope_factory_mem);

        scope_factory = scope_factory_mem;
        gp_thread_local_set(gp_scope_factory_key, scope_factory);
    }
    GP_ATOMIC_OP(gp_total_scope_count++);
    const size_t size = _size == 0 ?
        gp_max(2 * gp_scope_average_memory_usage(), (size_t)GP_MIN_DEFAULT_SCOPE_SIZE)
      : _size;

    GPScope* previous = gp_last_scope_of(scope_factory);
    if (previous == (GPScope*)scope_factory)
        previous = NULL;

    GPScope* scope = gp_arena_alloc((GPAllocator*)scope_factory, sizeof*scope);
    *(GPArena*)scope = gp_arena_new(size);
    scope->arena.allocator.alloc = gp_scope_alloc;
    scope->parent = previous;
    scope->defer_stack = NULL;

    return (GPAllocator*)scope;
}

void gp_end(GPAllocator*_scope)
{
    if (_scope == NULL)
        return;
    GPScope* scope = (GPScope*)_scope;
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    gp_end_scopes(gp_last_scope_of(scope_factory), scope);

    gp_arena_rewind(scope_factory, scope);
}

void gp_defer(GPAllocator*_scope, void (*f)(void*), void* arg)
{
    GPScope* scope = (GPScope*)_scope;
    if (scope->defer_stack == NULL)
    {
        const size_t init_cap = 8;
        scope->defer_stack = gp_arena_alloc((GPAllocator*)scope,
            sizeof*(scope->defer_stack) + init_cap * sizeof(GPDeferObject));

        scope->defer_stack->length   = 0;
        scope->defer_stack->capacity = init_cap;
        scope->defer_stack->stack    = (GPDeferObject*)(scope->defer_stack + 1);
    }
    else if (scope->defer_stack->length == scope->defer_stack->capacity)
    {
        GPDeferObject* old_stack  = scope->defer_stack->stack;
        scope->defer_stack->stack = gp_arena_alloc((GPAllocator*)scope,
            scope->defer_stack->capacity * 2 * sizeof(GPDeferObject));
        memcpy(scope->defer_stack->stack, old_stack,
            scope->defer_stack->length * sizeof(GPDeferObject));
        scope->defer_stack->capacity *= 2;
    }
    scope->defer_stack->stack[scope->defer_stack->length].f   = f;
    scope->defer_stack->stack[scope->defer_stack->length].arg = arg;
    scope->defer_stack->length++;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               d2fixed.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.
//
// -DRYU_ONLY_64_BIT_OPS Avoid using uint128_t or 64-bit intrinsics. Slower,
//     depending on your compiler.
//
// -DRYU_AVOID_UINT128 Avoid using uint128_t. Slower, depending on your compiler.


#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef RYU_DEBUG
#include <inttypes.h>
#include <stdio.h>
#endif


#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

#define POW10_ADDITIONAL_BITS 120

#if defined(HAS_UINT128)
static inline uint128_t umul256(const uint128_t a, const uint64_t bHi, const uint64_t bLo, uint128_t* const productHi) {
  const uint64_t aLo = (uint64_t)a;
  const uint64_t aHi = (uint64_t)(a >> 64);

  const uint128_t b00 = (uint128_t)aLo * bLo;
  const uint128_t b01 = (uint128_t)aLo * bHi;
  const uint128_t b10 = (uint128_t)aHi * bLo;
  const uint128_t b11 = (uint128_t)aHi * bHi;

  const uint64_t b00Lo = (uint64_t)b00;
  const uint64_t b00Hi = (uint64_t)(b00 >> 64);

  const uint128_t mid1 = b10 + b00Hi;
  const uint64_t mid1Lo = (uint64_t)(mid1);
  const uint64_t mid1Hi = (uint64_t)(mid1 >> 64);

  const uint128_t mid2 = b01 + mid1Lo;
  const uint64_t mid2Lo = (uint64_t)(mid2);
  const uint64_t mid2Hi = (uint64_t)(mid2 >> 64);

  const uint128_t pHi = b11 + mid1Hi + mid2Hi;
  const uint128_t pLo = ((uint128_t)mid2Lo << 64) | b00Lo;

  *productHi = pHi;
  return pLo;
}

// Returns the high 128 bits of the 256-bit product of a and b.
static inline uint128_t umul256_hi(const uint128_t a, const uint64_t bHi, const uint64_t bLo) {
  // Reuse the umul256 implementation.
  // Optimizers will likely eliminate the instructions used to compute the
  // low part of the product.
  uint128_t hi;
  umul256(a, bHi, bLo, &hi);
  return hi;
}

// Unfortunately, gcc/clang do not automatically turn a 128-bit integer division
// into a multiplication, so we have to do it manually.
static inline uint32_t uint128_mod1e9(const uint128_t v) {
  // After multiplying, we're going to shift right by 29, then truncate to uint32_t.
  // This means that we need only 29 + 32 = 61 bits, so we can truncate to uint64_t before shifting.
  const uint64_t multiplied = (uint64_t) umul256_hi(v, 0x89705F4136B4A597u, 0x31680A88F8953031u);

  // For uint32_t truncation, see the mod1e9() comment in d2s_intrinsics.h.
  const uint32_t shifted = (uint32_t) (multiplied >> 29);

  return ((uint32_t) v) - 1000000000 * shifted;
}

// Best case: use 128-bit type.
static inline uint32_t mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  const uint128_t b0 = ((uint128_t) m) * mul[0]; // 0
  const uint128_t b1 = ((uint128_t) m) * mul[1]; // 64
  const uint128_t b2 = ((uint128_t) m) * mul[2]; // 128
#ifdef RYU_DEBUG
  if (j < 128 || j > 180) {
    printf("%d\n", j);
  }
#endif
  assert(j >= 128);
  assert(j <= 180);
  // j: [128, 256)
  const uint128_t mid = b1 + (uint64_t) (b0 >> 64); // 64
  const uint128_t s1 = b2 + (uint64_t) (mid >> 64); // 128
  return uint128_mod1e9(s1 >> (j - 128));
}

#else // HAS_UINT128

#if defined(HAS_64_BIT_INTRINSICS)
// Returns the low 64 bits of the high 128 bits of the 256-bit product of a and b.
static inline uint64_t umul256_hi128_lo64(
  const uint64_t aHi, const uint64_t aLo, const uint64_t bHi, const uint64_t bLo) {
  uint64_t b00Hi;
  const uint64_t b00Lo = umul128(aLo, bLo, &b00Hi);
  uint64_t b01Hi;
  const uint64_t b01Lo = umul128(aLo, bHi, &b01Hi);
  uint64_t b10Hi;
  const uint64_t b10Lo = umul128(aHi, bLo, &b10Hi);
  uint64_t b11Hi;
  const uint64_t b11Lo = umul128(aHi, bHi, &b11Hi);
  (void) b00Lo; // unused
  (void) b11Hi; // unused
  const uint64_t temp1Lo = b10Lo + b00Hi;
  const uint64_t temp1Hi = b10Hi + (temp1Lo < b10Lo);
  const uint64_t temp2Lo = b01Lo + temp1Lo;
  const uint64_t temp2Hi = b01Hi + (temp2Lo < b01Lo);
  return b11Lo + temp1Hi + temp2Hi;
}

static inline uint32_t uint128_mod1e9(const uint64_t vHi, const uint64_t vLo) {
  // After multiplying, we're going to shift right by 29, then truncate to uint32_t.
  // This means that we need only 29 + 32 = 61 bits, so we can truncate to uint64_t before shifting.
  const uint64_t multiplied = umul256_hi128_lo64(vHi, vLo, 0x89705F4136B4A597u, 0x31680A88F8953031u);

  // For uint32_t truncation, see the mod1e9() comment in d2s_intrinsics.h.
  const uint32_t shifted = (uint32_t) (multiplied >> 29);

  return ((uint32_t) vLo) - 1000000000 * shifted;
}
#endif // HAS_64_BIT_INTRINSICS

static inline uint32_t mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  uint64_t high0;                                   // 64
  const uint64_t low0 = umul128(m, mul[0], &high0); // 0
  uint64_t high1;                                   // 128
  const uint64_t low1 = umul128(m, mul[1], &high1); // 64
  uint64_t high2;                                   // 192
  const uint64_t low2 = umul128(m, mul[2], &high2); // 128
  const uint64_t s0low = low0;              // 0
  (void) s0low; // unused
  const uint64_t s0high = low1 + high0;     // 64
  const uint32_t c1 = s0high < low1;
  const uint64_t s1low = low2 + high1 + c1; // 128
  const uint32_t c2 = s1low < low2; // high1 + c1 can't overflow, so compare against low2
  const uint64_t s1high = high2 + c2;       // 192
#ifdef RYU_DEBUG
  if (j < 128 || j > 180) {
    printf("%d\n", j);
  }
#endif
  assert(j >= 128);
  assert(j <= 180);
#if defined(HAS_64_BIT_INTRINSICS)
  const uint32_t dist = (uint32_t) (j - 128); // dist: [0, 52]
  const uint64_t shiftedhigh = s1high >> dist;
  const uint64_t shiftedlow = shiftright128(s1low, s1high, dist);
  return uint128_mod1e9(shiftedhigh, shiftedlow);
#else // HAS_64_BIT_INTRINSICS
  if (j < 160) { // j: [128, 160)
    const uint64_t r0 = mod1e9(s1high);
    const uint64_t r1 = mod1e9((r0 << 32) | (s1low >> 32));
    const uint64_t r2 = ((r1 << 32) | (s1low & 0xffffffff));
    return mod1e9(r2 >> (j - 128));
  } else { // j: [160, 192)
    const uint64_t r0 = mod1e9(s1high);
    const uint64_t r1 = ((r0 << 32) | (s1low >> 32));
    return mod1e9(r1 >> (j - 160));
  }
#endif // HAS_64_BIT_INTRINSICS
}
#endif // HAS_UINT128

// Convert `digits` to a sequence of decimal digits. Append the digits to the result.
// The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void append_n_digits(const uint32_t olength, uint32_t digits, char* const result) {
#ifdef RYU_DEBUG
  printf("DIGITS=%u\n", digits);
#endif

  uint32_t i = 0;
  while (digits >= 10000) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
    const uint32_t c = digits - 10000 * (digits / 10000);
#else
    const uint32_t c = digits % 10000;
#endif
    digits /= 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    memcpy(result + olength - i - 2, DIGIT_TABLE + c0, 2);
    memcpy(result + olength - i - 4, DIGIT_TABLE + c1, 2);
    i += 4;
  }
  if (digits >= 100) {
    const uint32_t c = (digits % 100) << 1;
    digits /= 100;
    memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
    i += 2;
  }
  if (digits >= 10) {
    const uint32_t c = digits << 1;
    memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
  } else {
    result[0] = (char) ('0' + digits);
  }
}

// Convert `digits` to a sequence of decimal digits. Print the first digit, followed by a decimal
// dot '.' followed by the remaining digits. The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void append_d_digits(const uint32_t olength, uint32_t digits, char* const result) {
#ifdef RYU_DEBUG
  printf("DIGITS=%u\n", digits);
#endif

  uint32_t i = 0;
  while (digits >= 10000) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
    const uint32_t c = digits - 10000 * (digits / 10000);
#else
    const uint32_t c = digits % 10000;
#endif
    digits /= 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c0, 2);
    memcpy(result + olength + 1 - i - 4, DIGIT_TABLE + c1, 2);
    i += 4;
  }
  if (digits >= 100) {
    const uint32_t c = (digits % 100) << 1;
    digits /= 100;
    memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c, 2);
    i += 2;
  }
  if (digits >= 10) {
    const uint32_t c = digits << 1;
    result[2] = DIGIT_TABLE[c + 1];
    result[1] = '.';
    result[0] = DIGIT_TABLE[c];
  } else {
    result[1] = '.';
    result[0] = (char) ('0' + digits);
  }
}

// Convert `digits` to decimal and write the last `count` decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void append_c_digits(const uint32_t count, uint32_t digits, char* const result) {
#ifdef RYU_DEBUG
  printf("DIGITS=%u\n", digits);
#endif
  // Copy pairs of digits from DIGIT_TABLE.
  uint32_t i = 0;
  for (; i < count - 1; i += 2) {
    const uint32_t c = (digits % 100) << 1;
    digits /= 100;
    memcpy(result + count - i - 2, DIGIT_TABLE + c, 2);
  }
  // Generate the last digit if count is odd.
  if (i < count) {
    const char c = (char) ('0' + (digits % 10));
    result[count - i - 1] = c;
  }
}

// Convert `digits` to decimal and write the last 9 decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void append_nine_digits(uint32_t digits, char* const result) {
#ifdef RYU_DEBUG
  printf("DIGITS=%u\n", digits);
#endif
  if (digits == 0) {
    memset(result, '0', 9);
    return;
  }

  for (uint32_t i = 0; i < 5; i += 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
    const uint32_t c = digits - 10000 * (digits / 10000);
#else
    const uint32_t c = digits % 10000;
#endif
    digits /= 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    memcpy(result + 7 - i, DIGIT_TABLE + c0, 2);
    memcpy(result + 5 - i, DIGIT_TABLE + c1, 2);
  }
  result[0] = (char) ('0' + digits);
}

static inline uint32_t indexForExponent(const uint32_t e) {
  return (e + 15) / 16;
}

static inline uint32_t pow10BitsForIndex(const uint32_t idx) {
  return 16 * idx + POW10_ADDITIONAL_BITS;
}

static inline uint32_t lengthForIndex(const uint32_t idx) {
  // +1 for ceil, +16 for mantissa, +8 to round up when dividing by 9
  return (log10Pow2(16 * (int32_t) idx) + 1 + 16 + 8) / 9;
}

static inline int copy_special_str_printf(char* const result, const bool sign, const uint64_t mantissa) {
#if defined(_MSC_VER)
  // TODO: Check that -nan is expected output on Windows.
  if (sign) {
    result[0] = '-';
  }
  if (mantissa) {
    if (mantissa < (1ull << (DOUBLE_MANTISSA_BITS - 1))) {
      memcpy(result + sign, "nan(snan)", 9);
      return sign + 9;
    }
    memcpy(result + sign, "nan", 3);
    return sign + 3;
  }
#else
  if (mantissa) {
    memcpy(result, "nan", 3);
    return 3;
  }
  if (sign) {
    result[0] = '-';
  }
#endif
  memcpy(result + sign, "Infinity", 8);
  return sign + 8;
}

int d2fixed_buffered_n(double d, uint32_t precision, char* result) {
  const uint64_t bits = double_to_bits(d);
#ifdef RYU_DEBUG
  printf("IN=");
  for (int32_t bit = 63; bit >= 0; --bit) {
    printf("%d", (int) ((bits >> bit) & 1));
  }
  printf("\n");
#endif

  // Decode bits into sign, mantissa, and exponent.
  const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
  const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t ieeeExponent = (uint32_t) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

  // Case distinction; exit early for the easy cases.
  if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u)) {
    return copy_special_str_printf(result, ieeeSign, ieeeMantissa);
  }
  if (ieeeExponent == 0 && ieeeMantissa == 0) {
    int index = 0;
    if (ieeeSign) {
      result[index++] = '-';
    }
    result[index++] = '0';
    if (precision > 0) {
      result[index++] = '.';
      memset(result + index, '0', precision);
      index += precision;
    }
    return index;
  }

  int32_t e2;
  uint64_t m2;
  if (ieeeExponent == 0) {
    e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    m2 = ieeeMantissa;
  } else {
    e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  }

#ifdef RYU_DEBUG
  printf("-> %" PRIu64 " * 2^%d\n", m2, e2);
#endif

  int index = 0;
  bool nonzero = false;
  if (ieeeSign) {
    result[index++] = '-';
  }
  if (e2 >= -52) {
    const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t) e2);
    const uint32_t p10bits = pow10BitsForIndex(idx);
    const int32_t len = (int32_t) lengthForIndex(idx);
#ifdef RYU_DEBUG
    printf("idx=%u\n", idx);
    printf("len=%d\n", len);
#endif
    for (int32_t i = len - 1; i >= 0; --i) {
      const uint32_t j = p10bits - e2;
      // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
      // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
      const uint32_t digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));
      if (nonzero) {
        append_nine_digits(digits, result + index);
        index += 9;
      } else if (digits != 0) {
        const uint32_t olength = decimalLength9(digits);
        append_n_digits(olength, digits, result + index);
        index += olength;
        nonzero = true;
      }
    }
  }
  if (!nonzero) {
    result[index++] = '0';
  }
  if (precision > 0) {
    result[index++] = '.';
  }
#ifdef RYU_DEBUG
  printf("e2=%d\n", e2);
#endif
  if (e2 < 0) {
    const int32_t idx = -e2 / 16;
#ifdef RYU_DEBUG
    printf("idx=%d\n", idx);
#endif
    const uint32_t blocks = precision / 9 + 1;
    // 0 = don't round up; 1 = round up unconditionally; 2 = round up if odd.
    int roundUp = 0;
    uint32_t i = 0;
    if (blocks <= MIN_BLOCK_2[idx]) {
      i = blocks;
      memset(result + index, '0', precision);
      index += precision;
    } else if (i < MIN_BLOCK_2[idx]) {
      i = MIN_BLOCK_2[idx];
      memset(result + index, '0', 9 * i);
      index += 9 * i;
    }
    for (; i < blocks; ++i) {
      const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
      const uint32_t p = POW10_OFFSET_2[idx] + i - MIN_BLOCK_2[idx];
      if (p >= POW10_OFFSET_2[idx + 1]) {
        // If the remaining digits are all 0, then we might as well use memset.
        // No rounding required in this case.
        const uint32_t fill = precision - 9 * i;
        memset(result + index, '0', fill);
        index += fill;
        break;
      }
      // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
      // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
      uint32_t digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);
#ifdef RYU_DEBUG
      printf("digits=%u\n", digits);
#endif
      if (i < blocks - 1) {
        append_nine_digits(digits, result + index);
        index += 9;
      } else {
        const uint32_t maximum = precision - 9 * i;
        uint32_t lastDigit = 0;
        for (uint32_t k = 0; k < 9 - maximum; ++k) {
          lastDigit = digits % 10;
          digits /= 10;
        }
#ifdef RYU_DEBUG
        printf("lastDigit=%u\n", lastDigit);
#endif
        if (lastDigit != 5) {
          roundUp = lastDigit > 5;
        } else {
          // Is m * 10^(additionalDigits + 1) / 2^(-e2) integer?
          const int32_t requiredTwos = -e2 - (int32_t) precision - 1;
          const bool trailingZeros = requiredTwos <= 0
            || (requiredTwos < 60 && multipleOfPowerOf2(m2, (uint32_t) requiredTwos));
          roundUp = trailingZeros ? 2 : 1;
#ifdef RYU_DEBUG
          printf("requiredTwos=%d\n", requiredTwos);
          printf("trailingZeros=%s\n", trailingZeros ? "true" : "false");
#endif
        }
        if (maximum > 0) {
          append_c_digits(maximum, digits, result + index);
          index += maximum;
        }
        break;
      }
    }
#ifdef RYU_DEBUG
    printf("roundUp=%d\n", roundUp);
#endif
    if (roundUp != 0) {
      int roundIndex = index;
      int dotIndex = 0; // '.' can't be located at index 0
      while (true) {
        --roundIndex;
        char c;
        if (roundIndex == -1 || (c = result[roundIndex], c == '-')) {
          result[roundIndex + 1] = '1';
          if (dotIndex > 0) {
            result[dotIndex] = '0';
            result[dotIndex + 1] = '.';
          }
          result[index++] = '0';
          break;
        }
        if (c == '.') {
          dotIndex = roundIndex;
          continue;
        } else if (c == '9') {
          result[roundIndex] = '0';
          roundUp = 1;
          continue;
        } else {
          if (roundUp == 2 && c % 2 == 0) {
            break;
          }
          result[roundIndex] = c + 1;
          break;
        }
      }
    }
  } else {
    memset(result + index, '0', precision);
    index += precision;
  }
  return index;
}

void d2fixed_buffered(double d, uint32_t precision, char* result) {
  const int len = d2fixed_buffered_n(d, precision, result);
  result[len] = '\0';
}

#if 0 // not used so shut up analyzer
char* d2fixed(double d, uint32_t precision) {
  char* const buffer = (char*)malloc(2000);
  const int index = d2fixed_buffered_n(d, precision, buffer);
  buffer[index] = '\0';
  return buffer;
}
#endif


int d2exp_buffered_n(double d, uint32_t precision, char* result) {
  const uint64_t bits = double_to_bits(d);
#ifdef RYU_DEBUG
  printf("IN=");
  for (int32_t bit = 63; bit >= 0; --bit) {
    printf("%d", (int) ((bits >> bit) & 1));
  }
  printf("\n");
#endif

  // Decode bits into sign, mantissa, and exponent.
  const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
  const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t ieeeExponent = (uint32_t) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

  // Case distinction; exit early for the easy cases.
  if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u)) {
    return copy_special_str_printf(result, ieeeSign, ieeeMantissa);
  }
  if (ieeeExponent == 0 && ieeeMantissa == 0) {
    int index = 0;
    if (ieeeSign) {
      result[index++] = '-';
    }
    result[index++] = '0';
    if (precision > 0) {
      result[index++] = '.';
      memset(result + index, '0', precision);
      index += precision;
    }
    memcpy(result + index, "e+00", 4);
    index += 4;
    return index;
  }

  int32_t e2;
  uint64_t m2;
  if (ieeeExponent == 0) {
    e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    m2 = ieeeMantissa;
  } else {
    e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  }

#ifdef RYU_DEBUG
  printf("-> %" PRIu64 " * 2^%d\n", m2, e2);
#endif

  const bool printDecimalPoint = precision > 0;
  ++precision;
  int index = 0;
  if (ieeeSign) {
    result[index++] = '-';
  }
  uint32_t digits = 0;
  uint32_t printedDigits = 0;
  uint32_t availableDigits = 0;
  int32_t exp = 0;
  if (e2 >= -52) {
    const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t) e2);
    const uint32_t p10bits = pow10BitsForIndex(idx);
    const int32_t len = (int32_t) lengthForIndex(idx);
#ifdef RYU_DEBUG
    printf("idx=%u\n", idx);
    printf("len=%d\n", len);
#endif
    for (int32_t i = len - 1; i >= 0; --i) {
      const uint32_t j = p10bits - e2;
      // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
      // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
      digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));
      if (printedDigits != 0) {
        if (printedDigits + 9 > precision) {
          availableDigits = 9;
          break;
        }
        append_nine_digits(digits, result + index);
        index += 9;
        printedDigits += 9;
      } else if (digits != 0) {
        availableDigits = decimalLength9(digits);
        exp = i * 9 + (int32_t) availableDigits - 1;
        if (availableDigits > precision) {
          break;
        }
        if (printDecimalPoint) {
          append_d_digits(availableDigits, digits, result + index);
          index += availableDigits + 1; // +1 for decimal point
        } else {
          result[index++] = (char) ('0' + digits);
        }
        printedDigits = availableDigits;
        availableDigits = 0;
      }
    }
  }

  if (e2 < 0 && availableDigits == 0) {
    const int32_t idx = -e2 / 16;
#ifdef RYU_DEBUG
    printf("idx=%d, e2=%d, min=%d\n", idx, e2, MIN_BLOCK_2[idx]);
#endif
    for (int32_t i = MIN_BLOCK_2[idx]; i < 200; ++i) {
      const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
      const uint32_t p = POW10_OFFSET_2[idx] + (uint32_t) i - MIN_BLOCK_2[idx];
      // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
      // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
      digits = (p >= POW10_OFFSET_2[idx + 1]) ? 0 : mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);
#ifdef RYU_DEBUG
      printf("exact=%" PRIu64 " * (%" PRIu64 " + %" PRIu64 " << 64) >> %d\n", m2, POW10_SPLIT_2[p][0], POW10_SPLIT_2[p][1], j);
      printf("digits=%u\n", digits);
#endif
      if (printedDigits != 0) {
        if (printedDigits + 9 > precision) {
          availableDigits = 9;
          break;
        }
        append_nine_digits(digits, result + index);
        index += 9;
        printedDigits += 9;
      } else if (digits != 0) {
        availableDigits = decimalLength9(digits);
        exp = -(i + 1) * 9 + (int32_t) availableDigits - 1;
        if (availableDigits > precision) {
          break;
        }
        if (printDecimalPoint) {
          append_d_digits(availableDigits, digits, result + index);
          index += availableDigits + 1; // +1 for decimal point
        } else {
          result[index++] = (char) ('0' + digits);
        }
        printedDigits = availableDigits;
        availableDigits = 0;
      }
    }
  }

  const uint32_t maximum = precision - printedDigits;
#ifdef RYU_DEBUG
  printf("availableDigits=%u\n", availableDigits);
  printf("digits=%u\n", digits);
  printf("maximum=%u\n", maximum);
#endif
  if (availableDigits == 0) {
    digits = 0;
  }
  uint32_t lastDigit = 0;
  if (availableDigits > maximum) {
    for (uint32_t k = 0; k < availableDigits - maximum; ++k) {
      lastDigit = digits % 10;
      digits /= 10;
    }
  }
#ifdef RYU_DEBUG
  printf("lastDigit=%u\n", lastDigit);
#endif
  // 0 = don't round up; 1 = round up unconditionally; 2 = round up if odd.
  int roundUp = 0;
  if (lastDigit != 5) {
    roundUp = lastDigit > 5;
  } else {
    // Is m * 2^e2 * 10^(precision + 1 - exp) integer?
    // precision was already increased by 1, so we don't need to write + 1 here.
    const int32_t rexp = (int32_t) precision - exp;
    const int32_t requiredTwos = -e2 - rexp;
    bool trailingZeros = requiredTwos <= 0
      || (requiredTwos < 60 && multipleOfPowerOf2(m2, (uint32_t) requiredTwos));
    if (rexp < 0) {
      const int32_t requiredFives = -rexp;
      trailingZeros = trailingZeros && multipleOfPowerOf5(m2, (uint32_t) requiredFives);
    }
    roundUp = trailingZeros ? 2 : 1;
#ifdef RYU_DEBUG
    printf("requiredTwos=%d\n", requiredTwos);
    printf("trailingZeros=%s\n", trailingZeros ? "true" : "false");
#endif
  }
  if (printedDigits != 0) {
    if (digits == 0) {
      memset(result + index, '0', maximum);
    } else {
      append_c_digits(maximum, digits, result + index);
    }
    index += maximum;
  } else {
    if (printDecimalPoint) {
      append_d_digits(maximum, digits, result + index);
      index += maximum + 1; // +1 for decimal point
    } else {
      result[index++] = (char) ('0' + digits);
    }
  }
#ifdef RYU_DEBUG
  printf("roundUp=%d\n", roundUp);
#endif
  if (roundUp != 0) {
    int roundIndex = index;
    while (true) {
      --roundIndex;
      char c;
      if (roundIndex == -1 || (c = result[roundIndex], c == '-')) {
        result[roundIndex + 1] = '1';
        ++exp;
        break;
      }
      if (c == '.') {
        continue;
      } else if (c == '9') {
        result[roundIndex] = '0';
        roundUp = 1;
        continue;
      } else {
        if (roundUp == 2 && c % 2 == 0) {
          break;
        }
        result[roundIndex] = c + 1;
        break;
      }
    }
  }
  result[index++] = 'e';
  if (exp < 0) {
    result[index++] = '-';
    exp = -exp;
  } else {
    result[index++] = '+';
  }

  if (exp >= 100) {
    const int32_t c = exp % 10;
    memcpy(result + index, DIGIT_TABLE + 2 * (exp / 10), 2);
    result[index + 2] = (char) ('0' + c);
    index += 3;
  } else {
    memcpy(result + index, DIGIT_TABLE + 2 * exp, 2);
    index += 2;
  }

  return index;
}

void d2exp_buffered(double d, uint32_t precision, char* result) {
  const int len = d2exp_buffered_n(d, precision, result);
  result[len] = '\0';
}

#if 0 // not used so shut up analyzer
char* d2exp(double d, uint32_t precision) {
  char* const buffer = (char*)malloc(2000);
  const int index = d2exp_buffered_n(d, precision, buffer);
  buffer[index] = '\0';
  return buffer;
}
#endif




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//
//                               conversions.c
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/conversions.h>
#include <stdint.h>

#include <inttypes.h>
#include <math.h>
#include <limits.h>

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

#define POW10_ADDITIONAL_BITS 120

// Max decimal digits in uintmax_t
#if CHAR_BIT == 8
#define MAX_DIGITS (sizeof(uintmax_t) * 3)
#else
#define MAX_DIGITS ((CHAR_BIT * sizeof(uintmax_t) * 3) / 8)
#endif

static void str_reverse_copy(
    char* restrict out,
    char* restrict buf,
    const size_t length,
    const size_t max)
{
    const size_t maxlen = max < length ? max : length;
    for (size_t i = 0; i < maxlen; i++)
        out[i] = buf[length - 1 - i];

    if (length < max)
        out[length] = '\0';
}

static inline void
append_n_digits(const uint32_t olength, uint32_t digits, char* const result);

size_t pf_utoa(const size_t n, char* out, unsigned long long x)
{
    if (n >= 10 && x < 1000000000) // use optimized
    {
        const uint32_t olength = decimalLength9(x);
        append_n_digits(olength, x, out);
        return olength;
    }

    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 10 + '0';
        x /= 10;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_itoa(size_t n, char* out, const long long ix)
{
    char buf[MAX_DIGITS];

    if (ix < 0)
    {
        if (n > 0)
        {
            out[0] = '-';
            n--;
        }
        out++;
    }

    unsigned long long x = imaxabs(ix);
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 10 + '0';
        x /= 10;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i + (ix < 0);
}

size_t pf_otoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 8 + '0';
        x /= 8;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_xtoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        size_t _x = x % 16;
        buf[i++] = _x < 10 ? _x + '0' : _x - 10 + 'a';
        x /= 16;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_Xtoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        size_t _x = x % 16;
        buf[i++] = _x < 10 ? _x + '0' : _x - 10 + 'A';
        x /= 16;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

// ---------------------------------------------------------------------------

static unsigned
pf_d2fixed_buffered_n(
    char* result,
    size_t n,
    PFFormatSpecifier fmt,
    double d);

static unsigned
pf_d2exp_buffered_n(
    char* result,
    const size_t n,
    PFFormatSpecifier fmt,
    double d);

size_t
pf_ftoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'f'};
    return pf_d2fixed_buffered_n(buf, n, fmt, f);
}

size_t
pf_Ftoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'F'};
    return pf_d2fixed_buffered_n(buf, n, fmt, f);
}

size_t
pf_etoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'e'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_Etoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'E'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_gtoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'g'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_Gtoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'G'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t pf_strfromd(
    char* const buf,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double f)
{
    if (fmt.conversion_format == 'f' || fmt.conversion_format == 'F')
        return pf_d2fixed_buffered_n(buf, n, fmt, f);
    else
        return pf_d2exp_buffered_n(buf, n, fmt, f);
}

// ---------------------------------------------------------------------------
//
// Modified Ryū
//
// https://dl.acm.org/doi/pdf/10.1145/3192366.3192369
// https://dl.acm.org/doi/pdf/10.1145/3360595
// https://github.com/ulfjack/ryu
//
// ---------------------------------------------------------------------------

// Convert `digits` to a sequence of decimal digits. Append the digits to the
// result.
// The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void
append_n_digits(const uint32_t olength, uint32_t digits, char* const result)
{
    uint32_t i = 0;
    while (digits >= 10000)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c0, 2);
        memcpy(result + olength - i - 4, DIGIT_TABLE + c1, 2);
        i += 4;
    }
    if (digits >= 100)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
        i += 2;
    }
    if (digits >= 10)
    {
        const uint32_t c = digits << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
    }
    else
    {
        result[0] = (char) ('0' + digits);
    }
}

static inline uint32_t
mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j)
{
    uint64_t high0;                                   // 64
    const uint64_t low0 = umul128(m, mul[0], &high0); // 0
    uint64_t high1;                                   // 128
    const uint64_t low1 = umul128(m, mul[1], &high1); // 64
    uint64_t high2;                                   // 192
    const uint64_t low2 = umul128(m, mul[2], &high2); // 128
    const uint64_t s0low = low0;              // 0
    (void) s0low; // unused
    const uint64_t s0high = low1 + high0;     // 64
    const uint32_t c1 = s0high < low1;
    const uint64_t s1low = low2 + high1 + c1; // 128
    // high1 + c1 can't overflow, so compare against low2
    const uint32_t c2 = s1low < low2;
    const uint64_t s1high = high2 + c2;       // 192
    assert(j >= 128);
    assert(j <= 180);
    #if defined(HAS_64_BIT_INTRINSICS)
        const uint32_t dist = (uint32_t) (j - 128); // dist: [0, 52]
        const uint64_t shiftedhigh = s1high >> dist;
        const uint64_t shiftedlow = shiftright128(s1low, s1high, dist);
        return uint128_mod1e9(shiftedhigh, shiftedlow);
    #else // HAS_64_BIT_INTRINSICS
        if (j < 160)
        { // j: [128, 160)
            const uint64_t r0 = mod1e9(s1high);
            const uint64_t r1 = mod1e9((r0 << 32) | (s1low >> 32));
            const uint64_t r2 = ((r1 << 32) | (s1low & 0xffffffff));
            return mod1e9(r2 >> (j - 128));
        }
        else
        { // j: [160, 192)
            const uint64_t r0 = mod1e9(s1high);
            const uint64_t r1 = ((r0 << 32) | (s1low >> 32));
            return mod1e9(r1 >> (j - 160));
        }
    #endif // HAS_64_BIT_INTRINSICS
}

// Convert `digits` to a sequence of decimal digits. Print the first digit,
// followed by a decimal dot '.' followed by the remaining digits. The caller
// has to guarantee that:
//     10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void
append_d_digits(const uint32_t olength, uint32_t digits, char* const result)
{
    uint32_t i = 0;
    while (digits >= 10000)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c0, 2);
        memcpy(result + olength + 1 - i - 4, DIGIT_TABLE + c1, 2);
        i += 4;
    }

    if (digits >= 100)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c, 2);
        i += 2;
    }

    if (digits >= 10)
    {
        const uint32_t c = digits << 1;
        result[2] = DIGIT_TABLE[c + 1];
        result[1] = '.';
        result[0] = DIGIT_TABLE[c];
    }
    else
    {
        result[1] = '.';
        result[0] = (char) ('0' + digits);
    }
}

static inline void
pf_append_d_digits(
    struct pf_string out[static 1],
    const uint32_t maximum, // first_available_digits
    const uint32_t digits)
{
    if (pf_capacity_left(*out) >= maximum) // write directly
    {
        append_d_digits(
            maximum, digits, out->data + out->length);
        out->length += maximum + strlen(".");
    }
    else // write only as much as fits
    {
        char buf[10];
        append_d_digits(maximum, digits, buf);
        pf_concat(out, buf, maximum + strlen("."));
    }
}

// Convert `digits` to decimal and write the last `count` decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_c_digits(const uint32_t count, uint32_t digits, char* const result)
{
    // Copy pairs of digits from DIGIT_TABLE.
    uint32_t i = 0;
    for (; i < count - 1; i += 2)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + count - i - 2, DIGIT_TABLE + c, 2);
    }
    // Generate the last digit if count is odd.
    if (i < count)
    {
        const char c = (char) ('0' + (digits % 10));
        result[count - i - 1] = c;
    }
}

static inline void
pf_append_c_digits(
    struct pf_string out[static 1],
    const uint32_t count,
    const uint32_t digits)
{
    if (pf_capacity_left(*out) >= count) // write directly
    {
        append_c_digits(
            count, digits, out->data + out->length);
        out->length += count;
    }
    else // write only as much as fits
    {
        char buf[10];
        append_c_digits(
            count, digits, buf);
        pf_concat(out, buf, count);
    }
}

// Convert `digits` to decimal and write the last 9 decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_nine_digits(uint32_t digits, char* const result)
{
    if (digits == 0)
    {
        memset(result, '0', 9);
        return;
    }

    for (uint32_t i = 0; i < 5; i += 4)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + 7 - i, DIGIT_TABLE + c0, 2);
        memcpy(result + 5 - i, DIGIT_TABLE + c1, 2);
    }
    result[0] = (char) ('0' + digits);
}

static inline void
pf_append_nine_digits(struct pf_string out[static 1], uint32_t digits)
{
    if (pf_capacity_left(*out) >= 9) // write directly
    {
        append_nine_digits(digits, out->data + out->length);
        out->length += 9;
    }
    else // write only as much as fits
    {
        char buf[10];
        append_nine_digits(digits, buf);
        pf_concat(out, buf, 9);
    }
}

static inline void
append_utoa(struct pf_string out[static 1], uint32_t digits)
{
    if (pf_capacity_left(*out) >= 9) // write directly
    {
        out->length += pf_utoa(
            pf_capacity_left(*out), out->data + out->length, digits);
    }
    else // write only as much as fits
    {
        char buf[10];
        unsigned buf_len = pf_utoa(sizeof(buf), buf, digits);
        pf_concat(out, buf, buf_len);
    }
}

static inline uint32_t indexForExponent(const uint32_t e)
{
    return (e + 15) / 16;
}

static inline uint32_t pow10BitsForIndex(const uint32_t idx)
{
    return 16 * idx + POW10_ADDITIONAL_BITS;
}

static inline uint32_t lengthForIndex(const uint32_t idx)
{
    // +1 for ceil, +16 for mantissa, +8 to round up when dividing by 9
    return (log10Pow2(16 * (int32_t) idx) + 1 + 16 + 8) / 9;
}

// ---------------------------------------------------------------------------
//
// START OF MODIFIED RYU

static inline unsigned
pf_copy_special_str_printf(
    struct pf_string out[const static 1],
    const uint64_t mantissa,
    const bool uppercase)
{
    if (mantissa != 0)
    {
        pf_concat(out, uppercase ? "NAN" : "nan", strlen("nan"));
        if (pf_capacity_left(*out))
            out->data[out->length] = '\0';
        return out->length;
    }
    else
    {
        pf_concat(out, uppercase ? "INF" : "inf", strlen("inf"));
        if (pf_capacity_left(*out))
            out->data[out->length] = '\0';
        return out->length;
    }
}

static unsigned
pf_d2fixed_buffered_n(
    char* const result,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double d)
{
    struct pf_string out = { result, .capacity = n };
    const bool fmt_is_g =
        fmt.conversion_format == 'g' || fmt.conversion_format == 'G';
    unsigned precision;
    if (fmt.precision.option == PF_SOME)
        precision = fmt.precision.width;
    else
        precision = 6;

    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign =
        ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent = (uint32_t)
        ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

    if (ieeeSign)
        pf_push_char(&out, '-');
    else if (fmt.flag.plus)
        pf_push_char(&out, '+');
    else if (fmt.flag.space)
        pf_push_char(&out, ' ');

    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u))
    {
        const bool uppercase =
            fmt.conversion_format == 'F' || fmt.conversion_format == 'G';
        return pf_copy_special_str_printf(&out, ieeeMantissa, uppercase);
    }

    if (ieeeExponent == 0 && ieeeMantissa == 0) // d == 0.0
    {
        pf_push_char(&out, '0');

        if (precision > 0 || fmt.flag.hash)
            pf_push_char(&out, '.');
        pf_pad(&out, '0', precision);

        if (pf_capacity_left(out))
            out.data[out.length] = '\0';
        return out.length;
    }

    int32_t e2;
    uint64_t m2;
    if (ieeeExponent == 0)
    {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
    }
    else
    {
        e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    bool is_zero = true; // for now

    uint32_t all_digits[256] = {}; // significant digits without trailing zeroes
    size_t digits_length = 0;
    size_t integer_part_end = 0; // place for decimal point

    if (e2 >= -52) // store integer part
    {
        const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t) e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t len = (int32_t)lengthForIndex(idx);

        for (int32_t i = len - 1; i >= 0; --i)
        {
            const uint32_t j = p10bits - e2;
            const uint32_t digits = mulShift_mod1e9(
                m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));

            if ( ! is_zero)
            { // always subsequent iterations of loop
                all_digits[digits_length++] = digits;
            }
            else if (digits != 0)
            { // always 1st iteration of loop
                all_digits[digits_length++] = digits;
                is_zero = false;
            }
        }
        integer_part_end = digits_length;
    }

    if (is_zero)
    {
        all_digits[0]    = 0;
        digits_length    = 1;
        integer_part_end = 1;
    }
    else if (fmt_is_g)
    {
        const uint32_t significant_digits = decimalLength9(all_digits[0]) +
            9*(integer_part_end - 1);

        if (significant_digits >= precision)
            precision = 0;
        else
            precision -= significant_digits;
    }

    bool round_up = false;
    uint32_t lastDigit = 0; // to be cut off. Determines roundUp.
    uint32_t last_digit_magnitude = 1000*1000*1000;
    uint32_t maximum = 9;
    unsigned fract_leading_zeroes = 0;
    unsigned fract_trailing_zeroes = 0;

    // Might have to update precision with 'g' and recalculate, thus loop
    bool first_try = true;
    while (e2 < 0) // store fractional part
    {
        const int32_t idx = -e2 / 16;
        const uint32_t blocks = precision / 9 + 1;

        uint32_t i = 0;
        if (blocks <= MIN_BLOCK_2[idx])
        {
            i = blocks; // skip the for-loop below
            fract_leading_zeroes = precision;
        }
        else if (i < MIN_BLOCK_2[idx])
        {
            i = MIN_BLOCK_2[idx];
            fract_leading_zeroes = 9 * i;
        }

        uint32_t digits = 0;
        for (; i < blocks; ++i) // store significant fractional digits
        {
            const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
            const uint32_t p = POW10_OFFSET_2[idx] + i - MIN_BLOCK_2[idx];

            if (p >= POW10_OFFSET_2[idx + 1])
            {
                fract_trailing_zeroes = precision - 9 * i;
                break;
            }

            digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);
            all_digits[digits_length++] = digits;
        }

        if (fmt_is_g && is_zero && first_try)
        {
            uint32_t total_leading_zeroes = fract_leading_zeroes;

            size_t i;
            for (i = integer_part_end; i < digits_length - 1; i++)
            {
                if (all_digits[i] == 0)
                    total_leading_zeroes += 9;
                else break;
            }
            total_leading_zeroes += 9 - decimalLength9(all_digits[i]);

            if (total_leading_zeroes > 0)
            {
                precision += total_leading_zeroes;
                digits_length = integer_part_end; // reset all_digits[]
                first_try = false;
                continue; // try again
            }
        }

        if (i == blocks)
        {
            maximum = precision - 9 * (i - 1);

            uint32_t k;
            for (k = 0; k < 9 - maximum; ++k) // trim digits from right
            {
                lastDigit = digits % 10;
                digits /= 10;
            }
            const uint32_t magnitude_table[] = { // avoid work in loop
                1000000000,
                100000000,
                10000000,
                1000000,
                100000,
                10000,
                1000,
                100,
                10,
                1
            };
            last_digit_magnitude = magnitude_table[k];

            if (lastDigit != 5)
            {
                round_up = lastDigit > 5;
            }
            else
            {
                const bool any_left_in_digits = k < 9;
                const uint32_t next_digit = any_left_in_digits ?
                    digits : all_digits[digits_length - 2];

                const int32_t requiredTwos = -e2 - (int32_t) precision - 1;
                const bool trailingZeros = requiredTwos <= 0 || (
                    requiredTwos < 60 &&
                    multipleOfPowerOf2(m2, (uint32_t)requiredTwos)
                );

                round_up = next_digit % 2 || ! trailingZeros;
            }

            if (digits_length != integer_part_end) // update modified digits
                all_digits[digits_length - 1] = digits;
            else // digits never stored, nowhere to round
                round_up = false;
        }

        break;
    }

    if (round_up)
    {
        uint32_t last_real_mag = 0;
        if (fmt_is_g && is_zero)
            last_real_mag = decimalLength9(all_digits[1]);

        all_digits[digits_length - 1] += 1;

        if (all_digits[digits_length - 1] == last_digit_magnitude)
            all_digits[digits_length - 1] = 0; // carry 1
        else
            round_up = false;

        if (round_up)
        {
            for (size_t i = digits_length - 2; i > 0; i--) // keep rounding
            {
                all_digits[i] += 1;
                if (all_digits[i] == (uint32_t)1000*1000*1000) {
                    all_digits[i] = 0; // carry 1
                } else {
                    round_up = false;
                    break;
                }
            }
        }

        if (round_up)
            all_digits[0] += 1;

        if (fmt_is_g && is_zero)
        {
            if (round_up) { // 0.xxx turned to 1.xxx
                maximum--;
            } else if (decimalLength9(all_digits[1]) > last_real_mag) {
                maximum--;
                all_digits[1] /= 10;
            }
        }
    }

    // Start writing digits for integer part

    append_utoa(&out, all_digits[0]);

    for (size_t i = 1; i < integer_part_end; i++)
    {
        pf_append_nine_digits(&out, all_digits[i]);
    }

    // Start writing digits for fractional part

    if ( ! fmt_is_g || fmt.flag.hash)
    {
        if (precision > 0 || fmt.flag.hash)
            pf_push_char(&out, '.');

        if (digits_length != integer_part_end)
        {
            pf_pad(&out, '0', fract_leading_zeroes);

            for (size_t k = integer_part_end; k < digits_length - 1; k++)
                pf_append_nine_digits(&out, all_digits[k]);

            if (maximum > 0) // write the last digits left
                pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);

            pf_pad(&out, '0', fract_trailing_zeroes);
        }
        else
        {
            pf_pad(&out, '0', precision);
        }
    }
    else
    {
        // Trim trailing zeroes
        while (digits_length != integer_part_end)
        {
            if (all_digits[digits_length - 1] == 0)
            {
                digits_length--;
                maximum = 9;
                continue;
            }
            else
            {
                while (all_digits[digits_length - 1] != 0)
                {
                    if (all_digits[digits_length - 1] % 10 == 0) {
                        all_digits[digits_length - 1] /= 10;
                        maximum--;
                    } else
                        goto end_trim_zeroes;
                }
            }
        } end_trim_zeroes:

        if (digits_length > integer_part_end)
        {
            pf_push_char(&out, '.');
            pf_pad(&out, '0', fract_leading_zeroes);

            for (size_t k = integer_part_end; k < digits_length - 1; k++)
                pf_append_nine_digits(&out, all_digits[k]);

            pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);
        }
    }

    if (pf_capacity_left(out))
        out.data[out.length] = '\0';
    return out.length;
}

static unsigned
pf_d2exp_buffered_n(
    char* const result,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double d)
{
    struct pf_string out = { result, .capacity = n };
    const bool fmt_is_g =
        fmt.conversion_format == 'g' || fmt.conversion_format == 'G';

    unsigned precision;
    if ( ! fmt_is_g)
    {
        if (fmt.precision.option == PF_SOME)
            precision = fmt.precision.width;
        else
            precision = 6;
    }
    else // precision = significant digits so subtract 1, integer part
    {
        if (fmt.precision.option == PF_SOME)
            precision = fmt.precision.width - !!fmt.precision.width;
        else
            precision = 6 - 1;
    }

    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign =
        ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent = (uint32_t)
        ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

    if (ieeeSign)
        pf_push_char(&out, '-');
    else if (fmt.flag.plus)
        pf_push_char(&out, '+');
    else if (fmt.flag.space)
        pf_push_char(&out, ' ');

    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u))
    {
        const bool uppercase =
            fmt.conversion_format == 'E' || fmt.conversion_format == 'G';
        return pf_copy_special_str_printf(&out, ieeeMantissa, uppercase);
    }

    if (ieeeExponent == 0 && ieeeMantissa == 0) // d = 0.0
    {
        pf_push_char(&out, '0');
        if (fmt_is_g && ! fmt.flag.hash) {
            if (pf_capacity_left(out))
                out.data[out.length] = '\0';
            return out.length;
        }

        if (precision > 0 || fmt.flag.hash)
        {
            pf_push_char(&out, '.');
            pf_pad(&out, '0', precision);
        }

        if (fmt.conversion_format == 'e')
            pf_concat(&out, "e+00", strlen("e+00"));
        else if (fmt.conversion_format == 'E')
            pf_concat(&out, "E+00", strlen("E+00"));

        if (pf_capacity_left(out))
            out.data[out.length] = '\0';
        return out.length;
    }

    int32_t e2;
    uint64_t m2;
    if (ieeeExponent == 0) {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
    } else {
        e2 = (int32_t)ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    const bool printDecimalPoint = precision > 0;
    ++precision;

    uint32_t digits = 0;
    uint32_t stored_digits = 0;
    uint32_t availableDigits = 0;
    int32_t exp = 0;

    uint32_t all_digits[256] = {}; // significant digits without trailing zeroes
    size_t digits_length = 0;
    uint32_t first_available_digits = 0;

    if (e2 >= -52)
    {
        const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t)e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t len = (int32_t)lengthForIndex(idx);
        for (int32_t i = len - 1; i >= 0; --i)
        {
            const uint32_t j = p10bits - e2;
            // Temporary: j is usually around 128, and by shifting a bit, we
            // push it to 128 or above, which is a slightly faster code path in
            // mulShift_mod1e9. Instead, we can just increase the multipliers.
            digits = mulShift_mod1e9(
                m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t)(j + 8));

            if (stored_digits != 0) // never first iteration
            { // store fractional part excluding last max 9 digits
                if (stored_digits + 9 > precision)
                {
                    availableDigits = 9;
                    break;
                }

                all_digits[digits_length++] = digits;
                stored_digits += 9;
            }
            else if (digits != 0) // only at first iteration
            { // store integer part, a single digit
                first_available_digits = decimalLength9(digits);
                exp = i * 9 + first_available_digits - 1;

                if (first_available_digits > precision)
                {
                    availableDigits = first_available_digits;
                    break;
                }

                all_digits[0] = digits;
                digits_length = 1;

                stored_digits = first_available_digits;
            }
        }
    }

    if (e2 < 0 && availableDigits == 0)
    {
        const int32_t idx = -e2 / 16;

        for (int32_t i = MIN_BLOCK_2[idx]; i < 200; ++i)
        {
            const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
            const uint32_t p = POW10_OFFSET_2[idx] + (uint32_t)i - MIN_BLOCK_2[idx];
            // Temporary: j is usually around 128, and by shifting a bit, we
            // push it to 128 or above, which is a slightly faster code path in
            // mulShift_mod1e9. Instead, we can just increase the multipliers.
            digits = (p >= POW10_OFFSET_2[idx + 1]) ?
                0 : mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);

            if (stored_digits != 0) // never first iteration
            { // store fractional part excluding last max 9 digits
                if (stored_digits + 9 > precision)
                {
                    availableDigits = 9;
                    break;
                }

                all_digits[digits_length++] = digits;
                stored_digits += 9;
            }
            else if (digits != 0) // only at first iteration
            { // store integer part, a single digit
                first_available_digits = decimalLength9(digits);
                exp = -(i + 1) * 9 + first_available_digits - 1;

                if (first_available_digits > precision)
                {
                    availableDigits = first_available_digits;
                    break;
                }

                all_digits[0] = digits;
                digits_length = 1;

                stored_digits = first_available_digits;
            }
        }
    }

    const uint32_t maximum = precision - stored_digits;

    if (availableDigits == 0)
        digits = 0;

    uint32_t lastDigit = 0;
    uint32_t k = 0;
    if (availableDigits > maximum) // find last digit
    {
        for (k = 0; k < availableDigits - maximum; ++k)
        {
            lastDigit = digits % 10;
            digits /= 10;
        }
    }
    const uint32_t magnitude_table[] = { // avoid work in loop
        1000000000,
        100000000,
        10000000,
        1000000,
        100000,
        10000,
        1000,
        100,
        10,
        1
    };
    const uint32_t last_digit_magnitude = magnitude_table[k];

    all_digits[digits_length++] = digits;

    bool round_up = false;
    if (lastDigit != 5)
    {
        round_up = lastDigit > 5;
    }
    else
    {
        const bool any_left_in_digits = k < 9;
        const uint32_t next_digit = any_left_in_digits ?
            digits : all_digits[digits_length - 2];

        const int32_t rexp = (int32_t)precision - exp;
        const int32_t requiredTwos = -e2 - rexp;
        bool trailingZeros = requiredTwos <= 0 ||
            (requiredTwos < 60 && multipleOfPowerOf2(m2, (uint32_t)requiredTwos));

        if (rexp < 0)
        {
            const int32_t requiredFives = -rexp;
            trailingZeros = trailingZeros &&
                multipleOfPowerOf5(m2, (uint32_t)requiredFives);
        }
        round_up = next_digit % 2 || ! trailingZeros;
    }

    if (round_up && digits_length >= 2)
    {
        all_digits[digits_length - 1] += 1;

        if (all_digits[digits_length - 1] == last_digit_magnitude)
            all_digits[digits_length - 1] = 0; // carry 1
        else
            round_up = false;

        if (round_up)
        {
            for (size_t i = digits_length - 2; i > 0; i--) // keep rounding
            {
                all_digits[i] += 1;
                if (all_digits[i] == (uint32_t)1000*1000*1000) {
                    all_digits[i] = 0; // carry 1
                } else {
                    round_up = false;
                    break;
                }
            }
        }

        if (round_up)
        {
            all_digits[0] += 1;
            if (all_digits[0] == magnitude_table[9 - first_available_digits])
            {
                all_digits[0] /= 10;
                ++exp;
            }
        }
    }
    else if (round_up)
    {
        all_digits[0] += 1;
        if (all_digits[0] ==
                last_digit_magnitude / magnitude_table[first_available_digits])
        {
            exp++;
        }
    }

    // Exponent is known now and we can determine the appropriate 'g' conversion
    if (fmt_is_g && ! (exp < -4 || exp >= (int32_t)precision))
        return pf_d2fixed_buffered_n(result, n, fmt, d);

    if ( ! printDecimalPoint)
    {
        if (all_digits[0] == 10) // rounded up from 9
            all_digits[0] = 1;
        pf_push_char(&out, '0' + all_digits[0]);
        if (fmt.flag.hash)
            pf_push_char(&out, '.');
    }
    else if ( ! fmt_is_g || fmt.flag.hash)
    {
        if (stored_digits != 0)
        {
            pf_append_d_digits(&out, first_available_digits, all_digits[0]);

            for (size_t i = 1; i < digits_length - 1; i++)
                pf_append_nine_digits(&out, all_digits[i]);

            if (all_digits[digits_length - 1] == 0)
                pf_pad(&out, '0', maximum);
            else
                pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);
        }
        else
        {
            pf_append_d_digits(&out, maximum, all_digits[0]);
        }
    }
    else // 'g'
    {
        uint32_t last_digits_length = maximum;
        // Trim trailing zeroes
        while (digits_length > 0)
        {
            if (all_digits[digits_length - 1] == 0)
            {
                digits_length--;
                last_digits_length = 9;
                continue;
            }
            else
            {
                while (all_digits[digits_length - 1] != 0)
                {
                    if (all_digits[digits_length - 1] % 10 == 0) {
                        all_digits[digits_length - 1] /= 10;
                        last_digits_length--;
                    } else
                        goto end_trim_zeroes;
                }
            }
        } end_trim_zeroes:

        if (digits_length > 1)
        {
            pf_append_d_digits(&out, first_available_digits, all_digits[0]);

            for (size_t i = 1; i < digits_length - 1; i++)
                pf_append_nine_digits(&out, all_digits[i]);

            if (all_digits[digits_length - 1] != 0)
                pf_append_c_digits(
                    &out, last_digits_length, all_digits[digits_length - 1]);
        }
        else
        {
            if (all_digits[0] >= 10)
                pf_append_d_digits(
                    &out, decimalLength9(all_digits[0]), all_digits[0]);
            else
                pf_push_char(&out, '0' + all_digits[0]);
        }
    }

    const bool uppercase =
        fmt.conversion_format == 'E' || fmt.conversion_format == 'G';
    pf_push_char(&out, uppercase ? 'E' : 'e');
    if (exp < 0) {
        pf_push_char(&out, '-');
        exp = -exp;
    } else {
        pf_push_char(&out, '+');
    }

    char buf[4] = "";
    if (exp >= 100) {
        const int32_t c = exp % 10;
        memcpy(buf, DIGIT_TABLE + 2 * (exp / 10), 2);
        buf[2] = '0' + c;
    } else {
        memcpy(buf, DIGIT_TABLE + 2 * exp, 2);
    }
    pf_concat(&out, buf, strlen(buf));

    if (pf_capacity_left(out))
        out.data[out.length] = '\0';
    return out.length;
}






#endif // GPC_IMPLEMENTATION
