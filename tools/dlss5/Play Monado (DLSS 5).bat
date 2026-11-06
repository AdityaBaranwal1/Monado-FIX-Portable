@echo off
rem Monado with DLSS 5 neural rendering (experimental): ReShade's Vulkan layer for this game only (build\reshade_layer),
rem dlss5-bridge + renodx-dlss5 add-ons and nvngx_dlssnr.dll in build\ (settings: build\ReShade.ini [RenoDX.DLSS5], F6 toggles,
rem Home opens ReShade). The bridge mirrors the game's Vulkan DLSS onto D3D12, where neural rendering runs even on driver
rem 610.88. Play Monado.bat starts the game without any of it. Setup: docs/DLSS5.md (copy this file to the portable folder's root).
cd /d "%~dp0"

rem ======================= toggles =======================
rem DLSS 5 neural rendering at start: 1 = on, 0 = off (F6 switches it in game either way), none = start without ReShade
set DLSS5=1
rem DLSS 5 strength (RenoDX NRIntensity, 0 to 1; 1 = the full effect, lower is subtler)
set DLSS5_STRENGTH=1
rem Output: 2560x1440 = 1440p exclusive fullscreen, the TV scales it up (DLSS 5 at 60+ fps on an RTX 4070 Ti);
rem desktop = borderless at the desktop's size (at 4K, DLSS 5 runs ~30 fps and fills 12 GB of VRAM)
set OUTPUT=2560x1440
rem DLSS render resolution: native (DLAA) / quality / balanced / performance / ultra
set DLSS_MODE=quality
rem =======================================================

set XCDE_FG_NO_HUDLESS=1
if /i not "%OUTPUT%"=="desktop" set "XCDE_FULLSCREEN_MODE=%OUTPUT%"
if /i not "%DLSS5%"=="none" set "VK_ADD_IMPLICIT_LAYER_PATH=%~dp0build\reshade_layer"
powershell -NoProfile -ExecutionPolicy Bypass -File "build\bat_settings.ps1" -Upscaler dlss -DlssMode "%DLSS_MODE%" -Dlss5 "%DLSS5%" -Dlss5Strength "%DLSS5_STRENGTH%"
start "" "build\xcde_launcher.exe"
