// [XCDE] T249: pure timing math; caller supplies monotonic or deterministic virtual nanoseconds.
#pragma once
#include <stdint.h>
#include <limits.h>
#include <math.h>

typedef struct { uint64_t last_ns; double period_ns; int initialized; } XcdeFrameClock;
static inline void xcde_frame_clock_step(XcdeFrameClock *c, uint64_t now) {
    if (!c->initialized) { c->initialized = 1; c->last_ns = now; return; }
    uint64_t dt = now >= c->last_ns ? now - c->last_ns : 0;
    c->last_ns = now;
    // A pause/loading stall is not an update cadence sample. Relearn on resume.
    if (!dt || dt > 250000000ull) { c->period_ns = 0; return; }
    c->period_ns = c->period_ns ? c->period_ns * 0.8 + (double)dt * 0.2 : (double)dt;
}
static inline uint32_t xcde_frame_threshold(const XcdeFrameClock *c, uint32_t n) {
    if (c->period_ns <= 0) return n;
    // Preserve the original strict 'counter > n' boundary, with fractional rates (144/30 = 4.8).
    double frames = ceil(((double)n + 1.0) * (1000000000.0 / 30.0) / c->period_ns - 1e-6);
    if (frames >= (double)UINT32_MAX) return UINT32_MAX - 1;
    return frames > 1 ? (uint32_t)frames - 1 : 0;
}
