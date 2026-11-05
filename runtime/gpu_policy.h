// [XCDE] T248: adapter preference policy, independent of Vulkan and windowing.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    char uuid[33], name[256], reason[256];
    uint32_t index, vendor, type, api, driver, max_storage_range;
    uint32_t vulkan_routes; // T256: supported backend routes (1.2 KHR, 1.3/1.4 core)
    bool usable;
} XcdeGpuInfo;

static inline bool xcde_gpu_uuid_valid(const char *s) {
    if (strlen(s) != 32) return false;
    for (int i = 0; i < 32; ++i) if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
    return true;
}
// wanted_index is a Vulkan enumeration index, not an offset into a filtered list.
static inline int xcde_gpu_choose(const XcdeGpuInfo *list, size_t n, const char *env,
                                  const char *uuid, int legacy_index, bool *fallback) {
    int wanted_index = legacy_index;
    bool by_env = env && *env;
    if (by_env) {
        char *end; long v = strtol(env, &end, 10);
        wanted_index = *end || v < -1 || v > INT_MAX ? -2 : (int)v;
    }
    bool by_uuid = !by_env && uuid && *uuid;
    int best = -1, score = -1;
    *fallback = false;
    for (size_t i = 0; i < n; ++i) {
        if (!list[i].usable) continue;
        if ((by_uuid && !strcmp(uuid, list[i].uuid)) ||
            (!by_uuid && wanted_index >= 0 && list[i].index == (uint32_t)wanted_index)) return (int)i;
        int s = list[i].type == 2 ? 3 : list[i].type == 1 ? 2 : 1; // Vulkan device types
        if (s > score) { best = (int)i; score = s; }
    }
    *fallback = by_uuid || wanted_index != -1;
    return best;
}
