// [XCDE] T248/T250: independently linked adapter diagnostic; no game data.
#include <SDL3/SDL_main.h>
#include "../runtime/nvn/vulkan/gpu_scan.h"
#include "vulkan_smoke.h"
int main(int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    if (argc > 1 && (!strcmp(argv[1], "--smoke") || !strcmp(argv[1], "--smoke-vulkan12"))) {
        int result = xcde_vulkan_smoke(!strcmp(argv[1], "--smoke-vulkan12")); SDL_Quit(); return result;
    }
    uint32_t n; char error[512];
    XcdeGpu *list = xcde_gpu_scan(&n, error, sizeof error);
    if (!list) fprintf(stderr, "%s\n", error);
    int usable = 0;
    for (uint32_t i = 0; i < n; ++i) {
        const XcdeGpuInfo *g = &list[i].info;
        printf("GPU %u: %s\n  uuid=%s vendor=%04x type=%u api=%u.%u.%u\n  usable=%d: %s\n", g->index, g->name,
               g->uuid, g->vendor, g->type, VK_API_VERSION_MAJOR(g->api), VK_API_VERSION_MINOR(g->api),
               VK_API_VERSION_PATCH(g->api), g->usable, g->reason);
        usable += g->usable;
        printf("  driver=%u maxStorageBufferRange=%u MiB; gameplay unverified\n", g->driver, g->max_storage_range >> 20);
    }
    free(list); SDL_Quit(); return usable ? 0 : 1;
}
