// [XCDE] T261: shared native UI theme; no textures, web runtime or decorative animation.
#pragma once
#include "imgui.h"
static inline ImVec4 monado_rgb(unsigned rgb) {
    return ImVec4(((rgb>>16)&255)/255.f,((rgb>>8)&255)/255.f,(rgb&255)/255.f,1);
}
static inline void monado_theme(float scale) {
    ImGui::StyleColorsDark();
    ImGuiStyle &s=ImGui::GetStyle();
    s.WindowRounding=12; s.ChildRounding=12; s.FrameRounding=8;
    s.PopupRounding=8; s.ScrollbarRounding=8; s.GrabRounding=6; s.TabRounding=7;
    s.WindowPadding=ImVec2(18,16); s.FramePadding=ImVec2(10,7);
    s.ItemSpacing=ImVec2(12,10); s.ItemInnerSpacing=ImVec2(8,6);
    s.WindowBorderSize=1; s.ChildBorderSize=1; s.FrameBorderSize=1;
    auto &c=s.Colors;
    c[ImGuiCol_Text]=monado_rgb(0xEDF2EE); c[ImGuiCol_TextDisabled]=monado_rgb(0xB3BFB6);
    c[ImGuiCol_WindowBg]=monado_rgb(0x101411); c[ImGuiCol_ChildBg]=monado_rgb(0x19211C);
    c[ImGuiCol_PopupBg]=monado_rgb(0x19211C); c[ImGuiCol_Border]=monado_rgb(0x304438);
    c[ImGuiCol_TitleBg]=monado_rgb(0x101411); c[ImGuiCol_TitleBgActive]=monado_rgb(0x203D2A);
    c[ImGuiCol_TitleBgCollapsed]=monado_rgb(0x19211C); c[ImGuiCol_MenuBarBg]=monado_rgb(0x19211C);
    c[ImGuiCol_FrameBg]=monado_rgb(0x202C24); c[ImGuiCol_FrameBgHovered]=monado_rgb(0x2B4033);
    c[ImGuiCol_FrameBgActive]=monado_rgb(0x35533F);
    c[ImGuiCol_Button]=monado_rgb(0x294A35); c[ImGuiCol_ButtonHovered]=monado_rgb(0x386347);
    c[ImGuiCol_ButtonActive]=monado_rgb(0x437A55);
    c[ImGuiCol_CheckMark]=c[ImGuiCol_SliderGrab]=monado_rgb(0x59C985);
    c[ImGuiCol_SliderGrabActive]=monado_rgb(0x86DDA8);
    c[ImGuiCol_Header]=monado_rgb(0x294A35); c[ImGuiCol_HeaderHovered]=monado_rgb(0x386347);
    c[ImGuiCol_HeaderActive]=monado_rgb(0x437A55);
    c[ImGuiCol_Tab]=monado_rgb(0x19211C); c[ImGuiCol_TabHovered]=monado_rgb(0x386347);
    c[ImGuiCol_TabSelected]=monado_rgb(0x294A35); c[ImGuiCol_Separator]=monado_rgb(0x304438);
    c[ImGuiCol_TabSelectedOverline]=monado_rgb(0x59C985);
    c[ImGuiCol_TextSelectedBg]=ImVec4(0.18f,0.35f,0.24f,0.85f);
    c[ImGuiCol_ResizeGrip]=monado_rgb(0x294A35); c[ImGuiCol_ResizeGripHovered]=monado_rgb(0x386347);
    c[ImGuiCol_ResizeGripActive]=monado_rgb(0x59C985);
    c[ImGuiCol_NavCursor]=monado_rgb(0x86DDA8); c[ImGuiCol_PlotHistogram]=monado_rgb(0x59C985);
    s.ScaleAllSizes(scale>0 ? scale : 1);
}
