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
extern inline enum GPStringErrorHandling gpstr_handle_error(
    GPString me[GP_NONNULL],
    enum GPStringError code,
    const char func[GP_NONNULL],
    const char message[GP_NONNULL]);

// ----------------------------------------------------------------------------

static const char* s_err_msg[GPSTR_ERROR_LENGTH] = {
    [GPSTR_NO_ERROR]           = "No error.",
    [GPSTR_OUT_OF_BOUNDS]      = "Index out of bounds.",
    [GPSTR_ALLOCATION_FAILURE] = "Allocation failed.",
};
#define HANDLE_ERROR(str, ...) GP_OVERLOAD2(__VA_ARGS__, HE2, HE1)(str, __VA_ARGS__)
#define HE2(str, err, msg) gpstr_handle_error(str, err, __func__, msg)
#define HE1(str, err) gpstr_handle_error(str, err, __func__, s_err_msg[err])

// Offset from allocation address to the beginning of string data
static size_t s_l_capacity(GPString s)
{
    if (s.allocation != NULL)
        return (size_t)(s.data - s.allocation);
    else return 0;
}

static size_t s_r_capacity(GPString s)
{
    return s.capacity - s_l_capacity(s);
}

static char* s_allocate_string_buffer(GPString s[GP_NONNULL], size_t cap)
{
    char* buf = NULL;
    if (s->allocator == NULL || (buf = gpmem_alloc(s->allocator, cap)) == NULL)
        return NULL;

    return buf;
}

static void s_free_string(GPString s[GP_NONNULL])
{
    if (s->allocation && s->allocator != NULL)
        gpmem_dealloc(s->allocator, s->allocation);
}

static enum GPStringError s_reserve(GPString s[GP_NONNULL], const size_t requested_capacity)
{
    if (requested_capacity > s->capacity)
    {
        const size_t final_cap = gp_next_power_of_2(requested_capacity);
        char* buf = s_allocate_string_buffer(s, final_cap);
        if (buf == NULL)
            return GPSTR_ALLOCATION_FAILURE;

        if (s->data != NULL)
            memcpy(buf, s->data, s->length);
        else
            s->length = 0;
        s_free_string(s);
        s->data = s->allocation = buf;
        s->capacity = final_cap;
    }
    return GPSTR_NO_ERROR;
}

// ----------------------------------------------------------------------------

enum GPStringError gpstr_reserve(GPString s[GP_NONNULL], const size_t requested_capacity)
{
    int error = s_reserve(s, requested_capacity);
    if (error)
        HANDLE_ERROR(s, GPSTR_ALLOCATION_FAILURE);
    return error;
}

const char* gpcstr(GPString str[GP_NONNULL])
{
    int error = s_reserve(str, str->length + 1);
    if ( ! error)
    {
        str->data[str->length] = '\0';
        return str->data;
    }
    #ifdef GP_THREAD_LOCAL // C11 -> We have hope
    else if (HANDLE_ERROR(str, GPSTR_ALLOCATION_FAILURE) == GPSTR_CONTINUE)
    {
        // Emergency buffers. Shouldn't be too bad to resort to global state
        // because the return value is supposed to be used as a temporary.
        GP_THREAD_LOCAL static char* heap_buf = NULL;
        GP_THREAD_LOCAL static char small_buf[64];

        if (str->length < sizeof(small_buf)) {
            memcpy(small_buf, str->data, str->length);
            small_buf[str->length] = '\0';
            return small_buf;
        }

        heap_buf = realloc(heap_buf, str->length + 1);
        if (heap_buf != NULL) {
            memcpy(heap_buf, str->data, str->length);
            heap_buf[str->length] = '\0';
            return heap_buf;
        } else {
            HANDLE_ERROR(str, GPSTR_ALLOCATION_FAILURE);
        }
    }
    #endif
    return "\0Allocation failed in gpcstr().";
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
        char* buf = s_allocate_string_buffer(dest, requested_capacity);
        if (buf == NULL)
        {
            if (HANDLE_ERROR(dest, GPSTR_ALLOCATION_FAILURE) != GPSTR_CONTINUE)
                return GPSTR_ALLOCATION_FAILURE;

            if (dest->allocation != NULL) // use full capacity
                dest->data = dest->allocation;
            memcpy(dest->data, src, dest->capacity);

            return GPSTR_NO_ERROR;
        }
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
    if (i >= s->length) {
        HANDLE_ERROR(s, GPSTR_OUT_OF_BOUNDS);
        return GPSTR_OUT_OF_BOUNDS;
    }
    if (s_reserve(s, s->length) != GPSTR_NO_ERROR) {
        HANDLE_ERROR(s, GPSTR_ALLOCATION_FAILURE);
        return GPSTR_ALLOCATION_FAILURE;
    }
    s->data[i] = c;
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_slice(
    GPString str[GP_NONNULL],
    const size_t start,
    const size_t end)
{
    const bool out_of_bounds = start >= str->length || end > str->length;
    if (out_of_bounds && HANDLE_ERROR(str, GPSTR_OUT_OF_BOUNDS) != GPSTR_CONTINUE)
        return GPSTR_OUT_OF_BOUNDS;
    if (end < start &&
        HANDLE_ERROR(str, GPSTR_OUT_OF_BOUNDS, "End index smaller that start.") != GPSTR_CONTINUE)
        return GPSTR_OUT_OF_BOUNDS;

    if (start >= str->length || end <= start) {
        str->length = 0;
        return GPSTR_NO_ERROR;
    }
    const size_t clipped_end = end >= str->length ? str->length : end;
    const size_t length = clipped_end - start;

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
    if (end < start &&
        HANDLE_ERROR(dest, GPSTR_OUT_OF_BOUNDS, "End index smaller than start.") == GPSTR_RETURN) {
        return GPSTR_OUT_OF_BOUNDS;
    } else if (end < start) {
        dest->length = 0;
        return GPSTR_NO_ERROR;
    }
    const size_t src_length = strlen(src);
    if (start >= src_length &&
        HANDLE_ERROR(dest, GPSTR_OUT_OF_BOUNDS) == GPSTR_RETURN) {
        return GPSTR_OUT_OF_BOUNDS;
    } else if (start >= src_length) {
        dest->length = 0;
        return GPSTR_NO_ERROR;
    }

    size_t length = end - start;
    if (gpstr_reserve(dest, length) != GPSTR_NO_ERROR)
    {
        if (HANDLE_ERROR(dest, GPSTR_ALLOCATION_FAILURE) == GPSTR_RETURN)
            return GPSTR_ALLOCATION_FAILURE;

        length = dest->capacity;
    }
    memcpy(dest->data, src + start, length);
    dest->length = length;
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_insert(
    GPString dest[GP_NONNULL],
    const size_t pos,
    const char src[GP_NONNULL])
{
    if (pos >= dest->length + 1) {// +1 because +0 is allowed for appending
        HANDLE_ERROR(dest, GPSTR_OUT_OF_BOUNDS);
        return GPSTR_OUT_OF_BOUNDS;
    }
    size_t src_length = strlen(src);

    if (s_reserve(dest, dest->length + src_length) != GPSTR_NO_ERROR) {
        if (HANDLE_ERROR(dest, GPSTR_ALLOCATION_FAILURE) != GPSTR_CONTINUE)
            return GPSTR_ALLOCATION_FAILURE;

    }
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

