// [XCDE] T260: actual texture-index ownership code, linked with section GC (no GPU calls).
#include <assert.h>
#include <stdio.h>
#include "../../runtime/nvn/vulkan/texture_index.h"
static XcdeTextureIndex address_index;
static NvnTexture *g_live[4];
static int g_nlive;
static void live_add(NvnTexture *t,TexBackend *b) { g_live[g_nlive++]=t; b->reg=g_nlive; xcde_texture_index_add(&address_index,t,b); }
static void live_remove(TexBackend *b) {
    xcde_texture_index_remove(&address_index,b);
    NvnTexture *last=g_live[--g_nlive]; g_live[b->reg-1]=last;
    if (last->backend) ((TexBackend *)last->backend)->reg=b->reg;
    b->reg=0;
}
static TexBackend *shared_backend(NvnTexture *t) { return xcde_texture_index_find(&address_index,g_live,t); }
bool renderer_format_is_depth(uint32_t format) { return format==0x35 || format==0x36; }
int main(void) {
    NvnTexture textures[4]={0}; TexBackend backends[4]={0};
    for (int i=0;i<4;i++) {
        textures[i].addr=i<2 ? 0x1000 : 0x1000+(uint64_t)i*0x10000;
        textures[i].width=128; textures[i].height=64; textures[i].depth=1;
        textures[i].levels=1; textures[i].samples=1; textures[i].target=1; textures[i].format=0x25;
        textures[i].backend=&backends[i]; live_add(&textures[i],&backends[i]);
    }
    NvnTexture query=textures[0]; query.backend=NULL;
    assert(shared_backend(&query)==&backends[0]);
    backends[0].nshare=8; assert(shared_backend(&query)==&backends[1]); backends[0].nshare=0;
    query.width=256; assert(!shared_backend(&query)); query.width=128;
    query.format=0x26; assert(!shared_backend(&query)); query.format=0x25;
    live_remove(&backends[0]); assert(shared_backend(&query)==&backends[1]);
    assert(backends[3].reg==1); // last owner moved without invalidating the address chain
    NvnTexture heir=textures[1]; g_live[backends[1].reg-1]=&heir;
    assert(shared_backend(&query)==&backends[1]);
    textures[1].format=heir.format=0x35; query.format=0x36;
    assert(shared_backend(&query)==&backends[1]);
    live_remove(&backends[1]); live_remove(&backends[2]); live_remove(&backends[3]);
    assert(!g_nlive); for (int i=0;i<1024;i++) assert(!address_index.buckets[i]);
    puts("texture address index: sharing precedence, formats, owner transfer and swap removal passed"); return 0;
}
