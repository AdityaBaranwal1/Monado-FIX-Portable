// [XCDE] T247/T248/T249/T253: executable behavior checks, no GPU or game data.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../runtime/gpu_policy.h"
#include "../../runtime/frame_clock.h"
#include "../../runtime/cache_validation.h"
#include "../../runtime/host/ini_writer.h"
#include "../../runtime/resource_policy.h"
#include "../../runtime/staging_copy.h"
#include "../../runtime/window_reuse.h"
static XcdeWindowSlot test_slot;
static int reuse_order;
static void wait_present(uint64_t ticket) { assert(ticket == 7 && reuse_order == 0); test_slot.seq = 42; reuse_order = 1; }
static void wait_gpu(uint64_t seq) { assert(seq == 42 && reuse_order == 1); reuse_order = 2; }

static void gpu_tests(void) {
    XcdeGpuInfo g[3] = {0}; bool fallback;
    for (int i = 0; i < 3; ++i) { g[i].usable = true; g[i].index = (uint32_t)i; }
    strcpy(g[0].uuid, "00000000000000000000000000000000");
    strcpy(g[1].uuid, "11111111111111111111111111111111");
    g[0].type = 1; g[1].type = 2; g[2].type = 2; g[2].usable = false;
    assert(xcde_gpu_choose(g, 3, NULL, "", -1, &fallback) == 1 && !fallback);
    assert(xcde_gpu_choose(g, 3, NULL, g[0].uuid, 1, &fallback) == 0 && !fallback);
    assert(xcde_gpu_choose(g, 3, "1", g[0].uuid, 0, &fallback) == 1 && !fallback);
    assert(xcde_gpu_choose(g, 3, NULL, "missing", 0, &fallback) == 1 && fallback);
    assert(xcde_gpu_choose(g, 3, "bad", "", 0, &fallback) == 1 && fallback);
    assert(xcde_gpu_choose(g, 3, "2", "", -1, &fallback) == 1 && fallback);
    assert(xcde_gpu_choose(g, 3, NULL, "", 0, &fallback) == 0 && !fallback);
    assert(xcde_gpu_choose(g, 3, "-1", g[0].uuid, 0, &fallback) == 1 && !fallback);
    XcdeGpuInfo reordered[] = {g[1], g[0]};
    reordered[0].index = 0; reordered[1].index = 1;
    assert(xcde_gpu_choose(reordered, 2, NULL, g[0].uuid, -1, &fallback) == 1);
    g[1].usable = g[0].usable = false;
    assert(xcde_gpu_choose(g, 3, NULL, "", -1, &fallback) == -1);
    assert(xcde_gpu_uuid_valid(g[0].uuid)); assert(!xcde_gpu_uuid_valid("../cache"));
}
static void timing_tests(void) {
    const int rates[] = {30, 45, 60, 120, 144, 165, 240};
    for (unsigned j = 0; j < sizeof rates / sizeof rates[0]; ++j) {
        int fps = rates[j]; XcdeFrameClock c = {0};
        for (int i = 0; i <= fps * 60; ++i) xcde_frame_clock_step(&c, (uint64_t)i * 1000000000ull / (uint64_t)fps);
        uint32_t threshold = xcde_frame_threshold(&c, 29); // one second in the original game
        assert(threshold + 1 >= (unsigned)fps && threshold + 1 <= (unsigned)fps + 1);
        assert(fabs((double)(threshold + 1) / fps - 1.0) <= 0.01);
        if (fps == 144) assert(xcde_frame_threshold(&c, 9) == 47);
        xcde_frame_clock_step(&c, c.last_ns + 2000000000ull);
        assert(c.period_ns == 0); // no loading stall carried into the cadence
        xcde_frame_clock_step(&c, c.last_ns + 1000000000ull / fps);
        assert(c.period_ns > 0);
    }
    XcdeFrameClock c = {0}; assert(xcde_frame_threshold(&c, 9) == 9);
}
static void cache_tests(void) {
    uint32_t spv[5] = {0x07230203, 0x10500, 0, 20, 0};
    assert(xcde_spirv_valid(spv, sizeof spv, 0x10500));
    assert(!xcde_spirv_valid(spv, 19, 0x10500));
    assert(!xcde_spirv_size_valid(UINT64_MAX));
    assert(!xcde_spirv_size_valid(XCDE_SPIRV_MAX_BYTES + 4));
    spv[1] = 0x10600; assert(!xcde_spirv_valid(spv, sizeof spv, 0x10500));
    uint32_t pc[8] = {32, 1, 0x8086, 42, 0, 0, 0, 0}; uint8_t uuid[16] = {0};
    assert(xcde_pipeline_cache_valid(pc, sizeof pc, 0x8086, 42, uuid));
    assert(!xcde_pipeline_cache_valid(pc, 31, 0x8086, 42, uuid));
    assert(!xcde_pipeline_cache_valid(pc, sizeof pc, 0x10de, 42, uuid));
    uuid[0] = 1; assert(!xcde_pipeline_cache_valid(pc, sizeof pc, 0x8086, 42, uuid));
}
static void ini_tests(const char *path) {
    FILE *f = fopen(path, "w"); assert(f);
    fputs("known = old\nfuture = retain me\ngpu_uuid = abc\n", f); assert(!fclose(f));
    char tmp[1200], contents[4096] = {0};
    f = xcde_ini_begin(path, tmp, sizeof tmp); assert(f);
    fputs("known = new\ngpu_uuid = selected\n", f); assert(xcde_ini_commit(f, tmp, path));
    f = fopen(path, "r"); assert(f); assert(fread(contents, 1, sizeof contents - 1, f)); fclose(f);
    assert(strstr(contents, "known = new")); assert(!strstr(contents, "known = old"));
    assert(strstr(contents, "future = retain me")); assert(strstr(contents, "gpu_uuid = selected"));
    assert(!strstr(contents, "gpu_uuid = abc"));
    // Failed replacement must not remove the previous file.
    assert(!xcde_atomic_replace("nonexistent-test-source.tmp", path));
    f = fopen(path, "r"); assert(f); fclose(f);
    assert(!remove(path));
}
int main(int argc, char **argv) {
    assert(argc == 2); gpu_tests(); timing_tests(); cache_tests(); ini_tests(argv[1]);
    assert(xcde_shader_workers(1, 8000, 1, 0) == 1);
    assert(xcde_shader_workers(4, 8000, 1, 0) == 2);
    assert(xcde_shader_workers(32, 8000, 0, 0) == 5);
    assert(xcde_shader_workers(32, 32000, 0, 1) == 16);  // [PERF-V5] the preset no longer caps the workers
    assert(xcde_shader_workers(12, 16000, 1, 1) == 8);   // Ryzen 5 5650U (12 threads) + Vega 7, as V4
    assert(xcde_shader_workers(12, 16000, 1, 0) == 8);
    uint32_t before[] = {1, 2, 3}, after[] = {1, 9, 3}, guest[] = {7, 2, 3};
    xcde_merge_gpu_words(guest, before, after, 3);
    assert(guest[0] == 7 && guest[1] == 9 && guest[2] == 3);
    test_slot.ticket = 7;
    xcde_window_reuse_wait(&test_slot, wait_present, wait_gpu);
    assert(reuse_order == 2 && !test_slot.ticket && !test_slot.seq);
    assert(xcde_window_active(20, 3) == 3 && xcde_window_active(0, 3) == 3);
    puts("port core: adapter policy, fractional timing, caches, atomic INI OK"); return 0;
}
