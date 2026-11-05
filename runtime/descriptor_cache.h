// [XCDE] T260: bounded, submission-local descriptor cache. Keys contain values, never pointers to writes.
#pragma once
#include <stdint.h>
#include <string.h>
#define XCDE_DESC_BINDINGS 96
#define XCDE_DESC_SLOTS 64
typedef struct { uint64_t word[4]; } XcdeDescriptorKey;
typedef struct {
    uint64_t sequence, layout, handle;
    unsigned count;
    XcdeDescriptorKey key[XCDE_DESC_BINDINGS];
} XcdeDescriptorEntry;
static inline unsigned xcde_descriptor_slot(uint64_t layout, const XcdeDescriptorKey *keys, unsigned count) {
    uint64_t h=layout ^ 1469598103934665603ull;
    for (unsigned i=0;i<count;i++) for (unsigned j=0;j<4;j++) h=(h^keys[i].word[j])*1099511628211ull;
    return (unsigned)(h^(h>>32)) & (XCDE_DESC_SLOTS-1);
}
static inline int xcde_descriptor_match(const XcdeDescriptorEntry *e, uint64_t sequence, uint64_t layout,
                                        const XcdeDescriptorKey *keys, unsigned count) {
    return e->handle && e->sequence==sequence && e->layout==layout && e->count==count &&
           !memcmp(e->key, keys, count*sizeof *keys);
}
