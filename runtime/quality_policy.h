// [XCDE] T259: one quality policy for the launcher and the runtime. Applying is explicit;
// loading an old INI must never silently overwrite the user's individual choices.
#pragma once
#include "settings.h"
#include <string.h>

enum { QUALITY_BALANCED, QUALITY_PERFORMANCE, QUALITY_POTATO, QUALITY_CUSTOM, QUALITY_COUNT };
enum { EFFECT_TSSAO = 1, EFFECT_SSR = 2, EFFECT_BLOOM = 4, EFFECT_GODRAY = 8, EFFECT_CLOUD_SHADOW = 16 };
typedef struct {
    int resolution, upscaler, aa, min_h, max_h, upscale_quality;
    bool automatic, dof_off, blur, frame_gen;
    unsigned effects;
} XcdeQuality;
static inline int xcde_quality_parse(const char *name) {
    return !strcmp(name, "performance") ? QUALITY_PERFORMANCE : !strcmp(name, "potato") ? QUALITY_POTATO :
           !strcmp(name, "custom") ? QUALITY_CUSTOM : QUALITY_BALANCED;
}
static inline const char *xcde_quality_name(int p) {
    static const char *names[] = {"balanced", "performance", "potato", "custom"};
    return names[p >= 0 && p < QUALITY_COUNT ? p : 0];
}
static inline XcdeQuality xcde_quality_values(int preset) {
    XcdeQuality q = {0};
    q.upscaler = UPSCALE_FSR1; q.blur = true; q.automatic = true;
    q.upscale_quality=UQ_QUALITY;
    if (preset == QUALITY_PERFORMANCE || preset == QUALITY_POTATO) {
        q.resolution = RES_DYNAMIC; q.automatic = false; q.blur = false; q.dof_off = true;
        q.min_h = preset == QUALITY_POTATO ? 360 : 540;
        q.max_h = preset == QUALITY_POTATO ? 540 : 720;
        // Cosmetic effects are explicitly opt-in until their routes have visual evidence.
        // The UI exposes them individually; presets do not silently skip unvalidated passes.
    }
    return q;
}
static inline XcdeQuality xcde_quality_current(const Settings *s) {
    XcdeQuality q = {0};
    q.resolution=s->res_mode; q.upscaler=s->upscaler; q.aa=s->antialias;
    q.min_h=s->dynamic_min_height; q.max_h=s->dynamic_max_height;
    q.upscale_quality=s->upscale_quality;
    q.automatic=s->upscaler_auto; q.dof_off=s->dof_off; q.blur=s->game_blur;
    q.frame_gen=s->frame_gen || s->cutscene_mode==2; q.effects=s->effects_off;
    return q;
}
static inline bool xcde_quality_equal(XcdeQuality a, XcdeQuality b) {
    return a.resolution==b.resolution && a.upscaler==b.upscaler && a.aa==b.aa && a.min_h==b.min_h &&
           a.max_h==b.max_h && a.automatic==b.automatic && a.dof_off==b.dof_off && a.blur==b.blur &&
           a.frame_gen==b.frame_gen && a.effects==b.effects && a.upscale_quality==b.upscale_quality;
}
static inline void xcde_quality_apply(Settings *s, int p) {
    if (p < 0 || p >= QUALITY_CUSTOM) return;
    XcdeQuality q=xcde_quality_values(p);
    s->quality_preset=p; s->gpu_performance=p!=QUALITY_BALANCED;
    s->res_mode=q.resolution; s->upscaler=q.upscaler; s->antialias=q.aa;
    s->dynamic_min_height=q.min_h; s->dynamic_max_height=q.max_h;
    s->upscale_quality=q.upscale_quality;
    s->upscaler_auto=q.automatic; s->dof_off=q.dof_off; s->game_blur=q.blur;
    s->frame_gen=q.frame_gen; s->effects_off=q.effects;
    if (s->cutscene_mode==2) s->cutscene_mode=1; // restoring any preset also clears cutscene-only generation
}
