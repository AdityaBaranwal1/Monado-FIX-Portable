// [XCDE] T259/T261: identical profile semantics in both frontends.
#pragma once
#include "imgui.h"
#include "responsive.h"
#include "../quality_policy.h"
static inline bool monado_quality_controls(Settings *s, bool en) {
    XcdeQuality before=xcde_quality_current(s); int old_fps=s->fps, old_preset=s->quality_preset;
    int p=s->quality_preset;
    if (monado_combo(en ? "Quality preset" : "Perfil de calidad", &p,
                     en ? "Balanced\0Performance\0Potato\0Custom\0" : "Equilibrado\0Rendimiento\0Patata\0Personalizado\0")) {
        if (p==QUALITY_CUSTOM) s->quality_preset=p; else xcde_quality_apply(s,p);
    }
    int target=s->fps==30 ? 0 : s->fps==60 ? 1 : !s->fps ? 2 : 3;
    if (monado_combo(en ? "Frame rate target" : "Objetivo de FPS", &target,
                     en ? "30 FPS - steady target\0 60 FPS\0Maximum - uncapped, may fluctuate\0Advanced\0" :
                          "30 FPS - objetivo estable\0 60 FPS\0Máximo - sin límite, puede fluctuar\0Avanzado\0")) {
        if (target<3) s->fps=target==0 ? 30 : target==1 ? 60 : 0;
        else if (s->fps==30 || s->fps==60 || !s->fps) s->fps=120;
    }
    if (target==3) {
        int rate=s->fps==144 ? 1 : s->fps==165 ? 2 : 0;
        if (monado_combo(en ? "Advanced FPS (restart)" : "FPS avanzados (reiniciar)",&rate,"120\0 144\0 165\0")) {
            const int rates[]={120,144,165}; s->fps=rates[rate];
        }
    }
    ImGui::TextWrapped("%s",en ? "Targets depend on hardware. Frame rate changes require restarting the game." :
                       "El objetivo depende del equipo. Cambiar los FPS requiere reiniciar el juego.");
    if (s->res_mode==RES_DYNAMIC && s->dynamic_max_height)
        ImGui::Text(en ? "Internal range: %d-%dp (capped by output)" : "Rango interno: %d-%dp (limitado por la salida)",s->dynamic_min_height,s->dynamic_max_height);
    if (s->res_mode==RES_DYNAMIC && ImGui::CollapsingHeader(en ? "Dynamic resolution limits" : "Límites de resolución dinamica")) {
        bool changed=ImGui::InputInt(en ? "Minimum height (0: auto)" : "Altura mínima (0: auto)",&s->dynamic_min_height,90);
        changed|=ImGui::InputInt(en ? "Maximum height (0: output)" : "Altura máxima (0: salida)",&s->dynamic_max_height,90);
        if (changed) {
            if (s->dynamic_min_height<0) s->dynamic_min_height=0;
            if (s->dynamic_max_height<0) s->dynamic_max_height=0;
            if (s->dynamic_min_height>4320) s->dynamic_min_height=4320;
            if (s->dynamic_max_height>4320) s->dynamic_max_height=4320;
            if (s->dynamic_max_height && s->dynamic_min_height>s->dynamic_max_height) s->dynamic_min_height=s->dynamic_max_height;
            s->quality_preset=QUALITY_CUSTOM;
        }
    }
    return old_fps!=s->fps || old_preset!=s->quality_preset || !xcde_quality_equal(before,xcde_quality_current(s));
}
static inline void monado_effect_controls(Settings *s, bool en) {
    if (!ImGui::CollapsingHeader(en ? "Experimental cosmetic reductions (restart)" : "Recortes decorativos experimentales (reiniciar)")) return;
    ImGui::TextWrapped("%s",en ? "Opt-in until visual tests pass. Checked means disabled. Original files are preserved." :
                       "Opcionales hasta completar pruebas visuales. Marcado significa desactivado. Se conservan los archivos originales.");
    const char *es[]={"Oclusión ambiental temporal", "Reflejos de pantalla", "Bloom", "Rayos de luz", "Sombras de nubes"};
    const char *eng[]={"Temporal ambient occlusion", "Screen-space reflections", "Bloom", "Light shafts", "Cloud shadows"};
    for (unsigned i=0;i<5;i++) {
        bool off=(s->effects_off&(1u<<i))!=0;
        if (ImGui::Checkbox(en ? eng[i] : es[i],&off)) {
            if (off) s->effects_off|=1u<<i; else s->effects_off&=~(1u<<i);
            s->quality_preset=QUALITY_CUSTOM;
        }
    }
}
