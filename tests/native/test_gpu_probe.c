// [XCDE] T248/T250: exercise actual probe/dispatch with a fake Vulkan driver.
#include <assert.h>
#include "../../runtime/nvn/vulkan/gpu_probe.h"
#include "../../runtime/nvn/vulkan/dispatch.h"
static uint32_t api = VK_API_VERSION_1_2;
static int extensions = 1, features = 1, present_ok = 1, missing_proc;
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice dev, VkPhysicalDeviceProperties2 *p) {
    (void)dev; p->properties.apiVersion = api; p->properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    strcpy(p->properties.deviceName, "Fake Intel");
    VkPhysicalDeviceIDProperties *id = p->pNext; memset(id->deviceUUID, 0x42, VK_UUID_SIZE);
}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice d, const char *layer, uint32_t *n, VkExtensionProperties *p) {
    (void)d; (void)layer;
    if (!p) { *n = extensions ? 3 : 1; return VK_SUCCESS; }
    strcpy(p[0].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (extensions) { strcpy(p[1].extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME); strcpy(p[2].extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME); }
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(VkPhysicalDevice d, VkPhysicalDeviceFeatures2 *f) {
    (void)d; f->features.shaderInt64 = f->features.geometryShader = (VkBool32)features;
    VkPhysicalDeviceVulkan12Features *v = f->pNext;
    VkPhysicalDeviceDynamicRenderingFeatures *r = v->pNext;
    VkPhysicalDeviceSynchronization2Features *s = r->pNext;
    v->timelineSemaphore = r->dynamicRendering = s->synchronization2 = (VkBool32)features;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice d, uint32_t *n, VkQueueFamilyProperties *p) {
    (void)d; if (!p) { *n = 1; return; }
    memset(p, 0, sizeof *p); p->queueCount = 1; p->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice d, uint32_t q, VkSurfaceKHR s, VkBool32 *p) {
    (void)d; (void)q; (void)s; *p = (VkBool32)present_ok; return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL stub(void) {}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL get_proc(VkDevice d, const char *name) {
    (void)d;
    int khr = strstr(name, "KHR") != NULL;
    assert(khr == (api < VK_API_VERSION_1_3));
    return missing_proc ? NULL : stub;
}
int main(void) {
    XcdeGpu g; XcdeVkDispatch dispatch;
    VkSurfaceKHR surface = (VkSurfaceKHR)(uintptr_t)1;
    assert(xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g) && g.info.usable);
    api = VK_API_VERSION_1_1; assert(!xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g)); api = VK_API_VERSION_1_2;
    assert(!strcmp(g.info.uuid, "42424242424242424242424242424242"));
    assert(xcde_vk_dispatch_load(&dispatch, VK_NULL_HANDLE, api, get_proc));
    extensions = 0; assert(!xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g));
    api = VK_API_VERSION_1_3; assert(xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g));
    assert(xcde_vk_dispatch_load(&dispatch, VK_NULL_HANDLE, api, get_proc));
    features = 0; assert(!xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g)); features = 1;
    present_ok = 0; assert(!xcde_gpu_probe(VK_NULL_HANDLE, surface, 0, &g));
    missing_proc = 1; assert(!xcde_vk_dispatch_load(&dispatch, VK_NULL_HANDLE, api, get_proc));
    puts("GPU probe: Vulkan 1.2 KHR / 1.3 core, missing features, presentation, dispatch OK"); return 0;
}
