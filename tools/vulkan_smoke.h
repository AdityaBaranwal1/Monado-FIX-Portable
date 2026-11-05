// [XCDE] T253: procedural offscreen clear/readback, exercises real core/KHR dispatch.
// No shaders, game data, screenshot or saved settings. This is not a gameplay test.
#pragma once
#include "../runtime/nvn/vulkan/dispatch.h"
static inline uint32_t smoke_memory(VkPhysicalDevice pd, uint32_t bits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p; vkGetPhysicalDeviceMemoryProperties(pd, &p);
    for (uint32_t i = 0; i < p.memoryTypeCount; ++i)
        if ((bits & (1u << i)) && (p.memoryTypes[i].propertyFlags & flags) == flags) return i;
    return UINT32_MAX;
}
static int xcde_vulkan_smoke(bool legacy) {
    int result = 1; uint32_t n = 0; XcdeGpu *list = NULL;
    VkInstance instance = VK_NULL_HANDLE; VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE; VkImageView view = VK_NULL_HANDLE; VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory image_mem = VK_NULL_HANDLE, buffer_mem = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE; VkSemaphore timeline = VK_NULL_HANDLE; void *mapped = NULL;
    uint32_t api = legacy ? VK_API_VERSION_1_2 : VK_API_VERSION_1_3;
    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Monado procedural smoke", .apiVersion = api};
    VkInstanceCreateInfo ic = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
#define SMOKE_CHECK(call) do { VkResult r_ = (call); if (r_ != VK_SUCCESS) { fprintf(stderr, "smoke: %s = %d\n", #call, r_); goto cleanup; } } while (0)
    SMOKE_CHECK(vkCreateInstance(&ic, NULL, &instance));
    list = xcde_gpu_enumerate(instance, VK_NULL_HANDLE, &n);
    if (!list) goto cleanup;
    int best = -1;
    for (uint32_t i = 0; i < n; ++i) if (list[i].info.usable &&
        (best < 0 || list[i].info.type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) best = (int)i;
    if (best < 0) goto cleanup;
    VkPhysicalDevice pd = list[best].phys; uint32_t qfam = list[best].qfam;
    VkPhysicalDeviceSynchronization2Features sync = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, .synchronization2 = VK_TRUE};
    VkPhysicalDeviceDynamicRenderingFeatures render = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES, .pNext = &sync, .dynamicRendering = VK_TRUE};
    VkPhysicalDeviceVulkan12Features features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &render, .timelineSemaphore = VK_TRUE};
    float priority = 1;
    VkDeviceQueueCreateInfo qc = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = qfam, .queueCount = 1, .pQueuePriorities = &priority};
    const char *extensions[] = {VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};
    VkDeviceCreateInfo dc = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = &features, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qc,
                             .enabledExtensionCount = legacy ? 2 : 0, .ppEnabledExtensionNames = extensions};
    SMOKE_CHECK(vkCreateDevice(pd, &dc, NULL, &device));
    XcdeVkDispatch dispatch;
    if (!xcde_vk_dispatch_load(&dispatch, device, api, vkGetDeviceProcAddr)) goto cleanup;
    VkQueue queue; vkGetDeviceQueue(device, qfam, 0, &queue);
    VkImageCreateInfo im = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {16, 16, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT};
    SMOKE_CHECK(vkCreateImage(device, &im, NULL, &image));
    VkMemoryRequirements ir; vkGetImageMemoryRequirements(device, image, &ir);
    uint32_t mt = smoke_memory(pd, ir.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mt == UINT32_MAX) goto cleanup;
    VkMemoryAllocateInfo ia = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = ir.size, .memoryTypeIndex = mt};
    SMOKE_CHECK(vkAllocateMemory(device, &ia, NULL, &image_mem));
    SMOKE_CHECK(vkBindImageMemory(device, image, image_mem, 0));
    VkImageViewCreateInfo iv = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    SMOKE_CHECK(vkCreateImageView(device, &iv, NULL, &view));
    VkBufferCreateInfo bc = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 16 * 16 * 4, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    SMOKE_CHECK(vkCreateBuffer(device, &bc, NULL, &buffer));
    VkMemoryRequirements br; vkGetBufferMemoryRequirements(device, buffer, &br);
    mt = smoke_memory(pd, br.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (mt == UINT32_MAX) goto cleanup;
    VkMemoryAllocateInfo ba = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = br.size, .memoryTypeIndex = mt};
    SMOKE_CHECK(vkAllocateMemory(device, &ba, NULL, &buffer_mem));
    SMOKE_CHECK(vkBindBufferMemory(device, buffer, buffer_mem, 0));
    SMOKE_CHECK(vkMapMemory(device, buffer_mem, 0, VK_WHOLE_SIZE, 0, &mapped));
    VkCommandPoolCreateInfo pc = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qfam};
    SMOKE_CHECK(vkCreateCommandPool(device, &pc, NULL, &pool));
    VkCommandBufferAllocateInfo ca = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
    VkCommandBuffer cb; SMOKE_CHECK(vkAllocateCommandBuffers(device, &ca, &cb));
    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    SMOKE_CHECK(vkBeginCommandBuffer(cb, &begin));
    VkImageMemoryBarrier2 ib = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkDependencyInfo dep = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &ib};
    dispatch.barrier2(cb, &dep);
    VkRenderingAttachmentInfo attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color.float32 = {0.25f, 0.5f, 0.75f, 1.0f}};
    VkRenderingInfo ri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .renderArea = {{0, 0}, {16, 16}}, .layerCount = 1,
        .colorAttachmentCount = 1, .pColorAttachments = &attachment};
    dispatch.begin_rendering(cb, &ri); dispatch.end_rendering(cb);
    ib.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; ib.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    ib.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT; ib.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT; ib.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    dispatch.barrier2(cb, &dep);
    VkBufferImageCopy copy = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .imageExtent = {16, 16, 1}};
    vkCmdCopyImageToBuffer(cb, image, VK_IMAGE_LAYOUT_GENERAL, buffer, 1, &copy);
    VkMemoryBarrier2 host = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT, .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT, .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT};
    VkDependencyInfo hd = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .memoryBarrierCount = 1, .pMemoryBarriers = &host};
    dispatch.barrier2(cb, &hd); SMOKE_CHECK(vkEndCommandBuffer(cb));
    VkSemaphoreTypeCreateInfo type = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE};
    VkSemaphoreCreateInfo sc = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &type};
    SMOKE_CHECK(vkCreateSemaphore(device, &sc, NULL, &timeline));
    VkCommandBufferSubmitInfo cs = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cb};
    VkSemaphoreSubmitInfo signal = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = timeline,
        .value = 1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
    VkSubmitInfo2 submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cs,
        .signalSemaphoreInfoCount = 1, .pSignalSemaphoreInfos = &signal};
    SMOKE_CHECK(dispatch.submit2(queue, 1, &submit, VK_NULL_HANDLE));
    uint64_t value = 1;
    VkSemaphoreWaitInfo wait = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, .semaphoreCount = 1, .pSemaphores = &timeline, .pValues = &value};
    SMOKE_CHECK(vkWaitSemaphores(device, &wait, 10000000000ull));
    VkMappedMemoryRange range = {.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .memory = buffer_mem, .size = VK_WHOLE_SIZE};
    SMOKE_CHECK(vkInvalidateMappedMemoryRanges(device, 1, &range));
    const unsigned char *pixels = mapped;
    for (unsigned i = 0; i < 16 * 16; ++i)
        if (abs((int)pixels[i * 4] - 64) > 1 || abs((int)pixels[i * 4 + 1] - 128) > 1 ||
            abs((int)pixels[i * 4 + 2] - 191) > 1 || pixels[i * 4 + 3] != 255) {
            fprintf(stderr, "smoke: pixel mismatch at %u\n", i); goto cleanup;
        }
    printf("smoke PASS: %s, Vulkan %s, dynamic rendering + sync2 + timeline + 256 verified pixels\n",
           list[best].info.name, legacy ? "1.2 KHR" : "1.3 core"); result = 0;
cleanup:
    if (device) vkDeviceWaitIdle(device);
    if (mapped) vkUnmapMemory(device, buffer_mem);
    if (timeline) vkDestroySemaphore(device, timeline, NULL);
    if (pool) vkDestroyCommandPool(device, pool, NULL);
    if (view) vkDestroyImageView(device, view, NULL);
    if (image) vkDestroyImage(device, image, NULL);
    if (buffer) vkDestroyBuffer(device, buffer, NULL);
    if (image_mem) vkFreeMemory(device, image_mem, NULL);
    if (buffer_mem) vkFreeMemory(device, buffer_mem, NULL);
    if (device) vkDestroyDevice(device, NULL);
    free(list); if (instance) vkDestroyInstance(instance, NULL);
    return result;
#undef SMOKE_CHECK
}
