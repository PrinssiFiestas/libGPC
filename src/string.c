// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <signal.h>

#include "../include/gpc/string.h"
#include "../include/gpc/memory.h"
#include "../include/gpc/utils.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

extern inline char gpstr_at(GPString s, size_t i);
extern inline bool gpstr_is_view(GPString s);
extern inline enum GPStringError gpstr_handle_error(
    GPString me[GP_NONNULL],
    enum GPStringError code,
    const char func[GP_NONNULL],
    const char message[GP_NONNULL]);

// ----------------------------------------------------------------------------

// Default messages for HANDLE_ERROR(). Can be overwritten but these are
// convinient.
enum InternalError
{
    NO_ERROR            = GPSTR_NO_ERROR,
    INDEX_OUT_OF_BOUNDS = GPSTR_INDEX_ERROR,
    ALLOCATION_FAILURE  = GPSTR_ALLOCATION_ERROR,
    END_INDEX_LT_START  = GPSTR_ERROR_LENGTH,
    NO_ALLOCATOR
};
static const char* s_emsg[] = {
    [NO_ERROR]            = "No error.",
    [INDEX_OUT_OF_BOUNDS] = "Index out of bounds.",
    [ALLOCATION_FAILURE]  = "Allocation failed.",
    [END_INDEX_LT_START]  = "End index is smaller than start index.",
    [NO_ALLOCATOR]        = "Exceeding capacity with no allocator.",
};
static enum GPStringError map_error_code(const enum InternalError code)
{
    switch (code) {
        case NO_ERROR:            return GPSTR_NO_ERROR;
        case INDEX_OUT_OF_BOUNDS: return GPSTR_INDEX_ERROR;
        case ALLOCATION_FAILURE:  return GPSTR_ALLOCATION_ERROR;
        case END_INDEX_LT_START:  return GPSTR_INDEX_ERROR;
        case NO_ALLOCATOR:        return GPSTR_ALLOCATION_ERROR;
    }
    return 0;
}
#define HANDLE_ERROR(me, code) \
gpstr_handle_error(me, map_error_code(code), __func__, s_emsg[code])

// Offset from allocation address to the beginning of string data
static size_t s_l_capacity(GPString s)
{
    if (s.allocation != NULL)
        return (size_t)(s.data - s.allocation);
    else return 0;
}

// Capacity from start of the string to the end of the allocated block. Heap
// allocated strings may have offset from the start of allocation so the
// returned value might differ from s->capacity.
static size_t s_r_capacity(GPString s)
{
    return s.capacity - s_l_capacity(s);
}

enum InternalError s_allocate_string_buffer(char** buf, GPString s[GP_NONNULL], size_t cap)
{
    if ( ! s->allocator)
        return NO_ALLOCATOR;

    if ((*buf = gpmem_alloc(s->allocator, cap)) == NULL)
        return ALLOCATION_FAILURE;

    return NO_ERROR;
}

static void s_free_string(GPString s[GP_NONNULL])
{
    if (s->allocation && s->allocator != NULL)
        gpmem_dealloc(s->allocator, s->allocation);
}

static enum InternalError s_reserve(GPString s[GP_NONNULL], const size_t requested_capacity)
{
    if (requested_capacity > s->capacity)
    {
        const size_t final_cap = gp_next_power_of_2(requested_capacity);
        char* buf;
        enum InternalError error = s_allocate_string_buffer(&buf, s, final_cap);
        if (error)
            return error;
        memcpy(buf, s->data, s->length);
        s_free_string(s);
        s->data = s->allocation = buf;
        s->capacity = final_cap;
    }
    return NO_ERROR;
}

// ----------------------------------------------------------------------------

enum GPStringError gpstr_reserve(GPString s[GP_NONNULL], const size_t requested_capacity)
{
    enum InternalError error = s_reserve(s, requested_capacity);
    if (error)
        return HANDLE_ERROR(s, error);
    return GPSTR_NO_ERROR;
}

const char* gpcstr(GPString str[GP_NONNULL])
{
    enum InternalError error = s_reserve(str, str->length + 1);
    if ( ! error || HANDLE_ERROR(str, error) == GPSTR_NO_ERROR)
    {
        str->data[str->length] = '\0';
        return str->data;
    }
    else return "\0Allocation failed in gpcstr().";
}

void gpstr_clear(GPString s[GP_NONNULL])
{
    s_free_string(s);
    *s = (GPString){0};
}

enum GPStringError gpstr_copy(GPString dest[GP_NONNULL], const char src[GP_NONNULL])
{
    size_t src_length = strlen(src);
    if (src_length > dest->capacity) // allocation needed
    {
        const size_t requested_capacity = gp_next_power_of_2(src_length);
        char* buf;
        enum InternalError error = s_allocate_string_buffer(&buf, dest, requested_capacity);
        if (error)
            return HANDLE_ERROR(dest, error);

        s_free_string(dest);
        dest->data = dest->allocation = buf;
        dest->capacity = requested_capacity;
    }
    else if (src_length > s_r_capacity(*dest)) // no alloc needed but need more space
    {
        dest->data = dest->allocation;
    }

    memcpy(dest->data, src, src_length);
    dest->length = src_length;
    return GPSTR_NO_ERROR;
}

bool gpstr_eq(const GPString s1, const char s2[GP_NONNULL])
{
    if (s1.length != strlen(s2))
        return false;
    return memcmp(s1.data, s2, s1.length) == 0;
}

enum GPStringError gpstr_replace_char(GPString s[GP_NONNULL], size_t i, char c)
{
    if (i >= s->length)
        return HANDLE_ERROR(s, INDEX_OUT_OF_BOUNDS);

    enum InternalError error = s_reserve(s, s->length);
    if (error)
        return HANDLE_ERROR(s, error);

    s->data[i] = c;
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_slice(
    GPString str[GP_NONNULL],
    const size_t start,
    const size_t end) // NOT inclusive!
{
    if (start >= str->length || end > str->length)
        return HANDLE_ERROR(str, INDEX_OUT_OF_BOUNDS);
    if (end < start)
        return HANDLE_ERROR(str, END_INDEX_LT_START);

    const size_t length = end - start;
    if (str->allocation != NULL || gpstr_is_view(*str))
        str->data += start;
    else // Move stack allocated data to retain capacity
        memmove(str->data, str->data + start, length);
    str->length = length;

    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_substr(
    GPString dest[GP_NONNULL],
    const char src[GP_NONNULL],
    const size_t start,
    const size_t end)
{
    if (end < start)
        return HANDLE_ERROR(dest, END_INDEX_LT_START);

    const size_t src_length = strlen(src);
    if (start >= src_length || end > src_length)
        return HANDLE_ERROR(dest, INDEX_OUT_OF_BOUNDS);

    const size_t length = end - start;
    enum InternalError error = s_reserve(dest, length);
    if (error)
        return HANDLE_ERROR(dest, error);

    memcpy(dest->data, src + start, length);
    dest->length = end - start;

    memcpy(dest->data, src + start, length);
    dest->length = length;
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_insert(
    GPString dest[GP_NONNULL],
    const size_t pos,
    const char src[GP_NONNULL])
{
    if (pos >= dest->length + 1) // +1 because +0 is allowed for appending
        return HANDLE_ERROR(dest, INDEX_OUT_OF_BOUNDS);

    size_t src_length = strlen(src);

    enum InternalError error = s_reserve(dest, dest->length + src_length);
    if (error)
        return HANDLE_ERROR(dest, error);

    bool can_move_left = src_length <= s_l_capacity(*dest);
    bool pos_is_left   = pos <= dest->length / 2;
    bool no_room_right = dest->length + src_length > s_r_capacity(*dest);

    if ((pos_is_left && can_move_left) || no_room_right)
    { // move data to the left
        memmove(dest->data - src_length, dest->data, pos);
        dest->data -= src_length;
    }
    else
    { // move data to the right
        memmove(dest->data + pos + src_length, dest->data + pos, dest->length - pos);
    }
    memcpy(dest->data + pos, src, src_length);
    dest->length += src_length;
    return GPSTR_NO_ERROR;
}

