// Vulkan 1.1 compatibility shims: see vk_compat.h.
#define VKC_NO_REDIRECT
#include "vk_compat.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

bool g_vkc_native = true;
_Thread_local uint32_t g_vkc_samples = 1;
static VkDevice g_dev;
void vkc_init(VkDevice dev, bool native) { g_dev = dev; g_vkc_native = native; }

// ---- 64-bit stage / access masks -> legacy 32-bit masks
VkPipelineStageFlags vkc_stage2_to_1(VkPipelineStageFlags2 s, bool is_src) {
    if (s == VK_PIPELINE_STAGE_2_NONE) return is_src ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkPipelineStageFlags r = (VkPipelineStageFlags)(s & 0xffffffffull);  // the first 32 bits mean the same thing
    if (s & (VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT))
        r |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    if (s & (VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT)) r |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    if (s & VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT)
        r |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
             VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    return r ? r : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // (only stages 1.1 doesn't have: be conservative)
}

VkAccessFlags vkc_access2_to_1(VkAccessFlags2 a) {
    VkAccessFlags r = (VkAccessFlags)(a & 0xffffffffull);
    if (a & (VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT)) r |= VK_ACCESS_SHADER_READ_BIT;
    if (a & VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT) r |= VK_ACCESS_SHADER_WRITE_BIT;
    return r;
}

// ---- vkCmdPipelineBarrier2 -> vkCmdPipelineBarrier (one call: all barriers share the OR of the stage masks)
void vkc_CmdPipelineBarrier2(VkCommandBuffer cb, const VkDependencyInfo *di) {
    if (g_vkc_native) { vkCmdPipelineBarrier2(cb, di); return; }
    uint32_t nm = di->memoryBarrierCount, nb = di->bufferMemoryBarrierCount, ni = di->imageMemoryBarrierCount;
    VkMemoryBarrier mb[nm ? nm : 1];
    VkBufferMemoryBarrier bb[nb ? nb : 1];
    VkImageMemoryBarrier ib[ni ? ni : 1];
    VkPipelineStageFlags src = 0, dst = 0;
    for (uint32_t i = 0; i < nm; i++) {
        const VkMemoryBarrier2 *m = &di->pMemoryBarriers[i];
        mb[i] = (VkMemoryBarrier){.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = vkc_access2_to_1(m->srcAccessMask),
                                  .dstAccessMask = vkc_access2_to_1(m->dstAccessMask)};
        src |= vkc_stage2_to_1(m->srcStageMask, true);
        dst |= vkc_stage2_to_1(m->dstStageMask, false);
    }
    for (uint32_t i = 0; i < nb; i++) {
        const VkBufferMemoryBarrier2 *b = &di->pBufferMemoryBarriers[i];
        bb[i] = (VkBufferMemoryBarrier){.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, .srcAccessMask = vkc_access2_to_1(b->srcAccessMask),
                                        .dstAccessMask = vkc_access2_to_1(b->dstAccessMask), .srcQueueFamilyIndex = b->srcQueueFamilyIndex,
                                        .dstQueueFamilyIndex = b->dstQueueFamilyIndex, .buffer = b->buffer, .offset = b->offset, .size = b->size};
        src |= vkc_stage2_to_1(b->srcStageMask, true);
        dst |= vkc_stage2_to_1(b->dstStageMask, false);
    }
    for (uint32_t i = 0; i < ni; i++) {
        const VkImageMemoryBarrier2 *m = &di->pImageMemoryBarriers[i];
        ib[i] = (VkImageMemoryBarrier){.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = vkc_access2_to_1(m->srcAccessMask),
                                       .dstAccessMask = vkc_access2_to_1(m->dstAccessMask), .oldLayout = m->oldLayout, .newLayout = m->newLayout,
                                       .srcQueueFamilyIndex = m->srcQueueFamilyIndex, .dstQueueFamilyIndex = m->dstQueueFamilyIndex,
                                       .image = m->image, .subresourceRange = m->subresourceRange};
        src |= vkc_stage2_to_1(m->srcStageMask, true);
        dst |= vkc_stage2_to_1(m->dstStageMask, false);
    }
    if (!src) src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    if (!dst) dst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    vkCmdPipelineBarrier(cb, src, dst, di->dependencyFlags, nm, mb, nb, bb, ni, ib);
}

// ---- vkQueueSubmit2 -> vkQueueSubmit + VkTimelineSemaphoreSubmitInfo (the renderer's semaphores: one timeline + binary swapchain ones)
VkResult vkc_QueueSubmit2(VkQueue q, uint32_t n, const VkSubmitInfo2 *si, VkFence fence) {
    if (g_vkc_native) return vkQueueSubmit2(q, n, si, fence);
    for (uint32_t k = 0; k < n; k++) {
        const VkSubmitInfo2 *s = &si[k];
        uint32_t nw = s->waitSemaphoreInfoCount, nc = s->commandBufferInfoCount, ns = s->signalSemaphoreInfoCount;
        VkSemaphore ws[nw ? nw : 1], ss[ns ? ns : 1];
        VkPipelineStageFlags wst[nw ? nw : 1];
        uint64_t wv[nw ? nw : 1], sv[ns ? ns : 1];
        VkCommandBuffer cbs[nc ? nc : 1];
        for (uint32_t i = 0; i < nw; i++) {
            ws[i] = s->pWaitSemaphoreInfos[i].semaphore;
            wv[i] = s->pWaitSemaphoreInfos[i].value;
            wst[i] = vkc_stage2_to_1(s->pWaitSemaphoreInfos[i].stageMask, false);
        }
        for (uint32_t i = 0; i < ns; i++) { ss[i] = s->pSignalSemaphoreInfos[i].semaphore; sv[i] = s->pSignalSemaphoreInfos[i].value; }
        for (uint32_t i = 0; i < nc; i++) cbs[i] = s->pCommandBufferInfos[i].commandBuffer;
        VkTimelineSemaphoreSubmitInfo tl = {.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, .waitSemaphoreValueCount = nw,
                                            .pWaitSemaphoreValues = wv, .signalSemaphoreValueCount = ns, .pSignalSemaphoreValues = sv};
        VkSubmitInfo legacy = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .pNext = &tl, .waitSemaphoreCount = nw, .pWaitSemaphores = ws,
                               .pWaitDstStageMask = wst, .commandBufferCount = nc, .pCommandBuffers = cbs, .signalSemaphoreCount = ns,
                               .pSignalSemaphores = ss};
        VkResult r = vkQueueSubmit(q, 1, &legacy, k + 1 == n ? fence : VK_NULL_HANDLE);
        if (r != VK_SUCCESS) return r;
    }
    return VK_SUCCESS;
}

// ---- vkCmdWriteTimestamp2 -> vkCmdWriteTimestamp (a single legacy stage bit)
void vkc_CmdWriteTimestamp2(VkCommandBuffer cb, VkPipelineStageFlags2 stage, VkQueryPool pool, uint32_t query) {
    if (g_vkc_native) { vkCmdWriteTimestamp2(cb, stage, pool, query); return; }
    VkPipelineStageFlagBits s = stage == VK_PIPELINE_STAGE_2_NONE ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    vkCmdWriteTimestamp(cb, s, pool, query);  // (after everything before it: what ALL_COMMANDS asks for)
}

// ---- vkCmdBindVertexBuffers2 -> vkCmdBindVertexBuffers. Only the call shape the renderer uses: no dynamic strides (the
// pipeline's static stride applies) and sizes are not needed (no robustness2 on this class of GPU).
void vkc_CmdBindVertexBuffers2(VkCommandBuffer cb, uint32_t first, uint32_t count, const VkBuffer *bufs, const VkDeviceSize *offs,
                               const VkDeviceSize *sizes, const VkDeviceSize *strides) {
    if (g_vkc_native) { vkCmdBindVertexBuffers2(cb, first, count, bufs, offs, sizes, strides); return; }
    (void)sizes; (void)strides;
    vkCmdBindVertexBuffers(cb, first, count, bufs, offs);
}

// =====================================================================================================================
// Dynamic rendering -> render passes (Vulkan 1.1). Every image lives in one layout (GENERAL) and the renderer separates
// passes with full barriers, so a render pass is just: formats + load/store ops, initial = final = the attachment layout.
// Render passes are cached by that description; pipelines are given a *compatible* one (same formats and sample count;
// ops and layouts don't matter for compatibility); framebuffers are cached by (render pass, views, size) and die with
// their image views (vkc_DestroyImageView).
// =====================================================================================================================
#define MAXC 8

typedef struct {
    VkFormat color[MAXC];
    VkAttachmentLoadOp cload[MAXC];
    VkAttachmentStoreOp cstore[MAXC];
    VkImageLayout clay[MAXC];
    uint32_t ncolor;
    VkFormat ds;  // depth and/or stencil attachment (one image view carries both aspects)
    VkAttachmentLoadOp dload, sload;
    VkAttachmentStoreOp dstore, sstore;
    VkImageLayout dlay;
    uint32_t samples;
} RpKey;

typedef struct { RpKey key; VkRenderPass rp; } RpEnt;
typedef struct { VkRenderPass rp; uint32_t nv, w, h, layers; VkImageView v[MAXC + 1]; VkFramebuffer fb; } FbEnt;
typedef struct VNode { VkImageView v; VkFormat fmt; struct VNode *next; } VNode;

static pthread_mutex_t vk_mu = PTHREAD_MUTEX_INITIALIZER;
static RpEnt *rps; static size_t n_rp, cap_rp;
static FbEnt *fbs; static size_t n_fb, cap_fb;
#define VBUCKETS 4096
static VNode *vbuck[VBUCKETS];
static size_t vhash(VkImageView v) { uint64_t x = (uint64_t)(uintptr_t)v; x ^= x >> 17; x *= 0x9E3779B97F4A7C15ull; return (size_t)(x >> 40) % VBUCKETS; }

VkResult vkc_CreateImageView(VkDevice dev, const VkImageViewCreateInfo *ci, const VkAllocationCallbacks *ac, VkImageView *out) {
    VkResult r = vkCreateImageView(dev, ci, ac, out);
    if (g_vkc_native || r != VK_SUCCESS) return r;
    pthread_mutex_lock(&vk_mu);
    size_t b = vhash(*out);
    VNode *n = vbuck[b];
    while (n && n->v != *out) n = n->next;
    if (!n) { n = malloc(sizeof *n); n->v = *out; n->next = vbuck[b]; vbuck[b] = n; }
    n->fmt = ci->format;
    pthread_mutex_unlock(&vk_mu);
    return r;
}

static VkFormat view_format(VkImageView v) {  // (vk_mu held)
    for (VNode *n = vbuck[vhash(v)]; n; n = n->next) if (n->v == v) return n->fmt;
    assert(!"image view not created through vkc_CreateImageView");
    return VK_FORMAT_UNDEFINED;
}

void vkc_DestroyImageView(VkDevice dev, VkImageView v, const VkAllocationCallbacks *ac) {
    if (!g_vkc_native && v) {
        pthread_mutex_lock(&vk_mu);
        for (size_t i = 0; i < n_fb;) {
            bool hit = false;
            for (uint32_t k = 0; k < fbs[i].nv; k++) hit |= fbs[i].v[k] == v;
            if (hit) { vkDestroyFramebuffer(dev, fbs[i].fb, NULL); fbs[i] = fbs[--n_fb]; } else i++;
        }
        for (VNode **pp = &vbuck[vhash(v)]; *pp; pp = &(*pp)->next)
            if ((*pp)->v == v) { VNode *d = *pp; *pp = d->next; free(d); break; }
        pthread_mutex_unlock(&vk_mu);
    }
    vkDestroyImageView(dev, v, ac);
}

static VkRenderPass get_rp(const RpKey *k) {  // (vk_mu held)
    for (size_t i = 0; i < n_rp; i++) if (!memcmp(&rps[i].key, k, sizeof *k)) return rps[i].rp;
    VkAttachmentDescription att[MAXC + 1];
    VkAttachmentReference cref[MAXC], dref = {0};
    uint32_t na = 0;
    for (uint32_t i = 0; i < k->ncolor; i++) {
        if (k->color[i] == VK_FORMAT_UNDEFINED) { cref[i] = (VkAttachmentReference){VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED}; continue; }
        att[na] = (VkAttachmentDescription){.format = k->color[i], .samples = (VkSampleCountFlagBits)k->samples, .loadOp = k->cload[i],
                                            .storeOp = k->cstore[i], .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, .initialLayout = k->clay[i], .finalLayout = k->clay[i]};
        cref[i] = (VkAttachmentReference){na++, k->clay[i]};
    }
    if (k->ds != VK_FORMAT_UNDEFINED) {
        att[na] = (VkAttachmentDescription){.format = k->ds, .samples = (VkSampleCountFlagBits)k->samples, .loadOp = k->dload, .storeOp = k->dstore,
                                            .stencilLoadOp = k->sload, .stencilStoreOp = k->sstore, .initialLayout = k->dlay, .finalLayout = k->dlay};
        dref = (VkAttachmentReference){na++, k->dlay};
    }
    VkSubpassDescription sp = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = k->ncolor,
                               .pColorAttachments = k->ncolor ? cref : NULL, .pDepthStencilAttachment = k->ds != VK_FORMAT_UNDEFINED ? &dref : NULL};
    VkRenderPassCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = na, .pAttachments = att,
                                 .subpassCount = 1, .pSubpasses = &sp};
    VkRenderPass rp = VK_NULL_HANDLE;
    if (vkCreateRenderPass(g_dev, &ci, NULL, &rp) != VK_SUCCESS) return VK_NULL_HANDLE;
    if (n_rp == cap_rp) { cap_rp = cap_rp ? cap_rp * 2 : 64; rps = realloc(rps, cap_rp * sizeof *rps); }
    rps[n_rp].key = *k;
    rps[n_rp++].rp = rp;
    return rp;
}

static VkFramebuffer get_fb(VkRenderPass rp, const VkImageView *v, uint32_t nv, uint32_t w, uint32_t h, uint32_t layers) {  // (vk_mu held)
    for (size_t i = 0; i < n_fb; i++) {
        FbEnt *e = &fbs[i];
        if (e->rp == rp && e->nv == nv && e->w == w && e->h == h && e->layers == layers && !memcmp(e->v, v, nv * sizeof *v)) return e->fb;
    }
    VkFramebufferCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = rp, .attachmentCount = nv, .pAttachments = v,
                                  .width = w, .height = h, .layers = layers};
    VkFramebuffer fb = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(g_dev, &ci, NULL, &fb) != VK_SUCCESS) return VK_NULL_HANDLE;
    if (n_fb == cap_fb) { cap_fb = cap_fb ? cap_fb * 2 : 256; fbs = realloc(fbs, cap_fb * sizeof *fbs); }
    FbEnt *e = &fbs[n_fb++];
    memset(e, 0, sizeof *e);
    e->rp = rp; e->nv = nv; e->w = w; e->h = h; e->layers = layers; e->fb = fb;
    memcpy(e->v, v, nv * sizeof *v);
    return fb;
}

void vkc_CmdBeginRendering(VkCommandBuffer cb, const VkRenderingInfo *ri) {
    if (g_vkc_native) { vkCmdBeginRendering(cb, ri); return; }
    assert(ri->colorAttachmentCount <= MAXC && ri->viewMask == 0);
    RpKey k;
    memset(&k, 0, sizeof k);
    k.samples = g_vkc_samples ? g_vkc_samples : 1;
    k.ncolor = ri->colorAttachmentCount;
    VkImageView views[MAXC + 1];
    VkClearValue clears[MAXC + 1];
    uint32_t nv = 0;
    pthread_mutex_lock(&vk_mu);
    for (uint32_t i = 0; i < k.ncolor; i++) {
        const VkRenderingAttachmentInfo *a = &ri->pColorAttachments[i];
        if (!a->imageView) continue;  // (an empty slot: format UNDEFINED -> VK_ATTACHMENT_UNUSED)
        k.color[i] = view_format(a->imageView);
        k.cload[i] = a->loadOp; k.cstore[i] = a->storeOp; k.clay[i] = a->imageLayout;
        views[nv] = a->imageView;
        clears[nv++].color = a->clearValue.color;
    }
    const VkRenderingAttachmentInfo *da = ri->pDepthAttachment, *sa = ri->pStencilAttachment;
    if ((da && da->imageView) || (sa && sa->imageView)) {
        const VkRenderingAttachmentInfo *p = (da && da->imageView) ? da : sa;
        k.ds = view_format(p->imageView);
        k.dlay = p->imageLayout;
        k.dload = da ? da->loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;   k.dstore = da ? da->storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        k.sload = sa ? sa->loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;   k.sstore = sa ? sa->storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        views[nv] = p->imageView;
        clears[nv].depthStencil = (VkClearDepthStencilValue){da ? da->clearValue.depthStencil.depth : 0.0f, sa ? sa->clearValue.depthStencil.stencil : 0};
        nv++;
    }
    VkRenderPass rp = get_rp(&k);
    VkFramebuffer fb = get_fb(rp, views, nv, ri->renderArea.offset.x + ri->renderArea.extent.width,
                              ri->renderArea.offset.y + ri->renderArea.extent.height, ri->layerCount ? ri->layerCount : 1);
    pthread_mutex_unlock(&vk_mu);
    VkRenderPassBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .renderPass = rp, .framebuffer = fb, .renderArea = ri->renderArea,
                                .clearValueCount = nv, .pClearValues = clears};
    vkCmdBeginRenderPass(cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
}

void vkc_CmdEndRendering(VkCommandBuffer cb) {
    if (g_vkc_native) { vkCmdEndRendering(cb); return; }
    vkCmdEndRenderPass(cb);
}

// =====================================================================================================================
// Extended dynamic state -> pipeline variants (Vulkan 1.1). The renderer sets cull mode, front face, topology, depth test /
// write / compare, stencil test / ops, primitive restart and depth-bias enable dynamically. A 1.1 GPU needs them baked into the
// pipeline, so: vkCreateGraphicsPipelines keeps a deep copy of the create info and hands back a *virtual* pipeline (a tagged
// handle); the vkCmdSet* calls only record the state; right before a draw, the real pipeline for (virtual pipeline, state) is
// found or built, and bound. The renderer's own caches/threads keep working with the virtual handles.
// =====================================================================================================================
typedef struct {
    uint8_t cull, ff, topo, have_topo, pre, dt, dw, dop, sten, dbe;
    uint8_t sop[2][4];  // [front, back][fail, pass, depth_fail, compare]
} ExtSt;

static _Thread_local ExtSt g_ext;

typedef struct { ExtSt e; VkPipeline pipe; } Variant;
typedef struct {
    VkDevice dev;
    VkPipelineCache cache;
    VkGraphicsPipelineCreateInfo ci;
    VkPipelineShaderStageCreateInfo *stages;
    char *names[5];
    VkPipelineVertexInputStateCreateInfo vi;
    VkVertexInputBindingDescription *vb;
    VkVertexInputAttributeDescription *va;
    VkPipelineVertexInputDivisorStateCreateInfoEXT dvi;
    VkVertexInputBindingDivisorDescriptionEXT *vd;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineViewportStateCreateInfo vs;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineMultisampleStateCreateInfo ms;
    VkPipelineDepthStencilStateCreateInfo ds;
    bool has_ds;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineColorBlendAttachmentState *cba;
    VkPipelineDynamicStateCreateInfo dy;
    VkDynamicState *dyn;
    Variant *var; uint32_t nvar, capvar;
} VPipe;

#define VTAG 0xC0DE000000000000ull
static VPipe **vps; static size_t nvps;
static uint32_t n_variants;
static VPipe *vp_of(VkPipeline h) {  // (vk_mu held) NULL if it isn't a virtual pipeline
    uint64_t v = (uint64_t)h;
    if ((v & 0xFFFF000000000000ull) != VTAG) return NULL;
    size_t i = (size_t)(v & 0xFFFFFFFFull) - 1;
    return i < nvps ? vps[i] : NULL;
}
static void *dupn(const void *p, size_t n) { void *d; if (!p || !n) return NULL; d = malloc(n); memcpy(d, p, n); return d; }

static bool ext_dyn_state(VkDynamicState s) {
    return (s >= 1000267000 && s <= 1000267012) || s == 1000377001 || s == 1000377002 || s == 1000377004;
}

// Deep copy of what the renderer puts in a graphics create info. Returns NULL (and the caller falls back to a plain real
// pipeline) for anything it doesn't know how to copy.
static VPipe *vp_make(VkDevice dev, VkPipelineCache cache, const VkGraphicsPipelineCreateInfo *ci) {
    if (ci->pNext || ci->pTessellationState || !ci->pVertexInputState || !ci->pInputAssemblyState || !ci->pRasterizationState ||
        !ci->pColorBlendState || !ci->pDynamicState || ci->stageCount > 5)
        return NULL;
    for (uint32_t i = 0; i < ci->stageCount; i++) if (ci->pStages[i].pSpecializationInfo) return NULL;
    if (ci->pRasterizationState->pNext || ci->pColorBlendState->pNext || ci->pInputAssemblyState->pNext) return NULL;
    if (ci->pMultisampleState && (ci->pMultisampleState->pNext || ci->pMultisampleState->pSampleMask)) return NULL;
    VPipe *p = calloc(1, sizeof *p);
    p->dev = dev; p->cache = cache; p->ci = *ci;
    p->stages = dupn(ci->pStages, ci->stageCount * sizeof *ci->pStages);
    for (uint32_t i = 0; i < ci->stageCount; i++) { p->names[i] = strdup(ci->pStages[i].pName); p->stages[i].pName = p->names[i]; }
    p->vi = *ci->pVertexInputState;
    p->vb = dupn(p->vi.pVertexBindingDescriptions, p->vi.vertexBindingDescriptionCount * sizeof *p->vb);
    p->va = dupn(p->vi.pVertexAttributeDescriptions, p->vi.vertexAttributeDescriptionCount * sizeof *p->va);
    p->vi.pVertexBindingDescriptions = p->vb; p->vi.pVertexAttributeDescriptions = p->va;
    if (p->vi.pNext) {
        const VkPipelineVertexInputDivisorStateCreateInfoEXT *d = p->vi.pNext;
        if (d->sType != VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT || d->pNext) { free(p); return NULL; }
        p->dvi = *d;
        p->vd = dupn(d->pVertexBindingDivisors, d->vertexBindingDivisorCount * sizeof *p->vd);
        p->dvi.pVertexBindingDivisors = p->vd;
        p->vi.pNext = &p->dvi;
    }
    p->ia = *ci->pInputAssemblyState;
    if (ci->pViewportState) p->vs = *ci->pViewportState;
    p->rs = *ci->pRasterizationState;
    if (ci->pMultisampleState) p->ms = *ci->pMultisampleState;
    p->has_ds = ci->pDepthStencilState != NULL;
    if (p->has_ds) p->ds = *ci->pDepthStencilState;
    p->cb = *ci->pColorBlendState;
    p->cba = dupn(p->cb.pAttachments, p->cb.attachmentCount * sizeof *p->cba);
    p->cb.pAttachments = p->cba;
    p->dy = *ci->pDynamicState;
    p->dyn = malloc((p->dy.dynamicStateCount + 1) * sizeof *p->dyn);
    uint32_t nd = 0;
    for (uint32_t i = 0; i < p->dy.dynamicStateCount; i++) if (!ext_dyn_state(p->dy.pDynamicStates[i])) p->dyn[nd++] = p->dy.pDynamicStates[i];
    p->dy.dynamicStateCount = nd; p->dy.pDynamicStates = p->dyn;
    p->ci.pStages = p->stages; p->ci.pVertexInputState = &p->vi; p->ci.pInputAssemblyState = &p->ia;
    p->ci.pViewportState = ci->pViewportState ? &p->vs : NULL; p->ci.pRasterizationState = &p->rs;
    p->ci.pMultisampleState = ci->pMultisampleState ? &p->ms : NULL;
    p->ci.pDepthStencilState = p->has_ds ? &p->ds : NULL;
    p->ci.pColorBlendState = &p->cb; p->ci.pDynamicState = &p->dy;
    p->ci.flags &= ~(VkPipelineCreateFlags)(0x100 | 0x200);  // FAIL_ON_PIPELINE_COMPILE_REQUIRED / EARLY_RETURN_ON_FAILURE: a variant is compiled for real
    return p;
}

static ExtSt ext_norm(const ExtSt *src, const VPipe *p) {  // the state a pipeline actually bakes (so equal pipelines are shared)
    ExtSt e = *src;
    if (!e.have_topo) { e.topo = (uint8_t)p->ia.topology; e.have_topo = 1; }
    if (!e.dt) { e.dw = 0; e.dop = 0; }
    if (!e.sten) memset(e.sop, 0, sizeof e.sop);
    return e;
}

static VkPipeline vp_variant(VPipe *p, const ExtSt *raw) {  // (vk_mu held)
    ExtSt e = ext_norm(raw, p);
    for (uint32_t i = 0; i < p->nvar; i++) if (!memcmp(&p->var[i].e, &e, sizeof e)) return p->var[i].pipe;
    VkPipelineInputAssemblyStateCreateInfo ia = p->ia;
    ia.topology = (VkPrimitiveTopology)e.topo; ia.primitiveRestartEnable = e.pre;
    VkPipelineRasterizationStateCreateInfo rs = p->rs;
    rs.cullMode = e.cull; rs.frontFace = (VkFrontFace)e.ff; rs.depthBiasEnable = e.dbe;
    VkPipelineDepthStencilStateCreateInfo ds = p->ds;
    if (p->has_ds) {
        ds.depthTestEnable = e.dt; ds.depthWriteEnable = e.dw; ds.depthCompareOp = (VkCompareOp)e.dop; ds.stencilTestEnable = e.sten;
        ds.front = (VkStencilOpState){(VkStencilOp)e.sop[0][0], (VkStencilOp)e.sop[0][1], (VkStencilOp)e.sop[0][2], (VkCompareOp)e.sop[0][3], 0, 0, 0};
        ds.back = (VkStencilOpState){(VkStencilOp)e.sop[1][0], (VkStencilOp)e.sop[1][1], (VkStencilOp)e.sop[1][2], (VkCompareOp)e.sop[1][3], 0, 0, 0};
    }
    VkGraphicsPipelineCreateInfo ci = p->ci;
    ci.pInputAssemblyState = &ia; ci.pRasterizationState = &rs;
    if (p->has_ds) ci.pDepthStencilState = &ds;
    VkPipeline out = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(p->dev, p->cache, 1, &ci, NULL, &out) != VK_SUCCESS) out = VK_NULL_HANDLE;
    if (p->nvar == p->capvar) { p->capvar = p->capvar ? p->capvar * 2 : 4; p->var = realloc(p->var, p->capvar * sizeof *p->var); }
    p->var[p->nvar++] = (Variant){e, out};
    n_variants++;
    return out;
}

uint32_t vkc_variant_count(void) { return n_variants; }

static void vp_free(VPipe *p, VkDevice dev) {
    for (uint32_t i = 0; i < p->nvar; i++) if (p->var[i].pipe) vkDestroyPipeline(dev, p->var[i].pipe, NULL);
    for (int i = 0; i < 5; i++) free(p->names[i]);
    free(p->stages); free(p->vb); free(p->va); free(p->vd); free(p->cba); free(p->dyn); free(p->var); free(p);
}

// ---- state recording (a no-op for the driver)
void vkc_CmdSetCullMode(VkCommandBuffer cb, VkCullModeFlags m) { if (g_vkc_native) vkCmdSetCullMode(cb, m); else g_ext.cull = (uint8_t)m; }
void vkc_CmdSetFrontFace(VkCommandBuffer cb, VkFrontFace f) { if (g_vkc_native) vkCmdSetFrontFace(cb, f); else g_ext.ff = (uint8_t)f; }
void vkc_CmdSetPrimitiveTopology(VkCommandBuffer cb, VkPrimitiveTopology t) { if (g_vkc_native) vkCmdSetPrimitiveTopology(cb, t); else { g_ext.topo = (uint8_t)t; g_ext.have_topo = 1; } }
void vkc_CmdSetDepthTestEnable(VkCommandBuffer cb, VkBool32 e) { if (g_vkc_native) vkCmdSetDepthTestEnable(cb, e); else g_ext.dt = e != 0; }
void vkc_CmdSetDepthWriteEnable(VkCommandBuffer cb, VkBool32 e) { if (g_vkc_native) vkCmdSetDepthWriteEnable(cb, e); else g_ext.dw = e != 0; }
void vkc_CmdSetDepthCompareOp(VkCommandBuffer cb, VkCompareOp op) { if (g_vkc_native) vkCmdSetDepthCompareOp(cb, op); else g_ext.dop = (uint8_t)op; }
void vkc_CmdSetStencilTestEnable(VkCommandBuffer cb, VkBool32 e) { if (g_vkc_native) vkCmdSetStencilTestEnable(cb, e); else g_ext.sten = e != 0; }
void vkc_CmdSetStencilOp(VkCommandBuffer cb, VkStencilFaceFlags face, VkStencilOp fail, VkStencilOp pass, VkStencilOp dfail, VkCompareOp cmp) {
    if (g_vkc_native) { vkCmdSetStencilOp(cb, face, fail, pass, dfail, cmp); return; }
    for (int f = 0; f < 2; f++)
        if (face & (f ? VK_STENCIL_FACE_BACK_BIT : VK_STENCIL_FACE_FRONT_BIT))
            memcpy(g_ext.sop[f], (uint8_t[4]){(uint8_t)fail, (uint8_t)pass, (uint8_t)dfail, (uint8_t)cmp}, 4);
}
void vkc_CmdSetPrimitiveRestartEnable(VkCommandBuffer cb, VkBool32 e) { if (g_vkc_native) vkCmdSetPrimitiveRestartEnable(cb, e); else g_ext.pre = e != 0; }
void vkc_CmdSetDepthBiasEnable(VkCommandBuffer cb, VkBool32 e) { if (g_vkc_native) vkCmdSetDepthBiasEnable(cb, e); else g_ext.dbe = e != 0; }

// ---- binding and drawing
static _Thread_local struct { VPipe *cur; VkPipeline real; ExtSt ext; VkCommandBuffer cb; bool dirty; } g_bind;

void vkc_CmdBindPipeline(VkCommandBuffer cb, VkPipelineBindPoint bp, VkPipeline p) {
    if (g_vkc_native || bp != VK_PIPELINE_BIND_POINT_GRAPHICS) { vkCmdBindPipeline(cb, bp, p); return; }
    pthread_mutex_lock(&vk_mu);
    VPipe *vp = vp_of(p);
    pthread_mutex_unlock(&vk_mu);
    if (!vp) { g_bind.cur = NULL; g_bind.real = p; g_bind.cb = cb; vkCmdBindPipeline(cb, bp, p); return; }  // (a plain pipeline)
    g_bind.cur = vp; g_bind.dirty = true;
}

static bool pre_draw(VkCommandBuffer cb) {  // false: the variant couldn't be built, skip the draw
    if (g_vkc_native || !g_bind.cur) return true;
    if (!g_bind.dirty && g_bind.cb == cb && !memcmp(&g_bind.ext, &g_ext, sizeof g_ext)) return true;
    pthread_mutex_lock(&vk_mu);
    VkPipeline p = vp_variant(g_bind.cur, &g_ext);
    pthread_mutex_unlock(&vk_mu);
    if (!p) return false;
    if (p != g_bind.real || g_bind.cb != cb) vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, p);
    g_bind.real = p; g_bind.ext = g_ext; g_bind.cb = cb; g_bind.dirty = false;
    return true;
}
void vkc_CmdDraw(VkCommandBuffer cb, uint32_t vc, uint32_t ic, uint32_t fv, uint32_t fi) { if (pre_draw(cb)) vkCmdDraw(cb, vc, ic, fv, fi); }
void vkc_CmdDrawIndexed(VkCommandBuffer cb, uint32_t ic, uint32_t inst, uint32_t fi, int32_t vo, uint32_t first_inst) {
    if (pre_draw(cb)) vkCmdDrawIndexed(cb, ic, inst, fi, vo, first_inst);
}

void vkc_DestroyPipeline(VkDevice dev, VkPipeline p, const VkAllocationCallbacks *ac) {
    if (!g_vkc_native && p) {
        pthread_mutex_lock(&vk_mu);
        VPipe *vp = vp_of(p);
        if (vp) {
            vps[(size_t)((uint64_t)p & 0xFFFFFFFFull) - 1] = NULL;
            pthread_mutex_unlock(&vk_mu);
            if (g_bind.cur == vp) g_bind.cur = NULL;
            vp_free(vp, dev);
            return;
        }
        pthread_mutex_unlock(&vk_mu);
    }
    vkDestroyPipeline(dev, p, ac);
}

// ---- pipelines: VkPipelineRenderingCreateInfo (first in the pNext chain, as the renderer builds them) -> a compatible
// VkRenderPass, then the create info becomes a virtual pipeline.
VkResult vkc_CreateGraphicsPipelines(VkDevice dev, VkPipelineCache cache, uint32_t n, const VkGraphicsPipelineCreateInfo *ci,
                                     const VkAllocationCallbacks *ac, VkPipeline *out) {
    if (g_vkc_native) return vkCreateGraphicsPipelines(dev, cache, n, ci, ac, out);
    if (!g_dev) g_dev = dev;
    VkGraphicsPipelineCreateInfo local[n ? n : 1];
    pthread_mutex_lock(&vk_mu);
    for (uint32_t i = 0; i < n; i++) {
        local[i] = ci[i];
        const VkPipelineRenderingCreateInfo *r = (const VkPipelineRenderingCreateInfo *)ci[i].pNext;
        if (!r || r->sType != VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO) continue;  // (not a dynamic-rendering pipeline)
        assert(r->colorAttachmentCount <= MAXC && r->viewMask == 0);
        RpKey k;
        memset(&k, 0, sizeof k);
        k.ncolor = r->colorAttachmentCount;
        k.samples = ci[i].pMultisampleState ? (uint32_t)ci[i].pMultisampleState->rasterizationSamples : 1;
        for (uint32_t c = 0; c < k.ncolor; c++) {
            k.color[c] = r->pColorAttachmentFormats[c];
            k.cload[c] = VK_ATTACHMENT_LOAD_OP_LOAD; k.cstore[c] = VK_ATTACHMENT_STORE_OP_STORE; k.clay[c] = VK_IMAGE_LAYOUT_GENERAL;
        }
        k.ds = r->depthAttachmentFormat != VK_FORMAT_UNDEFINED ? r->depthAttachmentFormat : r->stencilAttachmentFormat;
        k.dload = k.sload = VK_ATTACHMENT_LOAD_OP_LOAD; k.dstore = k.sstore = VK_ATTACHMENT_STORE_OP_STORE; k.dlay = VK_IMAGE_LAYOUT_GENERAL;
        local[i].pNext = r->pNext;
        local[i].renderPass = get_rp(&k);
        local[i].subpass = 0;
    }
    // virtual pipelines: the extended dynamic state is baked per draw (vp_variant)
    VkResult res = VK_SUCCESS;
    uint32_t done = 0;
    for (; done < n; done++) {
        VPipe *vp = vp_make(dev, cache, &local[done]);
        if (!vp) break;  // (something we can't copy: the rest go to the driver as they are)
        size_t slot = 0;
        while (slot < nvps && vps[slot]) slot++;
        if (slot == nvps) { vps = realloc(vps, (nvps + 64) * sizeof *vps); memset(vps + nvps, 0, 64 * sizeof *vps); nvps += 64; }
        vps[slot] = vp;
        out[done] = (VkPipeline)(VTAG | (slot + 1));
    }
    pthread_mutex_unlock(&vk_mu);
    if (done < n) {
        for (uint32_t i = 0; i < done; i++) vkc_DestroyPipeline(dev, out[i], NULL);
        for (uint32_t i = 0; i < n; i++) local[i].flags &= ~(VkPipelineCreateFlags)(0x100 | 0x200);
        res = vkCreateGraphicsPipelines(dev, cache, n, local, ac, out);
    }
    return res;
}

// ---- timeline semaphores: vkWaitSemaphores / vkGetSemaphoreCounterValue are 1.2; a 1.1 device has the KHR names only
static PFN_vkWaitSemaphoresKHR pfn_wait_khr;
static PFN_vkGetSemaphoreCounterValueKHR pfn_value_khr;
void vkc_use_khr_semaphores(VkDevice dev) {
    pfn_wait_khr = (PFN_vkWaitSemaphoresKHR)vkGetDeviceProcAddr(dev, "vkWaitSemaphoresKHR");
    pfn_value_khr = (PFN_vkGetSemaphoreCounterValueKHR)vkGetDeviceProcAddr(dev, "vkGetSemaphoreCounterValueKHR");
}
VkResult vkc_WaitSemaphores(VkDevice dev, const VkSemaphoreWaitInfo *wi, uint64_t timeout) {
    return pfn_wait_khr ? pfn_wait_khr(dev, wi, timeout) : vkWaitSemaphores(dev, wi, timeout);
}
VkResult vkc_GetSemaphoreCounterValue(VkDevice dev, VkSemaphore s, uint64_t *value) {
    return pfn_value_khr ? pfn_value_khr(dev, s, value) : vkGetSemaphoreCounterValue(dev, s, value);
}
