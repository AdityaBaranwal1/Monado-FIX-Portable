// Vulkan Memory Allocator implementation (header-only library, MSYS2 package vulkan-memory-allocator).
#define VMA_IMPLEMENTATION
// [XCDE-VK11] dynamic: VMA fetches 1.3 functions (vkGetDeviceBufferMemoryRequirements, ...) only when vulkanApiVersion >= 1.3,
// instead of importing them from vulkan-1.dll (which fails to load on a Vulkan 1.1 driver).
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#if __has_include(<vma/vk_mem_alloc.h>)
#include <vma/vk_mem_alloc.h>
#else
#include <vk_mem_alloc.h>  // (Debian / Ubuntu libvulkan-memory-allocator-dev)
#endif
