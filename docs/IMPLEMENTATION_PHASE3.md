# Implementation phase 3 — memory diagnostics and cache resilience

## Implemented

- Pipeline cache input is treated as untrusted: seek errors, empty files, files above 128 MiB, allocation failures and short reads are rejected. The renderer falls back to an empty Vulkan pipeline cache.
- Pipeline cache save checks the same 128 MiB bound and handles allocation failure without dereferencing a null pointer.
- The in-game GPU/Vulkan tab reports per-heap VMA usage, budget and Monado allocation bytes, with a pressure bar.
- Integrated GPU messaging explicitly distinguishes shared system memory from dedicated VRAM.
- Source-contract regression tests cover the new safeguards and memory reporting.

## Verification

- 62 Python unit tests: 52 passed, 10 skipped for missing `unicorn`, `lz4`, `capstone`; no failures.
- GCC strict syntax check passed for the settings translation units.
- Archive integrity check passed.

## Deliberately not claimed

- Vulkan 1.2 fallback is not implemented. This renderer directly calls Vulkan 1.3 core commands and consumes Vulkan 1.3 feature structures. A correct fallback needs extension dispatch wrappers and feature-chain negotiation across the renderer.
- Main renderer graphics/compute/present queues are not split. That would require an end-to-end submission and presentation synchronization refactor.
- VMA budget telemetry is diagnostic, not an active eviction/VRAM cap policy.
- No Windows full build or hardware/gameplay validation was run.

See `AUDIT_XDE_COMPILAR_V3_2026-11-02.md` for the cumulative status and `tests/gpu/GPU_COMPATIBILITY_MATRIX.md` for hardware acceptance criteria.
