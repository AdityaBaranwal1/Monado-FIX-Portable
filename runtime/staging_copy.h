// [XCDE] T254: merge GPU writes without replacing CPU-only changes made since upload.
#pragma once
#include <stddef.h>
#include <stdint.h>
static inline void xcde_merge_gpu_words(uint32_t *guest, const uint32_t *before, const uint32_t *after, size_t words) {
    for (size_t i = 0; i < words; ++i) if (after[i] != before[i]) guest[i] = after[i];
}
