# XDE-Compilar-V3 / Monado — auditoría técnica inicial

Fecha: 2026-11-02

## Alcance y límites

Se inspeccionó el ZIP recibido (894 entradas). El proyecto que contiene se identifica internamente como **Monado / XCDE**: recompilación estática x86-64 de Xenoblade Chronicles: Definitive Edition con runtime NVN sobre Vulkan. No es un emulador convencional.

No se incluyeron ROM, firmware, claves ni datos de juego adicionales. No se generaron ni reconstruyeron archivos protegidos.

## Cambios aplicados en esta revisión

1. `runtime/nvn/vulkan/device.c`: la recuperación cuando falla la creación de la instancia con validación ya no decrementa ciegamente el número de extensiones. Ahora elimina `VK_EXT_debug_utils` por nombre y conserva las extensiones que DLSS/Streamline haya añadido después.
2. `runtime/nvn/vulkan/device.c`: la selección automática considera solamente dispositivos Vulkan 1.3 con una cola graphics+compute que también pueda presentar a la superficie SDL. Evita seleccionar primero una GPU discreta que después resulte inutilizable y abortar sin probar la iGPU.
3. `runtime/nvn/vulkan/device.c`: el registro ahora incluye vendor ID, device ID, versión de driver, tipo de dispositivo, heaps de memoria local/no local y familia de cola seleccionada. En dispositivos integrados se identifica explícitamente la clase de memoria compartida; los heaps no se etiquetan como VRAM dedicada.
4. `runtime/nvn/vulkan/device.c`: los bits opcionales de Vulkan 1.1/1.2/1.3 que se habilitan ahora se copian de las capacidades consultadas al dispositivo, en vez de solicitar indiscriminadamente `VK_TRUE`. Se comprueban explícitamente las capacidades realmente requeridas por este backend (dynamic rendering, synchronization2 y timeline semaphores), con diagnóstico concreto si faltan.
5. `tests/unit/test_vk_source_contracts.py`: se añadieron pruebas de regresión para esos contratos.

## Verificación ejecutada

- Suite Python previa: 50 pruebas, 40 aprobadas, 10 omitidas por faltar `unicorn`, `lz4` y `capstone`; cero fallos.
- Suite ampliada: **54 pruebas, 44 aprobadas, 10 omitidas, 0 fallidas**.
- Las 10 omitidas requieren `unicorn`, `lz4` y `capstone`, ausentes en este entorno.
- `tools/package_guard.py .` sin `--git` no es el modo de CI y reporta directorios de mods ya presentes en el ZIP. El archivo recibido no contiene metadatos `.git`, por lo que no se pudo reproducir el modo CI `--git` sobre el índice original.
- No se afirma compilación C/C++ ni compilación Windows en este entorno: no están instalados MSYS2 UCRT64, MinGW-w64 ni las cabeceras SDL3/Vulkan. La compilación completa además requiere los archivos legítimos del juego requeridos por `scripts/build_all.sh`.

## Compatibilidad: estado honesto

| GPU / backend | Estado |
|---|---|
| NVIDIA RTX | El README del proyecto la identifica como probada por el autor original; no se repitió una sesión de juego en esta auditoría. |
| AMD Vega / Polaris / RDNA 1–4 | No verificada en hardware en esta auditoría. La selección y el reporte de capacidades mejoraron, pero no prueban renderizado correcto. |
| Intel UHD / Iris Xe / Arc | No verificada en hardware en esta auditoría. |
| Vulkan 1.3 | Requisito actual del backend; no se añadió una ruta Vulkan 1.2 ni se afirma compatibilidad con drivers que no ofrezcan las funciones requeridas. |

## Pendientes importantes

- La creación de dispositivo todavía contiene rutas NVN que pueden depender de capacidades específicas; cada una requiere auditoría funcional y, cuando proceda, una ruta alternativa real.
- La selección exige una misma familia de colas graphics+compute+present; no se implementó aún selección de familias separadas para el renderer principal.
- No se implementó todavía un selector manual de GPU en el menú. `LARECOMP_GPU=<índice enumerado>` sigue siendo el mecanismo manual de selección.
- No se implementó todavía un perfil automático por fabricante/modelo para memoria, texturas, resolución o límites de VRAM.
- No se añadió una UI completa para resolución, texturas, filtrado anisotrópico, cachés, FPS y opciones avanzadas de Vulkan.
- Requieren pruebas reales: validación de sincronización en AMD/Intel, shaders/pipelines, artefactos, device-lost, consumo de VRAM compartida, audio/vídeo, controles, guardado/carga y pruebas prolongadas.
- El proyecto declara en `docs/spec_lowend_gpu.md` riesgos conocidos sobre feedback sampling, sincronización, colas y shaders. Deben seguir considerándose abiertos hasta su aceptación en hardware.

## Compilación Windows

La ruta de compilación oficial sigue siendo `Build Monado.bat`, que configura MSYS2 UCRT64, Python 3.12 y dependencias y después ejecuta `scripts/build_all.sh`. La compilación completa también requiere los directorios `targets/exefs` y `targets/romfs` de la copia legítima versión 2.0.0 del usuario. Esos archivos no están en el ZIP y no se suministran.


## Segunda fase — controles GPU y robustez adicional

Se amplió el código en esta iteración:

1. `runtime/settings.h`, `runtime/host/settings_ini.c` y `runtime/host/settings_boot.c`: se añadieron opciones persistentes `gpu_index`, `gpu_profile`, `gpu_gpl` y `gpu_memory_budget`, con valores por defecto seguros y compatibles con la configuración anterior.
2. `runtime/ui/menu_vk.cpp`: se añadió la pestaña **GPU / Vulkan**, con información del adaptador activo (fabricante, IDs, driver, versión Vulkan, índice y clases de memoria) y controles de selección/perfil que indican que requieren reinicio.
3. `runtime/nvn/vulkan/device.c`: el perfil de compatibilidad desactiva las rutas opcionales de pipeline libraries y host-memory import; la selección manual se integra con `LARECOMP_GPU`; `VK_EXT_memory_budget` respeta la configuración; la detección de memoria distingue heaps host-visible compartidos de heaps device-local no host-visible.
4. Se consultan los bits de características de custom border color, vertex divisor y device fault antes de habilitarlos. La consulta de detalles de device fault ahora comprueba el resultado antes de leer las estructuras.
5. Se corrigió el uso de `f11.storageBuffer16BitAccess` antes de declarar `f11` y se acotó la cantidad de colas frente al arreglo fijo de prioridades.
6. Se añadieron `tests/gpu/GPU_COMPATIBILITY_MATRIX.md` y `tools/collect_gpu_diagnostics.bat` para recopilar evidencia por GPU/driver sin incluir archivos protegidos.

### Verificación de esta fase

- Suite Python: 59 pruebas, 49 aprobadas, 10 omitidas por faltar `unicorn`, `lz4` y `capstone`; 0 fallidas.
- `gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only -Iruntime runtime/host/settings_ini.c` y `settings_boot.c`: correctos.
- Prueba C aislada de carga/guardado de `gpu_index`, `gpu_profile`, `gpu_gpl` y `gpu_memory_budget`: round-trip correcto.
- La compilación C/C++ completa de Windows sigue sin ejecutarse: este entorno carece del SDK/cabeceras Vulkan, SDL3, shaderc, toolchain MSYS2 UCRT64 y de `targets/` legítimo.
- No se ha verificado renderizado real ni compatibilidad certificada en ninguna GPU adicional. La matriz sigue pendiente de ejecutar en hardware.

### Límites que siguen abiertos

- El backend sigue requiriendo Vulkan 1.3 con dynamic rendering, synchronization2 y timeline semaphores; no se ha escrito un backend alternativo Vulkan 1.2.
- La selección principal requiere graphics+compute+present en una misma familia de colas. No se implementó aún la ruta de familias separadas porque exige revisar el modelo de presentación/sincronización y probarlo en hardware.
- No se añadió un límite de VRAM artificial: VMA consulta el presupuesto real si la extensión está disponible, pero el presupuesto no limita por sí solo cada asignación. La estrategia de reducción de recursos debe medirse y diseñarse con datos de cargas reales.
- Los ajustes de textura, filtrado anisotrópico y resolución ya existentes no se han duplicado con controles que no estén conectados a rutas reales del renderer.
- Audio/vídeo, entrada, guardado, HLE/IPC/SVC, estabilidad prolongada y regresiones gráficas requieren ejecución integral con los archivos de juego del usuario.


## Fase 3 — memoria y resiliencia de caché (2026-11-02)

Cambios adicionales implementados:

1. `runtime/nvn/vulkan/device.c`: la lectura de `cache/vk_pipelines.bin` ahora valida `fseek`/`ftell`, rechaza archivos vacíos o mayores de 128 MiB y comprueba `malloc` antes de `fread`. Una caché corrupta, truncada o anormalmente grande se ignora con diagnóstico y el dispositivo crea una caché vacía.
2. `runtime/nvn/vulkan/device.c`: el guardado de la caché también rechaza tamaños superiores a 128 MiB y comprueba fallos de asignación antes de pedir los datos a Vulkan.
3. `runtime/ui/menu_vk.cpp`: la pestaña GPU/Vulkan muestra presupuesto/uso de cada heap mediante VMA, bytes asignados por Monado y una barra de presión de memoria. En GPU integradas aclara que el heap de memoria compartida no es VRAM dedicada.
4. `tests/unit/test_vk_source_contracts.py`: pruebas para límites de tamaño, errores de asignación/lectura y telemetría de presupuesto.

Verificación de esta fase:

- Suite Python: 62 pruebas; 52 aprobadas, 10 omitidas por dependencias ausentes (`unicorn`, `lz4`, `capstone`), 0 fallidas.
- `gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only runtime/host/settings_ini.c runtime/host/settings_boot.c -Iruntime`: correcto.
- El ZIP fue comprobado con `ZipFile.testzip()`.

## Pendientes que siguen abiertos (no simulados)

- Vulkan 1.2: el backend usa directamente comandos core de Vulkan 1.3 (`vkCmdBeginRendering`, `vkCmdPipelineBarrier2`, `vkQueueSubmit2`, `vkCmdWriteTimestamp2`) y `VkPhysicalDeviceVulkan13Features`. Una ruta 1.2 requiere wrappers de despacho, negociación de `VK_KHR_dynamic_rendering` y `VK_KHR_synchronization2`, cadena de features específica y pruebas de validación; no basta con relajar la comprobación de `apiVersion`. Por seguridad se mantiene el requisito 1.3.
- Colas separadas para el renderer principal: el runtime conserva una cola principal universal graphics+compute+present. Las colas extra existentes se reservan para FSR frame generation y Streamline. Dividir la cola principal requiere separar submit/present y sincronización en todo el backend; no se ha fingido que la selección de familias ya esté generalizada.
- La política de memoria informa presupuesto y activa la extensión VMA cuando existe, pero no impone todavía un límite dinámico propio ni expulsa recursos por presión. Hacerlo exige clasificar recursos NVN, conocer su vida útil y probar la recuperación de texturas/pipelines en juego.
- UI de GPU permite seleccionar por índice Vulkan y ver el adaptador activo, pero no enumera una lista de nombres seleccionables en el combo; el índice se confirma en el siguiente arranque.
- No se ha ejecutado compilación completa Windows ni prueba de juego. No hay certificación para ninguna familia AMD/Intel/NVIDIA en esta sesión.

La matriz `tests/gpu/GPU_COMPATIBILITY_MATRIX.md` sigue siendo un plan de aceptación y todas sus familias permanecen pendientes de hardware.
