// [XCDE] T256/T257/T258: route, SDK queue and native-FPS behavior without GPU/game assets.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../runtime/vulkan_policy.h"
#include "../../runtime/fps_meter.h"
#include "../../runtime/dlss_policy.h"
int main(void) {
    bool fallback;
    assert(xcde_vulkan_route(XCDE_VK12 | XCDE_VK13 | XCDE_VK14, 14, 0, &fallback) == 14 && !fallback);
    assert(xcde_vulkan_route(XCDE_VK12 | XCDE_VK13, 14, 12, &fallback) == 12 && !fallback);
    assert(xcde_vulkan_route(XCDE_VK13, 14, 12, &fallback) == 13 && fallback);
    assert(xcde_vulkan_route(XCDE_VK12 | XCDE_VK13 | XCDE_VK14, 13, 14, &fallback) == 13 && fallback);
    assert(!xcde_vulkan_route(0, 14, 0, &fallback));
    uint32_t graphics, compute;
    xcde_dlss_queue_indices(0, 0, &graphics, &compute); assert(graphics == 0 && compute == 0);
    xcde_dlss_queue_indices(2, 3, &graphics, &compute); assert(graphics == 1 && compute == 3);
    xcde_dlss_queue_indices(0, 1, &graphics, &compute); assert(graphics == 0 && compute == 1);
    const int rates[] = {30, 60, 120, 144, 165};
    for (unsigned j = 0; j < sizeof rates / sizeof rates[0]; ++j) {
        XcdeFpsMeter meter = {0};
        for (int i = 0; i <= rates[j] * 2; ++i) xcde_fps_tick(&meter, (uint64_t)i * 1000000000ull / (unsigned)rates[j]);
        assert(fabs(meter.fps - rates[j]) < 0.001);
        assert(fabs(meter.ms - 1000.0 / rates[j]) < 0.001);
        double old = meter.fps; xcde_fps_tick(&meter, meter.last); assert(meter.fps == old);
        xcde_fps_tick(&meter, meter.last + 3000000000ull); assert(!meter.fps && !meter.intervals);
    }
    puts("display options: Vulkan routes, zero-extra SL queues and native FPS passed"); return 0;
}
