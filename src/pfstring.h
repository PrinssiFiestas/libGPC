// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#ifndef PFSTRING_H_INCLUDED
#define PFSTRING_H_INCLUDED

#include <printf/printf.h>
#include <string.h>
#include <stdbool.h>

typedef struct pf_string
{
    // length is used to store the return value of printf() so it may exceed
    // capacity.

    char* data;
    size_t length;
    const size_t capacity;
} PFString;

static inline size_t pf_min(const size_t a, const size_t b)
{
    return a < b ? a : b;
}

static inline size_t pf_capacity_left(const struct pf_string me)
{
    return me.length >= me.capacity ? 0 : me.capacity - me.length;
}

// Useful for memcpy(), memmove(), memset(), etc.
static inline size_t pf_limit(const struct pf_string me, const size_t x)
{
    const size_t cap_left = pf_capacity_left(me);
    return pf_min(cap_left, x);
}

// Mutating functions return successfully written characters, or in other words,
// how much the resulting string grew.

static inline size_t
pf_concat(struct pf_string me[static 1], const char* src, const size_t length)
{
    memcpy(me->data + me->length, src, pf_limit(*me, length));
    me->length += length;
    return pf_limit(*me, length);
}

static inline size_t
pf_pad(struct pf_string me[static 1], const char c, const size_t length)
{
    memset(me->data + me->length, c, pf_limit(*me, length));
    me->length += length;
    return pf_limit(*me, length);
}

static inline size_t
pf_insert_pad(
    struct pf_string me[static 1],
    const size_t i,
    const char c,
    const size_t n)
{
    const size_t real_length = pf_min(me->length, me->capacity);
    me->length += n;

    if (i >= real_length)
        return 0;

    // Ignore string head by subtracting i
    const size_t cap = me->capacity - i;
    const size_t len = real_length - i;
    const size_t uncut_result_len = len + n;
    const size_t clipped_result_len = pf_min(cap, uncut_result_len);
    const size_t overflowed = uncut_result_len - clipped_result_len;
    const size_t max_move = len - overflowed;
    // End of ignoring i

    if (i + n < me->capacity)
        memmove(me->data + i + n, me->data + i, max_move);
    memset(me->data + i, c, pf_min(n, clipped_result_len));

    return n - overflowed;
}

static inline bool pf_push_char(struct pf_string me[static 1], const char c)
{
    if (pf_limit(*me, 1) != 0)
        me->data[me->length] = c;
    me->length++;
    return pf_limit(*me, 1);
}

#endif // PFSTRING_H_INCLUDED
