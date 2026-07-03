// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/utils.h>

// Having this in a dedicated translation unit should be enough to confuse most
// compilers. The attributes prevent LTO.
GP_GNU_ATTRIB(noinline) GP_OPTIMIZE_NONE
void gp_launder_noinline(void**_)
{
    (void)_;
}
