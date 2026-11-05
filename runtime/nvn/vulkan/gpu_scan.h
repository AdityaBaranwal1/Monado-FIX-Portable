// [XCDE] T248: standalone surface probe, no game files or renderer globals.
#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "gpu_probe.h"
static inline XcdeGpu *xcde_gpu_scan(uint32_t *count, char *error, size_t cap) {
    *count = 0; error[0] = 0;
    SDL_Window *w = SDL_CreateWindow("Monado GPU probe", 64, 64, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (!w) { snprintf(error, cap, "%s", SDL_GetError()); return NULL; }
    uint32_t api = xcde_vk_loader_version();
    VkInstance inst = VK_NULL_HANDLE; VkSurfaceKHR surface = VK_NULL_HANDLE;
    XcdeGpu *result = NULL;
    Uint32 ne = 0; const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&ne);
    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Monado GPU probe",
                             .apiVersion = api >= VK_API_VERSION_1_3 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_2 };
    VkInstanceCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app,
                               .enabledExtensionCount = ne, .ppEnabledExtensionNames = extensions };
    if (api < VK_API_VERSION_1_2) snprintf(error, cap, "Vulkan 1.2 loader required");
    else if (vkCreateInstance(&ci, NULL, &inst) != VK_SUCCESS) snprintf(error, cap, "Cannot create Vulkan probe instance");
    else if (!SDL_Vulkan_CreateSurface(w, inst, NULL, &surface)) snprintf(error, cap, "%s", SDL_GetError());
    else { result = xcde_gpu_enumerate(inst, surface, count); if (!result) snprintf(error, cap, "No Vulkan adapters found"); }
    if (surface) vkDestroySurfaceKHR(inst, surface, NULL);
    if (inst) vkDestroyInstance(inst, NULL);
    SDL_DestroyWindow(w);
    // Only info remains usable; physical-device handles belonged to the probe instance.
    for (uint32_t i = 0; i < *count; ++i) result[i].phys = VK_NULL_HANDLE;
    return result;
}
