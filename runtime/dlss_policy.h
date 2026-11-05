// [XCDE] T257: SL queue offsets are only reserved when that queue count is nonzero.
#pragma once
#include <stdint.h>
static inline void xcde_dlss_queue_indices(uint32_t graphics, uint32_t compute, uint32_t *g, uint32_t *c) {
    *g = graphics ? 1u : 0u;
    *c = compute ? 1u + graphics : 0u;
}
