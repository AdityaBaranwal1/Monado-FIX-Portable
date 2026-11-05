// [XCDE] T247: same-directory replacement keeps the previous file on any failure.
#pragma once
#include <stdio.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
static inline int xcde_atomic_replace(const char *tmp, const char *path) {
    return MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}
#else
static inline int xcde_atomic_replace(const char *tmp, const char *path) { return rename(tmp, path) == 0; }
#endif
