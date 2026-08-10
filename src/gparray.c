// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gparray.h>

void* gp_arr_finalize_sized(GPArrayAny arr, size_t element_size)
{
    gp_assume(gp_arr_alignment(arr) <= GP_ALLOC_ALIGNMENT,
              "Sized pointers do not support alignments larger than GP_ALLOC_ALIGNMENT.");

    size_t length = gp_arr_length(arr);
    size_t old_capacity = gp_arr_capacity(arr);
    size_t new_size = length*element_size;

    GPSizedPtrHeader* header = memmove(
        gp_arr_set(arr),
        (char*)arr - sizeof(GPSizedPtrHeader),
        sizeof(GPSizedPtrHeader) + length*element_size);

    // We cannot dereference header yet, the effective type is wrong. Use a
    // dummy variable and memcpy() to update effective type and hope that
    // compiler is smart enough to optimize the redundant copies away.
    if (header->allocator != NULL) {
        GPSizedPtrHeader dummy;
        memcpy(&dummy, header, sizeof dummy);
        memcpy(header, &dummy, sizeof dummy);
    } else // cannot modify effective type of static arrays.
        header = gp_launder(header);

    if (header->allocator != NULL) { // shrink to fit
        size_t ignore_out_size;
        GPSizedPtrHeader* new = header->allocator->alloc(
            header->allocator,
            header,
            sizeof(GPArrayHeader) + old_capacity*element_size,
            sizeof *header + new_size,
            GP_ALLOC_ALIGNMENT,
            true,
            &ignore_out_size);

        if (new != NULL)
            header = new;
        // else reallocation failed, but doesn't matter for shrink to fit, we'll
        // just use the old memory.
    }
    // else array is fixed, shrink to fit would be meaningless.

    header->size = new_size;
    return header + 1;
}
