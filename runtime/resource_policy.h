// [XCDE] T251: keep CPU and memory available to game/render threads on small PCs.
// [PERF-V5] T251 capped the shader workers at 2 on every integrated GPU / "performance" preset. Draws WAIT for their
// pipelines, so on a 12-thread APU (Ryzen 5 5650U + Vega 7) every new area stalled much longer than in V4 (8 workers).
// Back to the V4 counts (cpus-4, or cpus-2 up to 6 CPUs) with the RAM cap kept. XCDE_SHADER_WORKERS=<n> overrides.
#pragma once
#include <stdlib.h>
static inline int xcde_shader_workers(int cpus, int ram_mb, int integrated, int performance) {
    (void)integrated; (void)performance;
    const char *e = getenv("XCDE_SHADER_WORKERS");
    if (e && atoi(e) > 0) return atoi(e) > 16 ? 16 : atoi(e);
    int n = cpus <= 6 ? cpus - 2 : cpus - 4;
    if (n < 1) n = 1;
    if (n > 16) n = 16;
    int memory_cap = ram_mb > 0 ? ram_mb / 1600 : 2;
    if (memory_cap < 1) memory_cap = 1;
    if (n > memory_cap) n = memory_cap;
    return n;
}
