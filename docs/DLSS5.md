# DLSS 5 (neural rendering) on Monado — experimental

**Resumen (ES):** DLSS 5 ("neural rendering", función NGX 18) funciona en Monado v6 mediante la capa Vulkan de ReShade,
el add-on `dlss5-bridge` (refleja el DLSS Vulkan del juego en un dispositivo D3D12) y el add-on `renodx-dlss5`. Funciona
con el driver 610.88. A 4K satura los 12 GB de una RTX 4070 Ti; a 1440p (pantalla completa exclusiva, nueva variable
`XCDE_FULLSCREEN_MODE`) va a 65–80 fps. Este documento incluye además dos correcciones de DLSS: un cierre al arrancar y
otro al precompilar.

Findings from 2026-11-06 on one machine (below). Nothing here is from NVIDIA: DLSS 5 neural rendering is not a released
feature for this game, the runtime that backs it (`nvngx_dlssnr.dll`) is a community-distributed, **not NVIDIA-signed**
build, and the two ReShade add-ons are third-party betas. Use at your own risk; none of these binaries are in this
repository.

## How it works

```
xcde.exe (Vulkan) ── Streamline DLSS SR ── NVSDK_NGX_VULKAN_EvaluateFeature
                                               │ hooked by dlss5-bridge (vk_mirror=1)
                                               ▼
               colour / depth / motion vectors / output imported into a private D3D12 device
                                               │
                         renodx-dlss5 runs NGX feature 18 (nvngx_dlssnr.dll) there
                                               │
                         result copied back into the game's Vulkan DLSS output image
```

- ReShade 6.8 is loaded as a Vulkan layer **for this process only**, via `VK_ADD_IMPLICIT_LAYER_PATH` set in the bat.
  Nothing is installed system-wide and nothing needs admin rights.
- The game's own DLSS (Super Resolution or DLAA) keeps running. Neural rendering is applied on top of its output, at
  the **output** resolution.
- Creating feature 18 directly on the Vulkan device fails with `0xBAD00002`. That is harmless: the D3D12 route is the one
  that works.
- **Driver:** the tools warn that feature 18 needs driver 616.56 or newer (`0xBAD0000C OutOfDate`). On 610.88,
  neural rendering still started and ran ("NR runtime started on NVIDIA driver 610.88, below the 615.00 it
  declares").

## Setup

1. **DLSS must work first.** Choose DLSS in the launcher and check that the game reaches the title screen. Monado v6
   crashed right after `[dlss] Streamline loaded` on an RTX 4070 Ti, on every start and every precompile run; this PR
   fixes both crashes (see "Code changes").
2. Get the third-party files:

   | File | From | Tested version |
   |---|---|---|
   | `ReShade64.dll`, `ReShade64.json` | [reshade.me](https://reshade.me) "with full add-on support" setup (the setup `.exe` contains a zip with these two) | 6.8.0 |
   | `dlss5-bridge.addon64` | [NIGos/dlss5-bridge](https://github.com/NIGos/dlss5-bridge) releases | 1.4.13-pre8 |
   | `renodx-dlss5.addon64`, `nvngx_dlssnr.dll` | RenoDX DLSS 5 add-on (distribution linked from the dlss5-bridge README) | 10.8.0, runtime 310.9.16 |

3. Lay them out in the portable folder:

   ```
   Monado\
     Play Monado (DLSS 5).bat      <- tools/dlss5/Play Monado (DLSS 5).bat
     build\
       bat_settings.ps1            <- tools/dlss5/bat_settings.ps1
       ReShade.ini                 <- tools/dlss5/ReShade.ini  (required, see "Problems found")
       dlss5-bridge.addon64
       renodx-dlss5.addon64
       nvngx_dlssnr.dll
       reshade_layer\
         ReShade64.dll
         ReShade64.json
   ```

4. Run `Play Monado (DLSS 5).bat`, then press Play in the launcher. In game, **F6** turns neural rendering on and off and
   **Home** opens ReShade, which has the "DLSS 5 Bridge" and RenoDX panels. `Play Monado.bat` keeps starting the game
   without any of this.

### Toggles at the top of the bat

| Variable | Values | Effect |
|---|---|---|
| `DLSS5` | `1` / `0` / `none` | Neural rendering on or off at start (`NeuralUplift` in `ReShade.ini`; F6 still switches it). `none` starts without ReShade. |
| `DLSS5_STRENGTH` | `0`–`1` | RenoDX `NRIntensity`. |
| `OUTPUT` | `2560x1440` / `desktop` | Sets `XCDE_FULLSCREEN_MODE` (exclusive fullscreen at that display mode; the display scales it up), or borderless at the desktop's size. |
| `DLSS_MODE` | `native` / `quality` / `balanced` / `performance` / `ultra` | `upscale_quality` in `xcde.ini`. The bat also sets `upscaler = dlss` and `resolution = match`. |

`bat_settings.ps1` writes these before the launcher starts, so the launcher shows them. An invalid value is reported and
left unchanged.

## Problems found and how they were solved

| Symptom | Cause | Fix |
|---|---|---|
| Vulkan loader: `Failed to open dynamic library ...ReShade64.dll with error 1114`, `VK_LAYER_reshade failed to load` | ReShade 6.8's layer refuses to load (DllMain returns FALSE) unless a `ReShade.ini` exists in the executable's folder. `ReShadeApps.ini` is not used by 6.8. | `build\ReShade.ini` (seed in `tools/dlss5/ReShade.ini`). |
| 30 fps at 4K that drops to a steady 9 fps (100 ms frames) after a few minutes | At 3840x2160, DLAA plus the bridge's mirrored 4K textures plus a 255 MiB neural-rendering workset fill 11.2 of 12 GB of VRAM, and the GPU starts paging. With neural rendering off it still ran at 11 fps, so it is not the network's cost. | Run at 1440p with `XCDE_FULLSCREEN_MODE=2560x1440`. VRAM drops to 5.4–7 GB and neural rendering costs 5.3 ms instead of 16. |
| Fullscreen always runs at the desktop's size | SDL borderless fullscreen; the `window = WxH` setting only applies to windowed mode. | `[XCDE] XCDE_FULLSCREEN_MODE=WxH` (`runtime/nvn/vulkan/device.c`): exclusive fullscreen at the closest display mode, at the desktop's refresh rate. |
| Crash right after `[dlss] Streamline loaded`, on every start with DLSS chosen | `dlss_create_surface` called Streamline's proxy `vkCreateWin32SurfaceKHR` before `slSetVulkanInfo`, so Streamline had no instance dispatch yet and jumped to address 0. | Create the surface with SDL as before T257; DLSS does not need Streamline's surface hook (`runtime/nvn/vulkan/dlss.cpp`). |
| `xcde.exe --precompile` crashed ("Precompilation failed") | The precompile run starts before settings are read, so the default automatic upscaler loaded Streamline into the hidden window. | No DLSS when `XCDE_PRECOMPILING` is set. |
| Smudged fire embers and slightly ghosted HUD text with DLSS Quality | The particles carry no motion vectors, and the HUD is part of the image DLSS reconstructs from 960p. | Not fixed; it is DLSS behaviour. DLAA does not show it. |

### Known issue (not fixed): with DLSS, loading a save hangs the GPU

When DLSS is the upscaler, v6 often hangs the GPU about 10 s after a save starts loading. The driver resets (System
log: `nvlddmkm` event 153, kernel `LiveKernelEvent 141`) and the game stops with:

```
[vk] device fault: detail query failed
FATAL: sr failed: -4 (runtime/nvn/vulkan/exec.c:639)
```

The last lines before it are pipeline prewarm and `transform feedback mirror ... in device memory`. All runs below load
the same Colony 9 autosave from the title screen, on the same machine as the benchmarks:

| Upscaler | Output | ReShade + dlss5-bridge | Attempts | GPU hang |
|---|---|---|---|---|
| DLSS Quality | 3840x2160 | no | 3 (plain bat ×2, matrix; one hung while opening the save list) | 3 |
| DLAA | 3840x2160 | no | 1 | 1 |
| DLSS Quality | 2560x1440 | no | 1 | 1 |
| **FSR Quality** | 3840x2160 | no | 1 | **0** (104–119 fps in the world) |
| DLSS Quality | 3840x2160 | yes | 1 | 1 |
| DLAA | 3840x2160 | yes | 1 | 0 |
| DLAA / DLSS Quality | 2560x1440 | yes | 4 | 0 |

So it is tied to DLSS, not to this PR's fullscreen mode, and it is timing-dependent. While the bridge mirrors DLSS it
holds the game's queue around every evaluate, and loads mostly survived. Monado already re-binds its own state after
`slEvaluateFeature` (`vk_external_state_changed`), and `temporal.c` frees images through `vk_defer_image`, so neither is
an obvious cause.

Two leads to check:
- Streamline warns at every start: `Hook sl.common:Vulkan:CmdBindPipeline / CmdBindDescriptorSets / BeginCommandBuffer
  is NOT supported, plugin will not function properly`.
- It is not yet known whether this hang already existed before this PR. Without this PR's surface fix, v6 never got as
  far as loading a save with DLSS.

Until it is fixed, use FSR in `Play Monado.bat`; with `upscaler = auto` an NVIDIA GPU also picks DLSS. The DLSS 5 bat
at 1440p loaded every time.

## Benchmarks

Machine: AMD Ryzen 9 7900X, 32 GB, RTX 4070 Ti 12 GB, NVIDIA 610.88, Windows 11 26200, LG C4 TV (HDMI, 120 Hz).
Build: v6.0.0 plus this PR. Scene: Colony 9, Commercial District, loaded from the same autosave. Sources:
- RenoDX's 30-second telemetry windows in `build\ReShade.log` (`fps=`, and `NR cost: gpu_ms=`, which is the GPU time
  neural rendering takes per frame);
- the game's window-title fps counter, sampled while standing still and while walking.

Raw lines are in `docs/evidence/dlss5_2026-11-06.md`. The fps cap was 120.

| Output | DLSS | Neural rendering | Title screen | In the world | NR GPU time | VRAM |
|---|---|---|---|---|---|---|
| 3840x2160 borderless | DLAA | on | 33–40 fps | 29–30 fps, later a steady **9 fps** (paging) | 15–16 ms | 11.2 / 12 GB |
| 3840x2160 borderless | DLAA | off (F6) | ~72 fps (estimate) | 54–57 fps (estimate) | — | — |
| 2560x1440 exclusive | DLAA | on | 83 fps | 64–72 fps (walking 66–80) | 5.3 ms | 7.0 GB |
| 2560x1440 exclusive | Quality (960p) | on | 95 fps | 69–74 fps (walking 75–82) | 5.3 ms | — |
| 2560x1440 exclusive | Quality (960p) | off (F6) | — | 112–118 fps | — | — |

The "estimate" rows are RenoDX's own estimate of the fps without neural rendering, computed from its measured GPU time.
The other rows are measured. At 1440p, DLSS Quality instead of DLAA gained about 5–8 fps, but visibly softened ground
texture and HUD text. On this machine DLAA at 1440p already stays above 60 fps.

## What it changes in the picture

DLSS 5 does not sharpen the image; it re-lights it. With it on:
- shadows are deeper under armour, between plates and where feet meet the ground;
- metal looks like dark steel with highlights instead of flat blue-tinted plastic;
- emissive purple glows bloom less;
- the slight blue haze over the scene is gone.

With it off, the game looks as it does on the Switch: brighter, flatter and bluer. The comparison captures are not in
this repository because Constitution art. I forbids captures that show game assets.

## Code changes in this PR

- `runtime/nvn/vulkan/dlss.cpp`: `dlss_init_early` skips DLSS when `XCDE_PRECOMPILING` is set; `dlss_create_surface`
  returns `false`, so SDL creates the surface.
- `runtime/nvn/vulkan/device.c`: `XCDE_FULLSCREEN_MODE=WxH`.
- `tools/dlss5/`: the bat, `bat_settings.ps1` and the `ReShade.ini` seed.
