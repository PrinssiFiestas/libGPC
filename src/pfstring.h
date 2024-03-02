// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#ifndef PFSTRING_H_INCLUDED
#define PFSTRING_H_INCLUDED

#include <printf/printf.h>
#include <string.h>
#include <stdbool.h>

struct PFString
{
    // length is used to store the return value of printf() so it may exceed
    // capacity.

    char* data;
    size_t length;
    const size_t capacity;
};

static inline size_t min(const size_t a, const size_t b)
{
    return a < b ? a : b;
}

static inline size_t capacity_left(const struct PFString me)
{
    return me.length >= me.capacity ? 0 : me.capacity - me.length;
}

// Useful for memcpy(), memmove(), memset(), etc.
static inline size_t limit(const struct PFString me, const size_t x)
{
    const size_t cap_left = capacity_left(me);
    return min(cap_left, x);
}

// Mutating functions return successfully written characters, or in other words,
// how much the resulting string grew.

static inline size_t
concat(struct PFString me[static 1], const char* src, const size_t length)
{
    memcpy(me->data + me->length, src, limit(*me, length));
    me->length += length;
    return limit(*me, length);
}

static inline size_t
pad(struct PFString me[static 1], const char c, const size_t length)
{
    memset(me->data + me->length, c, limit(*me, length));
    me->length += length;
    return limit(*me, length);
}

static inline size_t
insert_pad(
    struct PFString me[static 1],
    const size_t i,
    const char c,
    const size_t n)
{
    const size_t real_length = min(me->length, me->capacity);
    me->length += n;

    if (i >= real_length)
        return 0;

    // Ignore string head by subtracting i
    const size_t cap = me->capacity - i;
    const size_t len = real_length - i;
    const size_t uncut_result_len = len + n;
    const size_t clipped_result_len = min(cap, uncut_result_len);
    const size_t overflowed = uncut_result_len - clipped_result_len;
    const size_t max_move = len - overflowed;
    // End of ignoring i

    if (i + n < me->capacity)
        memmove(me->data + i + n, me->data + i, max_move);
    memset(me->data + i, c, min(n, clipped_result_len));

    return n - overflowed;
}

static inline bool push_char(struct PFString me[static 1], const char c)
{
    if (limit(*me, 1) != 0)
        me->data[me->length] = c;
    me->length++;
    return limit(*me, 1);
}

#endif // PFSTRING_H_INCLUDED
