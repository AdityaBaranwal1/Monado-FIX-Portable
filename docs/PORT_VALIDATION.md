# Monado V5: correcciones y validación del port

Fecha: 2026-11-04. Trabajo sobre el árbol local sin metadatos `.git`, sin `targets`, sin código generado del juego y sin ejecutable completo del juego. No se ha publicado una release.

## Cambios disponibles

- **T247 — Configuración:** launcher y runtime comparten un escritor que conserva claves desconocidas y reemplaza el INI mediante un temporal en el mismo directorio. Un error conserva la configuración anterior. Se escriben también los bindings/defaults para que un ajuste antiguo no reaparezca después de restablecerlo.
- **T248 — Adaptadores:** detección común, selección por nombre, tipo de GPU, UUID persistente y motivos de rechazo. Prioridad: `LARECOMP_GPU`, `gpu_uuid`, `gpu_index` antiguo, automático. Si no está disponible la preferencia, se registra el retorno a automático. Cambiar GPU requiere reiniciar.
- **T249 — Tiempo:** muestreo en el limitador del juego con reloj monotónico/virtual; umbrales de seguidores con frecuencia fraccionaria, sin multiplicadores enteros ni dependencia de los FPS solicitados. Se corrige el redondeo del limitador de cinemáticas a 144 Hz. No se han identificado ni parcheado todos los contadores de menús/combate sin el ejecutable del juego.
- **T250 — Vulkan:** negociación 1.2 + `VK_KHR_dynamic_rendering` + `VK_KHR_synchronization2`, o 1.3 core. Despacho, VMA, ImGui y compilación SPIR-V usan la versión negociada. En 1.2 se desactivan GPL y SDK temporales opcionales; permanece el escalado espacial. DLSS comprueba sus requisitos opcionales de GPU, extensiones, features y colas antes de solicitarlos.
- **T251 — Recursos:** perfil Rendimiento explícito, resolución dinámica según el objetivo FPS/cinemáticas, trabajadores según CPU/RAM y presupuestos menores de caché. La presión de memoria permite retirar más pipelines reconstruibles conservando la espera por su secuencia GPU. No se afirma una ganancia de FPS sin un benchmark jugable.
- **T252 — Sincronización:** feedback con copias por defecto; flush de subidas e invalidación de readbacks no coherentes; barrera de GPU a host; reinicio de historia temporal al cambiar resolución/escalador. Los readbacks conservan las dimensiones originales aunque cambie la swapchain durante present.
- **T253 — Diagnóstico/cachés:** límites y comprobaciones de cabeceras, identificación de GPU/driver en fallos de shaders, guardado atómico de cachés sueltas/pipelines y claves SPIR-V que distinguen target/optimizaciones/revisión. El pack conserva su protocolo de append/index con bloqueo y detección de registros incompletos. `XCDE_ASSET_TRACE=1` registra aperturas/lecturas para localizar el fallo del DLC; `XCDE_STRICT_RENDER=1` detiene los fallos de compilación GLSL con diagnóstico.
- **T254 — Memoria del juego:** importación comprueba tipos de memoria compatibles, coherencia y resultado de bind. Al sustituirla, conserva las importaciones anteriores hasta terminar su secuencia GPU. El fallback de staging para shaders admite heaps completos dentro de `maxStorageBufferRange` y fusiona solo las palabras modificadas por GPU, conservando cambios independientes del CPU.
- **T255 — Reutilización de ventana:** adquirir una textura espera a que termine su trabajo de presentación y después su secuencia GPU. Antes solo rotaba índices y señalaba éxito inmediatamente. Se limita el número activo al de texturas reales. Esta corrección evita una reutilización prematura concreta; su relación con todos los parpadeos reportados requiere jugar.

El perfil Equilibrado conserva los ajustes existentes. Seleccionar Rendimiento aplica resolución dinámica, FSR 1 espacial, sin generación de cuadros, SMAA ni DOF; después se pueden ajustar individualmente. Las tres mitigaciones de efectos volumétricos AMD siguen activas por defecto en sus integradas. `gpu_keep_heavy=1` o el checkbox permite restaurar esos efectos; `XCDE_AMD_KEEP_HEAVY` conserva su prioridad para diagnóstico.

## Preparación y pruebas sin juego

Desde la raíz, con Python instalado; todo el SDK y las salidas quedan en `build`:

```powershell
python -m pip install --target build/toolchain ziglang==0.13.0 capstone lz4 unicorn
python tools/setup_validation.py
$env:PYTHONPATH = "$PWD\build\toolchain"
New-Item -ItemType Directory -Force build/tmp | Out-Null
$env:TEMP = "$PWD\build\tmp"
$env:TMP = $env:TEMP
python -X utf8 -W error::ResourceWarning -m unittest discover -s tests/unit -q
python tools/validate_port.py --build-ui
build/validation/gpu_diagnostics.exe
build/validation/gpu_diagnostics.exe --smoke
build/validation/gpu_diagnostics.exe --smoke-vulkan12
```

`setup_validation.py` necesita Python 3.14 para zstd de la biblioteca estándar, o el paquete `zstandard` en Python 3.12/3.13. Descarga paquetes de MSYS2 fijados por versión y registra URL/SHA-256 en `build/deps/validation-sdk.json`. No instala MSYS2 globalmente. `XCDE_ZIG` y `XCDE_SDK` permiten usar herramientas ya instaladas.

La validación compila 17 módulos C/C++, pruebas nativas, diagnóstico y, con `--build-ui`, el launcher independiente. El launcher de `build/validation` es un artefacto de comprobación; para jugar se necesita construir el runtime y colocar el launcher en la distribución normal. No existe todavía un `xcde.exe` completo en este árbol.

Las pruebas smoke generan un color de 16×16 sin shaders ni recursos del juego, ejecutan dynamic rendering/sync2/timeline y comprueban los 256 píxeles leídos. Comprueban el despacho y la GPU reales; no prueban los shaders traducidos, la presentación del juego, FSR/DLSS ni el DLC. Forzar 1.2 en un driver 1.3 tampoco certifica un driver antiguo que solo soporte 1.2.

En esta máquina Windows bloqueó ejecuciones de `test_time_math`, `test_translate_xfb`, y posteriormente de nuevas compilaciones de `test_port_core`, `test_settings_roundtrip` y `gpu_diagnostics`, con WinError 4551 (Control de aplicaciones). `--skip-test <nombre>` permite continuar las demás comprobaciones de forma explícita. No se modificó la directiva. Las omisiones no cuentan como pruebas aprobadas. Core, settings, time math y ambos smoke habían pasado en compilaciones anteriores; las nuevas compilaciones bloqueadas no se declaran ejecutadas.

## Pruebas jugables pendientes

1. Proporcionar los `exefs/romfs` propios 2.0.0 al procedimiento de build existente. Usar copias de configuración/guardados para pruebas y desactivar mods en esas copias. No incorporar datos derivados al repositorio.
2. Convertir las rutas conocidas a tiempo: `python tools/timed_route.py tests/routes/load_walk.txt build/load_walk_ms.txt --source-fps 30`. Usar `XCDE_INPUT_REPLAY=build/load_walk_ms.txt`; el encabezado `v2-ms` selecciona milisegundos desde la primera muestra de entrada. `XCDE_INPUT_TIME=1` permite registrar entradas con esa unidad. Los ficheros v1 conservan la interpretación por cuadros.
3. Comparar 30/60/120/144/165 FPS y sin límite con frame generation desactivado, mismo guardado, resolución, clima, mods y versión de driver. Medir FPS realmente conseguidos y separar el tiempo de cargas. Comparar posiciones/relojes y duración de animaciones a los mismos tiempos de entrada; no comparar el cuadro N entre frecuencias distintas. Tolerancia temporal objetivo: 1 %, considerando la cuantización de una actualización.
4. Cubrir menús y repetición de teclas, seguidores, cámara, batalla, cinemáticas, transiciones, guardado/carga y Future Connected. Capturar reproducciones de las texturas azules y parpadeos antes/después. Repetir el mismo build dos veces para cuantificar el ruido conocido antes de comparar hashes/imágenes.
5. En NVIDIA, probar cambios de resolución interna/salida, fullscreen, minimizar/restaurar, FSR/DLSS disponibles y drivers Vulkan 1.3/1.4. En un equipo híbrido, elegir cada GPU por nombre, comprobar el adaptador activo y probar la pérdida/reordenación de adaptadores al reiniciar.
6. Medir tres recorridos iguales por configuración: caché fría/caliente, FPS, tiempos de cuadro p50/p95/p99, RAM, presupuesto GPU y tirones. Registrar alimentación/estado térmico en laptops. No atribuir a GPU un límite de CPU ni contar cuadros generados como nativos.
7. Ejecutar sesiones de 60 minutos por GPU, incluyendo cambios de mapa y DLC, con guardado/carga al final. Actualizar la matriz por modelo/driver; no marcar toda una familia a partir de un dispositivo.

## Límites abiertos

- **Intel y heap por ventanas:** el staging global de esta etapa requiere que el heap completo quepa en un descriptor. Si excede `maxStorageBufferRange`, se detiene con diagnóstico; no se envía un descriptor inválido ni se sustituye silenciosamente la memoria del shader por ceros. El direccionamiento/importación por ventanas de T244 continúa pendiente. Las subidas de vértices/índices ya tienen copias acotadas independientes. No se promete que Intel HD 620 o el DLC en Intel estén solucionados.
- **144 FPS:** hay correcciones de temporización conocida y reutilización de recursos, pero no se declara resuelto el parpadeo general ni la aceleración de todos los menús sin una reproducción jugable y la identificación de sus contadores.
- **Texturas azules/Iris/NVIDIA:** cambios preventivos y causas concretas de sincronización corregidas en código; cierre de cada reporte pendiente de reproducción en el dispositivo afectado.
- **Rendimiento y memoria prolongada:** sin resultados del juego ni de todas las familias. El staging completo es una alternativa de compatibilidad costosa, no una optimización de integradas; falta medirlo y sustituirlo por ventanas donde corresponda. La memoria privada del driver sigue siendo un riesgo independiente de los presupuestos de Monado.
- No se implementaron colas principales graphics/present separadas, otro backend gráfico ni recuperación automática del juego después de device-lost.

La detección `usable=1` informa capacidades iniciales del backend, no compatibilidad jugable certificada. La evidencia real de esta etapa está en `docs/evidence/port_2026-11-04.md`.
