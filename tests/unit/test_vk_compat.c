// Unit test for runtime/nvn/vulkan/vk_compat.c: legacy translation of sync2 calls, with the Vulkan entry points stubbed.
// Build: zig cc tests/unit/test_vk_compat.c runtime/nvn/vulkan/vk_compat.c -Iruntime/nvn/vulkan -o t && ./t
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VKC_NO_REDIRECT  // (the stubs below define the real Vulkan names)
#include "vk_compat.h"

static VkPipelineStageFlags g_src, g_dst; static uint32_t g_nm, g_nb, g_ni; static VkMemoryBarrier g_mb0; static VkImageMemoryBarrier g_ib0;
static VkSubmitInfo g_sub[4]; static VkTimelineSemaphoreSubmitInfo g_tl; static int g_nsub; static VkFence g_fence;
static VkPipelineStageFlagBits g_ts; static uint32_t g_vb_count;

void vkCmdPipelineBarrier(VkCommandBuffer cb, VkPipelineStageFlags s, VkPipelineStageFlags d, VkDependencyFlags f, uint32_t nm,
                          const VkMemoryBarrier *mb, uint32_t nb, const VkBufferMemoryBarrier *bb, uint32_t ni, const VkImageMemoryBarrier *ib) {
    (void)cb; (void)f; (void)bb; g_src = s; g_dst = d; g_nm = nm; g_nb = nb; g_ni = ni;
    if (nm) g_mb0 = mb[0];
    if (ni) g_ib0 = ib[0];
}
VkResult vkQueueSubmit(VkQueue q, uint32_t n, const VkSubmitInfo *si, VkFence f) {
    (void)q; assert(n == 1); g_sub[g_nsub] = si[0]; g_tl = *(const VkTimelineSemaphoreSubmitInfo *)si[0].pNext; g_nsub++; g_fence = f; return VK_SUCCESS;
}
void vkCmdWriteTimestamp(VkCommandBuffer cb, VkPipelineStageFlagBits s, VkQueryPool p, uint32_t q) { (void)cb; (void)p; (void)q; g_ts = s; }
void vkCmdBindVertexBuffers(VkCommandBuffer cb, uint32_t first, uint32_t c, const VkBuffer *b, const VkDeviceSize *o) { (void)cb; (void)first; (void)b; (void)o; g_vb_count = c; }
void vkCmdPipelineBarrier2(VkCommandBuffer cb, const VkDependencyInfo *di) { (void)cb; (void)di; assert(!"native path must not be used"); }
VkResult vkQueueSubmit2(VkQueue q, uint32_t n, const VkSubmitInfo2 *si, VkFence f) { (void)q; (void)n; (void)si; (void)f; assert(!"native"); return VK_SUCCESS; }
void vkCmdWriteTimestamp2(VkCommandBuffer cb, VkPipelineStageFlags2 s, VkQueryPool p, uint32_t q) { (void)cb; (void)s; (void)p; (void)q; assert(!"native"); }
void vkCmdBindVertexBuffers2(VkCommandBuffer cb, uint32_t a, uint32_t c, const VkBuffer *b, const VkDeviceSize *o, const VkDeviceSize *s, const VkDeviceSize *st) { (void)cb; (void)a; (void)c; (void)b; (void)o; (void)s; (void)st; assert(!"native"); }

// ---- dynamic rendering stubs
VkResult vkWaitSemaphores(VkDevice d, const VkSemaphoreWaitInfo *w, uint64_t to) { (void)d; (void)w; (void)to; return VK_SUCCESS; }
VkResult vkGetSemaphoreCounterValue(VkDevice d, VkSemaphore s, uint64_t *v) { (void)d; (void)s; *v = 7; return VK_SUCCESS; }
PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice d, const char *n) { (void)d; (void)n; return NULL; }
static int n_rp_create, n_fb_create, n_fb_destroy, n_begin, n_end, n_pipe;
static VkAttachmentDescription g_att[9]; static uint32_t g_natt; static VkAttachmentReference g_cref[8]; static uint32_t g_ncref; static int g_has_dref; static VkAttachmentReference g_dref;
static VkFramebufferCreateInfo g_fbci; static VkRenderPassBeginInfo g_bi; static VkClearValue g_clr[9];
static VkRenderPass g_pipe_rp; static const void *g_pipe_pnext;
static uint64_t g_next_handle = 100;
VkResult vkCreateImageView(VkDevice d, const VkImageViewCreateInfo *ci, const VkAllocationCallbacks *a, VkImageView *o) { (void)d; (void)ci; (void)a; *o = (VkImageView)(uintptr_t)(g_next_handle++); return VK_SUCCESS; }
void vkDestroyImageView(VkDevice d, VkImageView v, const VkAllocationCallbacks *a) { (void)d; (void)v; (void)a; }
VkResult vkCreateRenderPass(VkDevice d, const VkRenderPassCreateInfo *ci, const VkAllocationCallbacks *a, VkRenderPass *o) {
    (void)d; (void)a; n_rp_create++; g_natt = ci->attachmentCount; memcpy(g_att, ci->pAttachments, g_natt * sizeof *g_att);
    g_ncref = ci->pSubpasses[0].colorAttachmentCount; if (g_ncref) memcpy(g_cref, ci->pSubpasses[0].pColorAttachments, g_ncref * sizeof *g_cref);
    g_has_dref = ci->pSubpasses[0].pDepthStencilAttachment != NULL; if (g_has_dref) g_dref = *ci->pSubpasses[0].pDepthStencilAttachment;
    *o = (VkRenderPass)(uintptr_t)(g_next_handle++); return VK_SUCCESS;
}
VkResult vkCreateFramebuffer(VkDevice d, const VkFramebufferCreateInfo *ci, const VkAllocationCallbacks *a, VkFramebuffer *o) { (void)d; (void)a; n_fb_create++; g_fbci = *ci; *o = (VkFramebuffer)(uintptr_t)(g_next_handle++); return VK_SUCCESS; }
void vkDestroyFramebuffer(VkDevice d, VkFramebuffer f, const VkAllocationCallbacks *a) { (void)d; (void)f; (void)a; n_fb_destroy++; }
void vkCmdBeginRenderPass(VkCommandBuffer cb, const VkRenderPassBeginInfo *bi, VkSubpassContents c) { (void)cb; (void)c; n_begin++; g_bi = *bi; memcpy(g_clr, bi->pClearValues, bi->clearValueCount * sizeof *g_clr); }
void vkCmdEndRenderPass(VkCommandBuffer cb) { (void)cb; n_end++; }
static VkGraphicsPipelineCreateInfo g_pci; static VkPipelineInputAssemblyStateCreateInfo g_pia; static VkPipelineRasterizationStateCreateInfo g_prs;
static VkPipelineDepthStencilStateCreateInfo g_pds; static uint32_t g_pndyn; static VkDynamicState g_pdyn[32]; static int n_pipe_destroy, n_bind, n_draw, n_drawi;
static VkPipeline g_bound;
VkResult vkCreateGraphicsPipelines(VkDevice d, VkPipelineCache c, uint32_t n, const VkGraphicsPipelineCreateInfo *ci, const VkAllocationCallbacks *a, VkPipeline *o) {
    (void)d; (void)c; (void)a; assert(n == 1); n_pipe++; g_pci = ci[0]; g_pia = *ci[0].pInputAssemblyState; g_prs = *ci[0].pRasterizationState;
    if (ci[0].pDepthStencilState) g_pds = *ci[0].pDepthStencilState;
    g_pndyn = ci[0].pDynamicState->dynamicStateCount; memcpy(g_pdyn, ci[0].pDynamicState->pDynamicStates, g_pndyn * sizeof g_pdyn[0]);
    g_pipe_rp = ci[0].renderPass; g_pipe_pnext = ci[0].pNext; *o = (VkPipeline)(uintptr_t)(g_next_handle++); return VK_SUCCESS;
}
void vkDestroyPipeline(VkDevice d, VkPipeline p, const VkAllocationCallbacks *a) { (void)d; (void)p; (void)a; n_pipe_destroy++; }
void vkCmdBindPipeline(VkCommandBuffer cb, VkPipelineBindPoint bp, VkPipeline p) { (void)cb; (void)bp; n_bind++; g_bound = p; }
void vkCmdDraw(VkCommandBuffer cb, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { (void)cb; (void)a; (void)b; (void)c; (void)d; n_draw++; }
void vkCmdDrawIndexed(VkCommandBuffer cb, uint32_t a, uint32_t b, uint32_t c, int32_t d, uint32_t e) { (void)cb; (void)a; (void)b; (void)c; (void)d; (void)e; n_drawi++; }
#define NATIVE_STUB1(fn, T) void fn(VkCommandBuffer cb, T v) { (void)cb; (void)v; assert(!"native"); }
NATIVE_STUB1(vkCmdSetCullMode, VkCullModeFlags) NATIVE_STUB1(vkCmdSetFrontFace, VkFrontFace) NATIVE_STUB1(vkCmdSetPrimitiveTopology, VkPrimitiveTopology)
NATIVE_STUB1(vkCmdSetDepthTestEnable, VkBool32) NATIVE_STUB1(vkCmdSetDepthWriteEnable, VkBool32) NATIVE_STUB1(vkCmdSetDepthCompareOp, VkCompareOp)
NATIVE_STUB1(vkCmdSetStencilTestEnable, VkBool32) NATIVE_STUB1(vkCmdSetPrimitiveRestartEnable, VkBool32) NATIVE_STUB1(vkCmdSetDepthBiasEnable, VkBool32)
void vkCmdSetStencilOp(VkCommandBuffer cb, VkStencilFaceFlags f, VkStencilOp a, VkStencilOp b, VkStencilOp c, VkCompareOp d) { (void)cb; (void)f; (void)a; (void)b; (void)c; (void)d; assert(!"native"); }
void vkCmdBeginRendering(VkCommandBuffer cb, const VkRenderingInfo *ri) { (void)cb; (void)ri; assert(!"native"); }
void vkCmdEndRendering(VkCommandBuffer cb) { (void)cb; assert(!"native"); }

int main(void) {
    g_vkc_native = false;
    // masks
    assert(vkc_stage2_to_1(VK_PIPELINE_STAGE_2_NONE, true) == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    assert(vkc_stage2_to_1(VK_PIPELINE_STAGE_2_NONE, false) == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    assert(vkc_stage2_to_1(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, true) == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    assert(vkc_stage2_to_1(VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, false) ==
           (VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));
    assert(vkc_stage2_to_1(VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT, false) == VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
    assert(vkc_access2_to_1(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT) == VK_ACCESS_SHADER_READ_BIT);
    assert(vkc_access2_to_1(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT) == (VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT));
    assert(vkc_access2_to_1(VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT) == (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT));

    // the renderer's vk_full_barrier
    VkMemoryBarrier2 mb = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                           .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT, .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                           .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT};
    VkDependencyInfo di = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .memoryBarrierCount = 1, .pMemoryBarriers = &mb};
    vkc_CmdPipelineBarrier2((VkCommandBuffer)1, &di);
    assert(g_nm == 1 && g_src == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT && g_dst == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    assert(g_mb0.srcAccessMask == VK_ACCESS_MEMORY_WRITE_BIT && g_mb0.dstAccessMask == (VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT));

    // an image barrier with layouts and a range
    VkImageMemoryBarrier2 im = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = (VkImage)7,
                                .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkDependencyInfo di2 = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &im};
    vkc_CmdPipelineBarrier2((VkCommandBuffer)1, &di2);
    assert(g_ni == 1 && g_src == VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT && g_dst == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    assert(g_ib0.dstAccessMask == VK_ACCESS_SHADER_READ_BIT && g_ib0.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && g_ib0.image == (VkImage)7);

    // submit: wait on a binary sem, signal the timeline (value 42) + a binary one
    VkSemaphoreSubmitInfo wt = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = (VkSemaphore)3, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
    VkSemaphoreSubmitInfo sg[2] = {{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = (VkSemaphore)4, .value = 42, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT},
                                   {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = (VkSemaphore)5, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT}};
    VkCommandBufferSubmitInfo cbi[2] = {{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = (VkCommandBuffer)10},
                                        {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = (VkCommandBuffer)11}};
    VkSubmitInfo2 s2 = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .waitSemaphoreInfoCount = 1, .pWaitSemaphoreInfos = &wt, .commandBufferInfoCount = 2,
                        .pCommandBufferInfos = cbi, .signalSemaphoreInfoCount = 2, .pSignalSemaphoreInfos = sg};
    assert(vkc_QueueSubmit2((VkQueue)1, 1, &s2, (VkFence)9) == VK_SUCCESS);
    assert(g_nsub == 1 && g_fence == (VkFence)9 && g_sub[0].waitSemaphoreCount == 1 && g_sub[0].commandBufferCount == 2 && g_sub[0].signalSemaphoreCount == 2);
    assert(g_sub[0].pWaitDstStageMask[0] == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT && g_tl.signalSemaphoreValueCount == 2 && g_tl.waitSemaphoreValueCount == 1);

    // timestamps and vertex buffers
    vkc_CmdWriteTimestamp2((VkCommandBuffer)1, VK_PIPELINE_STAGE_2_NONE, (VkQueryPool)1, 0);
    assert(g_ts == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    vkc_CmdWriteTimestamp2((VkCommandBuffer)1, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, (VkQueryPool)1, 1);
    assert(g_ts == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    VkBuffer b = (VkBuffer)1; VkDeviceSize o = 0, sz = 64;
    vkc_CmdBindVertexBuffers2((VkCommandBuffer)1, 0, 1, &b, &o, &sz, NULL);
    assert(g_vb_count == 1);

    // ---- dynamic rendering
    vkc_init((VkDevice)1, false);
    VkImageViewCreateInfo vci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkImageView cv, cv2, dv;
    vci.format = VK_FORMAT_R8G8B8A8_UNORM; vkc_CreateImageView((VkDevice)1, &vci, NULL, &cv);
    vci.format = VK_FORMAT_R16G16B16A16_SFLOAT; vkc_CreateImageView((VkDevice)1, &vci, NULL, &cv2);
    vci.format = VK_FORMAT_D24_UNORM_S8_UINT; vkc_CreateImageView((VkDevice)1, &vci, NULL, &dv);

    VkRenderingAttachmentInfo ca[2] = {{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = cv, .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE},
                                       {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO}};  // slot 1: empty
    ca[0].clearValue.color.float32[3] = 1.0f;
    VkRenderingAttachmentInfo da = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = dv, .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE};
    da.clearValue.depthStencil.depth = 0.5f;
    VkRenderingAttachmentInfo sa = da; sa.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; sa.clearValue.depthStencil.stencil = 7;
    VkRenderingInfo ri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .renderArea = {{0, 0}, {1280, 720}}, .layerCount = 1,
                          .colorAttachmentCount = 2, .pColorAttachments = ca, .pDepthAttachment = &da, .pStencilAttachment = &sa};
    g_vkc_samples = 1;
    vkc_CmdBeginRendering((VkCommandBuffer)1, &ri);
    assert(n_begin == 1 && n_rp_create == 1 && n_fb_create == 1);
    assert(g_natt == 2 && g_att[0].format == VK_FORMAT_R8G8B8A8_UNORM && g_att[0].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(g_att[1].format == VK_FORMAT_D24_UNORM_S8_UINT && g_att[1].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR && g_att[1].stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
    assert(g_ncref == 2 && g_cref[0].attachment == 0 && g_cref[1].attachment == VK_ATTACHMENT_UNUSED && g_has_dref && g_dref.attachment == 1);
    assert(g_fbci.attachmentCount == 2 && g_fbci.width == 1280 && g_fbci.height == 720 && g_fbci.layers == 1);
    assert(g_bi.clearValueCount == 2 && g_clr[0].color.float32[3] == 1.0f && g_clr[1].depthStencil.depth == 0.5f && g_clr[1].depthStencil.stencil == 7);
    vkc_CmdEndRendering((VkCommandBuffer)1);
    assert(n_end == 1);

    vkc_CmdBeginRendering((VkCommandBuffer)1, &ri);  // same pass again: cached render pass and framebuffer
    assert(n_begin == 2 && n_rp_create == 1 && n_fb_create == 1);

    // a pipeline for that pass: same formats and samples, VkPipelineRenderingCreateInfo gone, a render pass in its place. The pipeline is
    // virtual: nothing reaches the driver until a draw needs it, with the extended dynamic state baked in.
    VkFormat pf[2] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_UNDEFINED};
    VkPipelineRenderingCreateInfo pr = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, .colorAttachmentCount = 2, .pColorAttachmentFormats = pf,
                                        .depthAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT, .stencilAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT};
    VkPipelineMultisampleStateCreateInfo ms = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
    VkPipelineShaderStageCreateInfo stg[2] = {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .pName = "main"},
                                              {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .pName = "main"}};
    VkPipelineVertexInputStateCreateInfo vi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkPipelineViewportStateCreateInfo vsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1};
    VkPipelineRasterizationStateCreateInfo rsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .lineWidth = 1};
    VkPipelineDepthStencilStateCreateInfo dsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState cba[2] = {{.colorWriteMask = 0xF}, {0}};
    VkPipelineColorBlendStateCreateInfo cbsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 2, .pAttachments = cba};
    VkDynamicState dyns[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_CULL_MODE, VK_DYNAMIC_STATE_FRONT_FACE,
                             VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, VK_DYNAMIC_STATE_STENCIL_OP, VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                             VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE};
    VkPipelineDynamicStateCreateInfo dyi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = sizeof dyns / sizeof dyns[0], .pDynamicStates = dyns};
    VkGraphicsPipelineCreateInfo pi = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .pNext = &pr, .stageCount = 2, .pStages = stg, .pVertexInputState = &vi,
                                       .pInputAssemblyState = &ia, .pViewportState = &vsi, .pRasterizationState = &rsi, .pMultisampleState = &ms, .pDepthStencilState = &dsi,
                                       .pColorBlendState = &cbsi, .pDynamicState = &dyi, .flags = 0x100};
    VkPipeline pl;
    assert(vkc_CreateGraphicsPipelines((VkDevice)1, VK_NULL_HANDLE, 1, &pi, NULL, &pl) == VK_SUCCESS);
    assert(n_pipe == 0 && pl != VK_NULL_HANDLE);  // virtual: the driver hasn't seen it
    assert(pi.pNext == &pr);                       // (the caller's structure is untouched)
    stg[0].pName = "gone"; ia.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;  // (the caller's structs are reused after the call: a deep copy must not care)

    vkc_CmdBindPipeline((VkCommandBuffer)1, VK_PIPELINE_BIND_POINT_GRAPHICS, pl);
    assert(n_bind == 0);  // (binding is deferred to the draw)
    vkc_CmdSetCullMode((VkCommandBuffer)1, VK_CULL_MODE_BACK_BIT);
    vkc_CmdSetFrontFace((VkCommandBuffer)1, VK_FRONT_FACE_CLOCKWISE);
    vkc_CmdSetPrimitiveTopology((VkCommandBuffer)1, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    vkc_CmdSetPrimitiveRestartEnable((VkCommandBuffer)1, VK_TRUE);
    vkc_CmdSetDepthTestEnable((VkCommandBuffer)1, VK_TRUE);
    vkc_CmdSetDepthWriteEnable((VkCommandBuffer)1, VK_TRUE);
    vkc_CmdSetDepthCompareOp((VkCommandBuffer)1, VK_COMPARE_OP_LESS_OR_EQUAL);
    vkc_CmdSetStencilTestEnable((VkCommandBuffer)1, VK_TRUE);
    vkc_CmdSetStencilOp((VkCommandBuffer)1, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_ZERO, VK_COMPARE_OP_EQUAL);
    vkc_CmdSetStencilOp((VkCommandBuffer)1, VK_STENCIL_FACE_BACK_BIT, VK_STENCIL_OP_INVERT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS);
    vkc_CmdSetDepthBiasEnable((VkCommandBuffer)1, VK_TRUE);
    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);
    assert(n_pipe == 1 && n_bind == 1 && n_draw == 1 && vkc_variant_count() == 1);
    assert(g_pipe_pnext == NULL && g_pipe_rp != VK_NULL_HANDLE && (g_pci.flags & 0x100) == 0 && strcmp(g_pci.pStages[0].pName, "main") == 0);
    assert(g_pia.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP && g_pia.primitiveRestartEnable == VK_TRUE);
    assert(g_prs.cullMode == VK_CULL_MODE_BACK_BIT && g_prs.frontFace == VK_FRONT_FACE_CLOCKWISE && g_prs.depthBiasEnable == VK_TRUE);
    assert(g_pds.depthTestEnable && g_pds.depthWriteEnable && g_pds.depthCompareOp == VK_COMPARE_OP_LESS_OR_EQUAL && g_pds.stencilTestEnable);
    assert(g_pds.front.passOp == VK_STENCIL_OP_REPLACE && g_pds.front.depthFailOp == VK_STENCIL_OP_ZERO && g_pds.front.compareOp == VK_COMPARE_OP_EQUAL);
    assert(g_pds.back.failOp == VK_STENCIL_OP_INVERT && g_pds.back.compareOp == VK_COMPARE_OP_ALWAYS);
    assert(g_pndyn == 4);  // viewport, scissor, depth bias values, stencil reference: what a 1.1 pipeline can still change at draw time
    for (uint32_t i = 0; i < g_pndyn; i++) assert(g_pdyn[i] == VK_DYNAMIC_STATE_VIEWPORT || g_pdyn[i] == VK_DYNAMIC_STATE_SCISSOR || g_pdyn[i] == VK_DYNAMIC_STATE_DEPTH_BIAS || g_pdyn[i] == VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    assert(n_rp_create == 2 && g_natt == 2 && g_att[0].format == VK_FORMAT_R8G8B8A8_UNORM && g_att[1].format == VK_FORMAT_D24_UNORM_S8_UINT && g_cref[1].attachment == VK_ATTACHMENT_UNUSED);

    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);  // same pipeline and state: nothing new
    vkc_CmdDrawIndexed((VkCommandBuffer)1, 3, 1, 0, 0, 0);
    assert(n_pipe == 1 && n_bind == 1 && n_draw == 2 && n_drawi == 1);
    VkPipeline first = g_bound;
    vkc_CmdSetCullMode((VkCommandBuffer)1, VK_CULL_MODE_NONE);  // another state: a second variant, bound before its draw
    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);
    assert(n_pipe == 2 && n_bind == 2 && g_bound != first && g_prs.cullMode == VK_CULL_MODE_NONE);
    vkc_CmdSetCullMode((VkCommandBuffer)1, VK_CULL_MODE_BACK_BIT);  // back to the first: found, not rebuilt
    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);
    assert(n_pipe == 2 && n_bind == 3 && g_bound == first);
    vkc_CmdSetDepthTestEnable((VkCommandBuffer)1, VK_FALSE);  // depth test off: write/compare are normalised away, one variant for all of them
    vkc_CmdSetDepthWriteEnable((VkCommandBuffer)1, VK_FALSE);
    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);
    vkc_CmdSetDepthCompareOp((VkCommandBuffer)1, VK_COMPARE_OP_ALWAYS);
    vkc_CmdDraw((VkCommandBuffer)1, 3, 1, 0, 0);
    assert(n_pipe == 3 && vkc_variant_count() == 3);
    vkc_DestroyPipeline((VkDevice)1, pl, NULL);  // its variants go with it
    assert(n_pipe_destroy == 3);

    // MSAA: a different render pass
    VkImageView msv; vci.format = VK_FORMAT_R8G8B8A8_UNORM; vkc_CreateImageView((VkDevice)1, &vci, NULL, &msv);
    VkRenderingAttachmentInfo mca = ca[0]; mca.imageView = msv;
    VkRenderingInfo mri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .renderArea = {{0, 0}, {640, 360}}, .layerCount = 1, .colorAttachmentCount = 1, .pColorAttachments = &mca};
    g_vkc_samples = 4;
    vkc_CmdBeginRendering((VkCommandBuffer)1, &mri);
    assert(n_rp_create == 3 && g_att[0].samples == VK_SAMPLE_COUNT_4_BIT && g_has_dref == 0);
    g_vkc_samples = 1;

    // destroying a view drops the framebuffers that use it (a recycled handle must not hit a stale one)
    int fbd = n_fb_destroy;
    vkc_DestroyImageView((VkDevice)1, cv, NULL);
    assert(n_fb_destroy == fbd + 1);
    (void)cv2;
    // semaphores: without the KHR entry points the core ones answer
    vkc_use_khr_semaphores((VkDevice)1);
    uint64_t sv = 0;
    assert(vkc_GetSemaphoreCounterValue((VkDevice)1, (VkSemaphore)1, &sv) == VK_SUCCESS && sv == 7);
    assert(vkc_WaitSemaphores((VkDevice)1, NULL, 0) == VK_SUCCESS);
    puts("vk_compat: OK");
    return 0;
}
