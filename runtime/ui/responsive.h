// [XCDE] T261: stack labels at narrow effective widths (including high DPI), preserve keyboard/gamepad IDs.
#pragma once
#include "imgui.h"
#include <stdio.h>
static inline bool monado_compact() { return ImGui::GetContentRegionAvail().x < 600.f*ImGui::GetFontSize()/17.f; }
static inline const char *monado_field_label(const char *label,char *id,size_t size) {
    if (!monado_compact()) return label;
    ImGui::TextWrapped("%s",label); ImGui::SetNextItemWidth(-1);
    snprintf(id,size,"##%s",label); return id;
}
static inline bool monado_combo(const char *label,int *value,const char *const *items,int count,int height=-1) {
    char id[512]; return ImGui::Combo(monado_field_label(label,id,sizeof id),value,items,count,height);
}
static inline bool monado_combo(const char *label,int *value,const char *items,int height=-1) {
    char id[512]; return ImGui::Combo(monado_field_label(label,id,sizeof id),value,items,height);
}
static inline bool monado_begin_combo(const char *label,const char *preview,ImGuiComboFlags flags=0) {
    char id[512]; return ImGui::BeginCombo(monado_field_label(label,id,sizeof id),preview,flags);
}
