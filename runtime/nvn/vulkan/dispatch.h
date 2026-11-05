// [XCDE] T250: use negotiated core 1.3 or KHR 1.2 entry points throughout the backend.
#pragma once
#include <stdbool.h>
#include <vulkan/vulkan.h>
typedef struct {
    PFN_vkCmdBeginRendering begin_rendering;
    PFN_vkCmdEndRendering end_rendering;
    PFN_vkCmdPipelineBarrier2 barrier2;
    PFN_vkQueueSubmit2 submit2;
    PFN_vkCmdWriteTimestamp2 timestamp2;
} XcdeVkDispatch;
static inline bool xcde_vk_dispatch_load(XcdeVkDispatch *d, VkDevice dev, uint32_t api, PFN_vkGetDeviceProcAddr get) {
    bool core = api >= VK_API_VERSION_1_3;
    d->begin_rendering = (PFN_vkCmdBeginRendering)get(dev, core ? "vkCmdBeginRendering" : "vkCmdBeginRenderingKHR");
    d->end_rendering = (PFN_vkCmdEndRendering)get(dev, core ? "vkCmdEndRendering" : "vkCmdEndRenderingKHR");
    d->barrier2 = (PFN_vkCmdPipelineBarrier2)get(dev, core ? "vkCmdPipelineBarrier2" : "vkCmdPipelineBarrier2KHR");
    d->submit2 = (PFN_vkQueueSubmit2)get(dev, core ? "vkQueueSubmit2" : "vkQueueSubmit2KHR");
    d->timestamp2 = (PFN_vkCmdWriteTimestamp2)get(dev, core ? "vkCmdWriteTimestamp2" : "vkCmdWriteTimestamp2KHR");
    return d->begin_rendering && d->end_rendering && d->barrier2 && d->submit2 && d->timestamp2;
}
