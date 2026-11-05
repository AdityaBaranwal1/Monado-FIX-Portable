// [XCDE] T256: API route selection, independent of driver calls for unit testing.
#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { XCDE_VK12 = 1, XCDE_VK13 = 2, XCDE_VK14 = 4 };
static inline uint32_t xcde_vulkan_flag(int route) { return route >= 12 && route <= 14 ? 1u << (route - 12) : 0; }
static inline int xcde_vulkan_route(uint32_t supported, int loader_route, int requested, bool *fallback) {
    *fallback = false;
    if (requested && requested <= loader_route && (supported & xcde_vulkan_flag(requested))) return requested;
    *fallback = requested != 0;
    for (int i = 14; i >= 12; --i) if (i <= loader_route && (supported & xcde_vulkan_flag(i))) return i;
    return 0;
}
