// [XCDE] T255: a guest window image is reusable after its present job AND GPU work complete.
#pragma once
#include <stdint.h>
typedef struct { uint64_t ticket, seq; } XcdeWindowSlot;
static inline void xcde_window_reuse_wait(XcdeWindowSlot *slot, void (*wait_ticket)(uint64_t), void (*wait_gpu)(uint64_t)) {
    if (slot->ticket) wait_ticket(slot->ticket);
    // The render worker publishes seq before completing ticket: read it only after that wait.
    if (slot->seq) wait_gpu(slot->seq);
    slot->ticket = slot->seq = 0;
}
static inline int xcde_window_active(uint32_t requested, int textures) {
    return !requested || requested > (uint32_t)textures ? textures : (int)requested;
}
