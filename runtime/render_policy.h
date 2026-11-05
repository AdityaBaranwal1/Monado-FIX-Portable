// [XCDE] T260: pure timing/cache policies, testable without the game or a Vulkan driver.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
static inline bool xcde_repeat_has_budget(uint64_t start, uint64_t now, int fps, double hz) {
    if (!start || now < start || fps <= 0 || hz <= 0) return false;
    double budget = 1e9 / fps, refresh = 1e9 / hz;
    return (double)(now-start) + refresh <= budget + 250000.0;
}
static inline uint64_t xcde_timestamp_delta(uint64_t start, uint64_t end, unsigned bits) {
    if (!bits) return 0;
    return bits >= 64 ? end-start : (end-start) & ((1ull << bits)-1);
}
static inline int xcde_fd_slot(const int *files, const int *users, const uint64_t *ages, int ways, int wanted) {
    int victim=-1;
    for (int i=0;i<ways;i++) {
        if (files[i]==wanted) return i;
        if (!users[i] && (victim<0 || ages[i]<ages[victim])) victim=i;
    }
    return victim;
}
