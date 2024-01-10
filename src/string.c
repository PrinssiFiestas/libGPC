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
        memcpy(buf, s->data, s->length);
        s_free_string(s);
        s->data = s->allocation = buf;
        s->capacity = final_cap;
    }
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_reserve(GPString s[GP_NONNULL], const size_t requested_capacity)
{
    int error = s_reserve(s, requested_capacity);
    if (error && s->error_handler != NULL)
        s->error_handler(s, error);
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
    else if (str->error_handler != NULL && str->error_handler(str, error) == GPSTR_CONTINUE)
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
            return GPSTR_ALLOCATION_FAILURE;
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
        return GPSTR_OUT_OF_BOUNDS;

    if (s_reserve(s, s->length) != GPSTR_NO_ERROR)
        return GPSTR_ALLOCATION_FAILURE;

    s->data[i] = c;
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_slice(
    GPString str[GP_NONNULL],
    const size_t start,
    const size_t end)
{
    if (start > str->length || end > str->length || end < start)
        return GPSTR_OUT_OF_BOUNDS;

    str->data  += start;
    str->length = end - start;

    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_substr(
    GPString dest[GP_NONNULL],
    const char src[GP_NONNULL],
    const size_t start,
    const size_t end)
{
    const size_t src_length = strlen(src);
    if (start > src_length || end > src_length || end < start)
    {
        dest->length = 0;
    }
    else
    {
        if (gpstr_reserve(dest, end - start) != GPSTR_NO_ERROR)
            return GPSTR_ALLOCATION_FAILURE;
        memcpy(dest->data, src + start, end - start);
        dest->length = end - start;
    }
    return GPSTR_NO_ERROR;
}

enum GPStringError gpstr_insert(
    GPString dest[GP_NONNULL],
    const size_t pos,
    const char src[GP_NONNULL])
{
    if (pos >= dest->length + 1) // +1 because +0 is allowed for appending
        return GPSTR_OUT_OF_BOUNDS;

    size_t src_length = strlen(src);
    if (s_reserve(dest, dest->length + src_length) != GPSTR_NO_ERROR)
        return GPSTR_ALLOCATION_FAILURE;

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

