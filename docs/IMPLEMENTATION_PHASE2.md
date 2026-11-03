# GPU implementation phase 2

This phase adds persistent Vulkan adapter policy, a GPU/Vulkan diagnostics tab, safer feature negotiation, correct shared-memory classification, and repeatable hardware-test instructions.

## Changed files

- `runtime/settings.h`: GPU policy settings and enums.
- `runtime/host/settings_ini.c`, `runtime/host/settings_boot.c`: defaults and INI persistence.
- `runtime/nvn/vulkan/vk.h`: selected adapter index and Vulkan driver properties.
- `runtime/nvn/vulkan/device.c`: manual/automatic selection integration, driver diagnostics, host-visible heap classification, optional-feature queries, device-fault query validation, bounded queue counts, checked extension enumeration, and corrected feature-log declaration order.
- `runtime/nvn/vulkan/temporal_fg.inc`: compatibility profile disables frame generation.
- `runtime/ui/menu_vk.cpp`: GPU/Vulkan tab with adapter diagnostics and restart-scoped controls.
- `xcde.ini`: documented default GPU settings.
- `tests/unit/test_vk_source_contracts.py`: regression contracts.
- `tests/gpu/GPU_COMPATIBILITY_MATRIX.md`: per-family hardware acceptance matrix.
- `tools/collect_gpu_diagnostics.bat`: Windows diagnostic collection helper.
- `docs/AUDIT_XDE_COMPILAR_V3_2026-11-02.md`: cumulative audit and remaining limitations.

## Verification

- Python unit tests: 59 total; 49 passed; 10 skipped because `unicorn`, `lz4`, and `capstone` are unavailable; 0 failed.
- GCC syntax checks with `-Wall -Wextra -Werror`: `runtime/host/settings_ini.c` and `runtime/host/settings_boot.c` passed.
- A standalone C load/save round-trip test for all four GPU settings passed.
- Full Windows C/C++ build and GPU runtime tests were not possible in this environment. No AMD, NVIDIA, or Intel compatibility certification is claimed.

See `docs/AUDIT_XDE_COMPILAR_V3_2026-11-02.md` and `tests/gpu/GPU_COMPATIBILITY_MATRIX.md` before testing.
