// [XCDE] T248/T250: shared capability probe for runtime, launcher and standalone diagnostics.
// Header-only so the launcher does not link any game/runtime objects.
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include "../../gpu_policy.h"
#include "../../vulkan_policy.h"

typedef struct { XcdeGpuInfo info; VkPhysicalDevice phys; uint32_t qfam; } XcdeGpu;
static inline bool xcde_vk_ext(const VkExtensionProperties *p, uint32_t n, const char *name) {
    for (uint32_t i = 0; i < n; ++i) if (!strcmp(p[i].extensionName, name)) return true;
    return false;
}
static inline uint32_t xcde_vk_loader_version(void) {
    uint32_t v = VK_API_VERSION_1_0;
    PFN_vkEnumerateInstanceVersion f = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    if (f && f(&v) != VK_SUCCESS) return VK_API_VERSION_1_0;
    return v;
}
static inline bool xcde_gpu_probe(VkPhysicalDevice pd, VkSurfaceKHR surface, uint32_t index, XcdeGpu *out) {
    memset(out, 0, sizeof *out); out->phys = pd; out->info.index = index; out->qfam = UINT32_MAX;
    VkPhysicalDeviceIDProperties id = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
    VkPhysicalDeviceProperties2 p = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &id };
    vkGetPhysicalDeviceProperties2(pd, &p);
    snprintf(out->info.name, sizeof out->info.name, "%s", p.properties.deviceName);
    out->info.vendor = p.properties.vendorID; out->info.api = p.properties.apiVersion; out->info.type = p.properties.deviceType;
    out->info.driver = p.properties.driverVersion; out->info.max_storage_range = p.properties.limits.maxStorageBufferRange;
    for (unsigned i = 0; i < VK_UUID_SIZE; ++i) snprintf(out->info.uuid + i * 2, 3, "%02x", id.deviceUUID[i]);
#define GPU_REJECT(msg) do { snprintf(out->info.reason, sizeof out->info.reason, "%s", msg); return false; } while (0)
    if (p.properties.apiVersion < VK_API_VERSION_1_2) GPU_REJECT("Vulkan 1.2 or newer required");
    uint32_t n = 0;
    if (vkEnumerateDeviceExtensionProperties(pd, NULL, &n, NULL) != VK_SUCCESS) GPU_REJECT("Cannot enumerate device extensions");
    VkExtensionProperties *e = n ? (VkExtensionProperties *)calloc(n, sizeof *e) : NULL;
    if (!e) GPU_REJECT("Cannot allocate device extension list");
    VkResult r = vkEnumerateDeviceExtensionProperties(pd, NULL, &n, e);
    bool swap = r == VK_SUCCESS && xcde_vk_ext(e, n, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    bool legacy = p.properties.apiVersion < VK_API_VERSION_1_3;
    bool render = r == VK_SUCCESS && xcde_vk_ext(e, n, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    bool sync = r == VK_SUCCESS && xcde_vk_ext(e, n, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    free(e);
    if (!swap) GPU_REJECT("VK_KHR_swapchain unavailable");
    if (legacy && (!render || !sync)) GPU_REJECT("Vulkan 1.2 requires VK_KHR_dynamic_rendering and VK_KHR_synchronization2");
    VkPhysicalDeviceSynchronization2Features s = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
    VkPhysicalDeviceDynamicRenderingFeatures d = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES, .pNext = &s };
    VkPhysicalDeviceVulkan12Features v12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &d };
    VkPhysicalDeviceFeatures2 f = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &v12 };
    vkGetPhysicalDeviceFeatures2(pd, &f);
    if (!d.dynamicRendering || !s.synchronization2 || !v12.timelineSemaphore) GPU_REJECT("dynamicRendering, synchronization2 and timelineSemaphore required");
    if (render && sync) out->info.vulkan_routes |= XCDE_VK12;
    if (p.properties.apiVersion >= VK_API_VERSION_1_3) out->info.vulkan_routes |= XCDE_VK13;
    if (p.properties.apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0)) out->info.vulkan_routes |= XCDE_VK14;
    if (!f.features.shaderInt64) GPU_REJECT("shaderInt64 required by translated address arithmetic");
    if (!f.features.geometryShader) GPU_REJECT("geometryShader required by point sprites");
    uint32_t nq = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, NULL);
    VkQueueFamilyProperties *q = nq ? (VkQueueFamilyProperties *)calloc(nq, sizeof *q) : NULL;
    if (!q) GPU_REJECT("Cannot enumerate queues");
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, q);
    for (uint32_t i = 0; i < nq && i < 16; ++i) {
        VkBool32 present = surface ? VK_FALSE : VK_TRUE;
        if (surface && vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &present) != VK_SUCCESS) continue;
        if (present && q[i].queueCount && (q[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
            (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) { out->qfam = i; break; }
    }
    free(q);
    if (out->qfam == UINT32_MAX) GPU_REJECT("No graphics+compute+present queue");
    out->info.usable = true;
    snprintf(out->info.reason, sizeof out->info.reason, "%s", legacy ? "Vulkan 1.2 + KHR" : "Vulkan 1.3 core");
    return true;
#undef GPU_REJECT
}

static inline XcdeGpu *xcde_gpu_enumerate(VkInstance instance, VkSurfaceKHR surface, uint32_t *count) {
    *count = 0; uint32_t n = 0;
    if (vkEnumeratePhysicalDevices(instance, &n, NULL) != VK_SUCCESS || !n) return NULL;
    VkPhysicalDevice *pd = (VkPhysicalDevice *)calloc(n, sizeof *pd);
    XcdeGpu *result = (XcdeGpu *)calloc(n, sizeof *result);
    if (!pd || !result || vkEnumeratePhysicalDevices(instance, &n, pd) != VK_SUCCESS) { free(pd); free(result); return NULL; }
    for (uint32_t i = 0; i < n; ++i) xcde_gpu_probe(pd[i], surface, i, &result[i]);
    free(pd); *count = n; return result;
}
