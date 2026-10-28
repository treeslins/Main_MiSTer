// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <string.h>

// Join a path without ever selecting a truncated name. The destination may
// be the directory itself; on failure it is left unchanged.
inline bool TryJoinPath(char *destination, size_t capacity, const char *directory, const char *name)
{
    const size_t directory_length = strlen(directory);
    const size_t name_length = strlen(name);
    const size_t separator = directory_length && directory[directory_length - 1] != '/' && name_length ? 1 : 0;
    if (directory_length >= capacity || separator >= capacity - directory_length ||
        name_length >= capacity - directory_length - separator) return false;
    memmove(destination, directory, directory_length);
    if (separator) destination[directory_length] = '/';
    memmove(destination + directory_length + separator, name, name_length + 1);
    return true;
}

template<size_t N>
inline bool TryJoinPath(char (&destination)[N], const char *directory, const char *name)
{
    return TryJoinPath(destination, N, directory, name);
}
