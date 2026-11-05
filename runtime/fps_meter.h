// [XCDE] T258: native frame cadence; caller ticks once per rendered game frame, never for FG/repeated presents.
#pragma once
#include <stdint.h>
typedef struct { uint64_t start, last; unsigned intervals; double fps, ms; int initialized; } XcdeFpsMeter;
static inline void xcde_fps_tick(XcdeFpsMeter *m, uint64_t now) {
    if (!m->initialized || now < m->last || now - m->last > 2000000000ull) {
        m->initialized = 1; m->start = m->last = now; m->intervals = 0; m->fps = m->ms = 0; return;
    }
    if (now == m->last) return;
    m->last = now; ++m->intervals;
    uint64_t elapsed = now - m->start;
    if (elapsed >= 500000000ull) {
        m->fps = (double)m->intervals * 1000000000.0 / (double)elapsed;
        m->ms = (double)elapsed / (double)m->intervals / 1000000.0;
        m->start = now; m->intervals = 0;
    }
}
