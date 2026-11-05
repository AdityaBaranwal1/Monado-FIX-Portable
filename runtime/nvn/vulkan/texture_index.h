// [XCDE] T260: non-owning address index. Caller maintains live[] and defers GPU resource destruction as before.
#pragma once
#include "vk.h"
#include "../renderer.h"
typedef struct { TexBackend *buckets[1024]; } XcdeTextureIndex;
static inline unsigned xcde_texture_bucket(uint64_t addr) { return (unsigned)((addr>>8)^(addr>>23)) & 1023; }
static inline void xcde_texture_index_add(XcdeTextureIndex *index,NvnTexture *t,TexBackend *b) {
    b->address_bucket=xcde_texture_bucket(t->addr);
    b->address_next=index->buckets[b->address_bucket]; index->buckets[b->address_bucket]=b;
}
static inline void xcde_texture_index_remove(XcdeTextureIndex *index,TexBackend *b) {
    TexBackend **link=&index->buckets[b->address_bucket];
    while (*link && *link!=b) link=&(*link)->address_next;
    if (*link) *link=b->address_next;
    b->address_next=NULL;
}
static inline TexBackend *xcde_texture_index_find(const XcdeTextureIndex *index,NvnTexture *const *live,NvnTexture *t) {
    TexBackend *candidate=NULL;
    for (TexBackend *b=index->buckets[xcde_texture_bucket(t->addr)];b;b=b->address_next) {
        NvnTexture *o=live[b->reg-1];
        if (o==t || o->addr!=t->addr || o->width!=t->width || o->height!=t->height || o->depth!=t->depth ||
            o->levels!=t->levels || o->samples!=t->samples || o->target!=t->target) continue;
        bool compatible=o->format==t->format || (renderer_format_is_depth(o->format) && renderer_format_is_depth(t->format));
        if (!compatible || b->nshare>=(int)(sizeof b->share/sizeof b->share[0])) continue;
        if (!candidate || b->reg<candidate->reg) candidate=b;
    }
    return candidate;
}
