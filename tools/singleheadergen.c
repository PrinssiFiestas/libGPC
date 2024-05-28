// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/string.h>
#include <gpc/io.h>
#include <gpc/array.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !_WIN32
#include <dirent.h>
#else
#define stat _stat
// TODO use this on Windows
// https://github.com/tronkko/dirent/blob/master/include/dirent.h
#endif

