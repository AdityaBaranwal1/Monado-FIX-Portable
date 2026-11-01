// Vulkan 1.1 compatibility shims (Android port, Mali / Helio G99 class GPUs).
// The renderer is written against Vulkan 1.3 (synchronization2). Where a driver only has 1.1 + extensions, the calls
// below translate to their 1.0/1.1 equivalents. g_vkc_native is true (default) when the device really enabled
// synchronization2: then every call goes straight to the driver and nothing changes on desktop.
#pragma once
#include <stdbool.h>
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool g_vkc_native;  // false when the device was created without sync2 / dynamic rendering (Vulkan 1.1 class GPU)
extern _Thread_local uint32_t g_vkc_samples;  // sample count of the next vkCmdBeginRendering on this thread (default 1)
void vkc_init(VkDevice dev, bool native);      // device.c: right after vkCreateDevice

void vkc_CmdPipelineBarrier2(VkCommandBuffer cb, const VkDependencyInfo *di);
VkResult vkc_QueueSubmit2(VkQueue q, uint32_t n, const VkSubmitInfo2 *si, VkFence fence);
void vkc_CmdWriteTimestamp2(VkCommandBuffer cb, VkPipelineStageFlags2 stage, VkQueryPool pool, uint32_t query);
void vkc_CmdBindVertexBuffers2(VkCommandBuffer cb, uint32_t first, uint32_t count, const VkBuffer *bufs, const VkDeviceSize *offs,
                               const VkDeviceSize *sizes, const VkDeviceSize *strides);

// dynamic rendering -> render pass + framebuffer (cached); pipelines get the matching VkRenderPass instead of VkPipelineRenderingCreateInfo
void vkc_CmdBeginRendering(VkCommandBuffer cb, const VkRenderingInfo *ri);
void vkc_CmdEndRendering(VkCommandBuffer cb);
VkResult vkc_CreateGraphicsPipelines(VkDevice dev, VkPipelineCache cache, uint32_t n, const VkGraphicsPipelineCreateInfo *ci,
                                     const VkAllocationCallbacks *ac, VkPipeline *out);
// extended dynamic state (cull / front face / topology / depth / stencil ops / restart / bias enable) does not exist on a 1.1 GPU:
// pipelines become "virtual" (their create info is kept) and the real pipeline for the state set at draw time is built on the first
// draw that needs it (one per combination), bound right before the draw.
void vkc_CmdSetCullMode(VkCommandBuffer cb, VkCullModeFlags m);
void vkc_CmdSetFrontFace(VkCommandBuffer cb, VkFrontFace f);
void vkc_CmdSetPrimitiveTopology(VkCommandBuffer cb, VkPrimitiveTopology t);
void vkc_CmdSetDepthTestEnable(VkCommandBuffer cb, VkBool32 e);
void vkc_CmdSetDepthWriteEnable(VkCommandBuffer cb, VkBool32 e);
void vkc_CmdSetDepthCompareOp(VkCommandBuffer cb, VkCompareOp op);
void vkc_CmdSetStencilTestEnable(VkCommandBuffer cb, VkBool32 e);
void vkc_CmdSetStencilOp(VkCommandBuffer cb, VkStencilFaceFlags face, VkStencilOp fail, VkStencilOp pass, VkStencilOp depth_fail, VkCompareOp cmp);
void vkc_CmdSetPrimitiveRestartEnable(VkCommandBuffer cb, VkBool32 e);
void vkc_CmdSetDepthBiasEnable(VkCommandBuffer cb, VkBool32 e);
void vkc_CmdBindPipeline(VkCommandBuffer cb, VkPipelineBindPoint bp, VkPipeline p);
void vkc_CmdDraw(VkCommandBuffer cb, uint32_t vc, uint32_t ic, uint32_t fv, uint32_t fi);
void vkc_CmdDrawIndexed(VkCommandBuffer cb, uint32_t ic, uint32_t inst, uint32_t fi, int32_t vo, uint32_t first_inst);
void vkc_DestroyPipeline(VkDevice dev, VkPipeline p, const VkAllocationCallbacks *ac);
uint32_t vkc_variant_count(void);  // (statistics / tests) real pipelines built so far for virtual ones

// (image views are tracked: a render pass needs each attachment's format, and a framebuffer dies with its views)
VkResult vkc_CreateImageView(VkDevice dev, const VkImageViewCreateInfo *ci, const VkAllocationCallbacks *ac, VkImageView *out);
void vkc_DestroyImageView(VkDevice dev, VkImageView v, const VkAllocationCallbacks *ac);

// timeline semaphore waits: core in 1.2, the KHR entry points on a 1.1 device
void vkc_use_khr_semaphores(VkDevice dev);
VkResult vkc_WaitSemaphores(VkDevice dev, const VkSemaphoreWaitInfo *wi, uint64_t timeout);
VkResult vkc_GetSemaphoreCounterValue(VkDevice dev, VkSemaphore s, uint64_t *value);

// (exposed for the unit test)
VkPipelineStageFlags vkc_stage2_to_1(VkPipelineStageFlags2 s, bool is_src);
VkAccessFlags vkc_access2_to_1(VkAccessFlags2 a);

#ifdef __cplusplus
}
#endif

#ifndef VKC_NO_REDIRECT
#define vkCmdPipelineBarrier2 vkc_CmdPipelineBarrier2
#define vkQueueSubmit2 vkc_QueueSubmit2
#define vkCmdWriteTimestamp2 vkc_CmdWriteTimestamp2
#define vkCmdBindVertexBuffers2 vkc_CmdBindVertexBuffers2
#define vkCmdBeginRendering vkc_CmdBeginRendering
#define vkCmdEndRendering vkc_CmdEndRendering
#define vkCreateGraphicsPipelines vkc_CreateGraphicsPipelines
#define vkCreateImageView vkc_CreateImageView
#define vkDestroyImageView vkc_DestroyImageView
#define vkCmdSetCullMode vkc_CmdSetCullMode
#define vkCmdSetFrontFace vkc_CmdSetFrontFace
#define vkCmdSetPrimitiveTopology vkc_CmdSetPrimitiveTopology
#define vkCmdSetDepthTestEnable vkc_CmdSetDepthTestEnable
#define vkCmdSetDepthWriteEnable vkc_CmdSetDepthWriteEnable
#define vkCmdSetDepthCompareOp vkc_CmdSetDepthCompareOp
#define vkCmdSetStencilTestEnable vkc_CmdSetStencilTestEnable
#define vkCmdSetStencilOp vkc_CmdSetStencilOp
#define vkCmdSetPrimitiveRestartEnable vkc_CmdSetPrimitiveRestartEnable
#define vkCmdSetDepthBiasEnable vkc_CmdSetDepthBiasEnable
#define vkCmdBindPipeline vkc_CmdBindPipeline
#define vkCmdDraw vkc_CmdDraw
#define vkCmdDrawIndexed vkc_CmdDrawIndexed
#define vkDestroyPipeline vkc_DestroyPipeline
#define vkWaitSemaphores vkc_WaitSemaphores
#define vkGetSemaphoreCounterValue vkc_GetSemaphoreCounterValue
#endif
