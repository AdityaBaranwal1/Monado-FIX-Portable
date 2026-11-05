// [XCDE] T248: same named adapter selector in launcher and in-game menu.
#pragma once
#include "imgui.h"
#include "../nvn/vulkan/gpu_probe.h"
static inline const XcdeGpuInfo *xcde_gpu_preference(const XcdeGpu *list, uint32_t n, const char *uuid, int legacy) {
    const XcdeGpuInfo *best = nullptr;
    for (uint32_t i = 0; i < n; ++i) {
        const XcdeGpuInfo *g = &list[i].info;
        if (!g->usable) continue;
        if ((*uuid && !strcmp(uuid, g->uuid)) || (!*uuid && legacy >= 0 && g->index == (uint32_t)legacy)) return g;
        if (!best || (best->type != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && g->type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) best = g;
    }
    return best;
}
static inline void xcde_vulkan_picker(const XcdeGpuInfo *gpu, uint32_t loader, int *requested, bool english) {
    const char *label = english ? "Vulkan route (restart)" : "Ruta Vulkan (reiniciar)";
    const char *preview = *requested == 12 ? "Vulkan 1.2 (KHR)" : *requested == 13 ? "Vulkan 1.3" : *requested == 14 ? "Vulkan 1.4" : english ? "Automatic" : "Automático";
    if (!ImGui::BeginCombo(label, preview)) return;
    if (ImGui::Selectable(english ? "Automatic (recommended)" : "Automático (recomendado)", !*requested)) *requested = 0;
    for (int route = 12; route <= 14; ++route) {
        bool available = gpu && gpu->usable && (gpu->vulkan_routes & xcde_vulkan_flag(route)) &&
                         loader >= VK_MAKE_API_VERSION(0, 1, route - 10, 0);
        ImGui::BeginDisabled(!available);
        char text[80]; snprintf(text, sizeof text, "Vulkan 1.%d%s", route - 10, route == 12 ? " (KHR)" : "");
        if (ImGui::Selectable(text, *requested == route)) *requested = route;
        ImGui::EndDisabled();
        if (!available && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", english ? "Unavailable on the selected GPU / driver." : "No disponible en la GPU / controlador seleccionado.");
    }
    ImGui::EndCombo();
}
static inline void xcde_gpu_picker(const char *label, char *uuid, size_t cap, int *legacy,
                                  const XcdeGpu *list, uint32_t n, bool english) {
    const char *preview = *uuid ? (english ? "Unavailable adapter" : "Adaptador no disponible") :
        *legacy >= 0 ? (english ? "Legacy GPU index" : "Índice GPU anterior") : (english ? "Automatic" : "Automático");
    for (uint32_t i = 0; i < n; ++i)
        if ((*uuid && !strcmp(uuid, list[i].info.uuid)) || (!*uuid && *legacy >= 0 && (uint32_t)*legacy == list[i].info.index)) preview = list[i].info.name;
    if (!ImGui::BeginCombo(label, preview)) return;
    if (ImGui::Selectable(english ? "Automatic" : "Automático", !*uuid && *legacy < 0)) { *uuid = 0; *legacy = -1; }
    for (uint32_t i = 0; i < n; ++i) {
        const XcdeGpuInfo *g = &list[i].info;
        ImGui::PushID((int)i);
        char text[340];
        const char *kind = g->type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? (english ? "integrated" : "integrada") :
            g->type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? (english ? "dedicated" : "dedicada") : "Vulkan";
        snprintf(text, sizeof text, "%s (%s)%s", g->name, kind, g->usable ? "" : " [incompatible]");
        ImGui::BeginDisabled(!g->usable);
        if (ImGui::Selectable(text, !strcmp(uuid, g->uuid))) { snprintf(uuid, cap, "%s", g->uuid); *legacy = -1; }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s\nUUID: %s", g->reason, g->uuid);
        ImGui::PopID();
    }
    ImGui::EndCombo();
}
