// [XCDE] T253: validate disk cache bytes before allocating or passing them to a driver.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define XCDE_SPIRV_MAX_BYTES (16u * 1024u * 1024u)
static inline int xcde_spirv_size_valid(uint64_t bytes) {
    return bytes >= 20 && bytes <= XCDE_SPIRV_MAX_BYTES && bytes % 4 == 0;
}
static inline int xcde_spirv_valid(const uint32_t *data, size_t bytes, uint32_t max_version) {
    return data && xcde_spirv_size_valid(bytes) && data[0] == 0x07230203 && data[1] <= max_version &&
           data[1] >= 0x10000 && data[3] > 0 && data[4] == 0;
}
static inline int xcde_pipeline_cache_valid(const void *data, size_t bytes, uint32_t vendor, uint32_t device, const void *uuid) {
    uint32_t h[4];
    if (!data || bytes < 32) return 0;
    memcpy(h, data, sizeof h);
    return h[0] >= 32 && h[0] <= bytes && h[1] == 1 && h[2] == vendor && h[3] == device &&
           !memcmp((const unsigned char *)data + 16, uuid, 16);
}
