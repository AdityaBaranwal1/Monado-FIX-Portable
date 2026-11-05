# T259–T261 — evidencia local de rendimiento e interfaz

Fecha: 2026-11-04. Estado: implementación compilada y pruebas locales completas; aceptación jugable ampliada pendiente. No se publica una distribución ni contenido del juego.

## Entorno y construcción

Windows x64; Ryzen 7 7730U indicado por el usuario; SDL informa 16 procesadores lógicos y 15 711 MiB de RAM. Dispositivo seleccionado: **AMD Radeon (TM) Graphics**, integrada, vendor `1002`, driver Vulkan bruto `8388887`, Vulkan 1.3.260. Esta evidencia cubre este dispositivo/controlador; no certifica Intel, NVIDIA, otras AMD ni selección híbrida en laptop.

Se verificaron los `exefs/romfs` proporcionados por el usuario como XCDE 2.0.0 y se generaron/compilaron localmente. Los originales no se modificaron. Las pruebas usan directorios de guardado separados bajo `build/`. Código generado, capturas del juego, cachés y binarios quedan en directorios excluidos.

Antes de editar se guardaron las fuentes de referencia y sus hashes en `build/reference/before-performance-ui.zip` y `.json`. No hay directorio `.git` en este árbol; no se realizaron commits ni publicación.

La construcción completa se verificó con Clang de MSYS2 instalado localmente en `build/clang-sdk`, GNU ABI, `-O1 -march=x86-64-v2 -fno-strict-aliasing`, sin fast-math y con `-ffp-contract=off` en el juego generado. `build/build_native.py` y los response files de `build/full/` registran la construcción local. Las URL/SHA de paquetes están en `build/clang-sdk/manifest.json`, las licencias en `build/runtime-licenses/`. El pipeline predeterminado Zig/MSYS2 se conserva: Zig falló en este entorno al gestionar archivos/temporales y no se utiliza ese fallo como prueba de un defecto del proyecto.

| Artefacto | SHA-256 |
|---|---|
| Instalado `build/xcde.exe` | `8fc06a0953c91a7a07c96ad91cb84c05a50c2da761d37942c623c23c716a60a2` |
| Instalado `build/xcde_launcher.exe` | `3a3e4920d63cdae8662f497a1310d6fb78d7d26a9692921531038e103ef1b52a` |
| Referencia medida `build/xcde_reference.exe` | `cacea9f9176211fc1c37c61f5d0718d12bdcc39c9dc0c794278c948d95aa997b` |
| Candidato durante A/B | `9e54ad49f021e7beabf664b15983962ca66275776f4d7a7b0505f52b18cb86b0` |

La referencia incorpora solamente el observador ligero de intervalos nativos sobre las fuentes anteriores. Tras el A/B se corrigieron colores heredados del título/pestañas y la restauración de la opción FG exclusiva de cinemáticas al aplicar un perfil; el binario final incluye esas correcciones y pasó compilación/pruebas y otro arranque. No se presenta el A/B como una medición del hash final. `build/installed-build.json` identifica los ejecutables instalados.

## Comprobaciones sin datos del juego

| Comprobación | Resultado y registro |
|---|---|
| Suite Python | **72 pruebas correctas**, `build/unit-results.log`. El mensaje `VIOLATION ... a.nsp` es una fixture deliberada del guard de empaquetado, no un archivo del usuario. |
| Validación nativa | **17 pruebas ejecutadas correctas**, 18 módulos runtime/C++ compilados, launcher compilado/enlazado, cero fallos; `build/clang-validation.log`, `build/validation-clang/`. |
| Prueba de firma NVIDIA | Compila; no ejecutada en esta validación porque falta la DLL oficial en la ruta de esa prueba. No implica validación de DLSS. |
| Guard de fuentes | `tools/package_guard.py` sobre `runtime tools tests docs games data third_party`, salida 0; `build/source-guard.log`. No se ejecutó el modo basado en índice Git porque no existe `.git`. |
| Smoke Vulkan 1.3 core | Clear/readback y 256 píxeles correctos; `build/gpu-smoke.log`. |
| Smoke Vulkan 1.2 KHR forzado | Correcto en el mismo driver 1.3; `build/gpu-smoke12.log`. No demuestra compatibilidad con hardware/driver exclusivamente 1.2. |

Las pruebas nuevas ejercitan el código compartido real: identidad/secuencia/layout de descriptores, presupuesto de repeats, wrap de timestamps, perfiles y restauración conservando FPS, límites de resolución y respuesta GPU/CPU, precedencia/transferencia de propietarios de texturas y caché RomFS con cuatro vías fijadas, descriptor privado y cinco lectores concurrentes. Se amplió el round-trip del INI. La prueba Python del informe comprueba percentiles, comparación y rechazo de metadatos distintos.

Para repetir la validación de componentes en este entorno:

```powershell
$env:XCDE_SDK='C:/msys64/ucrt64'
.venv/Scripts/python -X utf8 tools/validate_port.py --clang build/clang-sdk/ucrt64/bin/clang.exe --build-ui
```

## Comparación visual y arranque real

Con configuración Equilibrado idéntica, replay `tests/routes/title_idle.txt` y guardados de prueba, las capturas de referencia/candidato en cuadros **1200 y 2600 fueron idénticas en RGB**: diferencia máxima 0, media 0 y 0 % de píxeles distintos. Registro: `build/qa/visual-comparison.json`; imágenes privadas en el mismo directorio. Esto cubre la pantalla de título con escenario, no campo/combate ni todos los efectos.

Patata se ejecutó durante 1300 cuadros con límites 360–540, objetivo 30, FSR 1, sin AA/DOF/blur/FG, memoria reducida y **sin recortes decorativos experimentales**. Se capturó el cuadro 1200, se abrió F1 en 1250 y se capturó en 1300. La temporización GPU asíncrona estuvo activa. Registros: `build/qa/potato-boot.log`, `potato-metrics.csv`, `potato.ini` y capturas locales. No hubo crash en esa ruta; no demuestra estabilidad prolongada.

El ejecutable final se arrancó nuevamente con el INI de Patata, abrió F1 y terminó tras la captura del cuadro 100: `build/qa/final-boot.log`, `final-f1.png`. Es una comprobación de arranque/interfaz, sin medir FPS de juego estable.

El dispositivo local permitió importar el heap completo del invitado y no activó staging global completo. Por ello no se atribuye aquí el bajo rendimiento comunicado por el usuario al fallback de staging.

## A/B de cuadros nativos

Método: tres pares alternados referencia/candidato, misma configuración explícita, misma ruta y cachés calientes; ventana oculta con presentación real en cada cuadro (`--present`), audio desactivado, guardados aislados, observador ligero `XCDE_NATIVE_INTERVALS`, sin perfilador CPU detallado. Medición de cuadros 700–1198 inclusive, excluyendo la captura de 1200 y su intervalo posterior. Se conservan tirones si aparecen; no se eliminan selectivamente.

Configuración: objetivo 30, resolución coincidente con ventana 1280×720, FSR, blur original, V-Sync activo y HUD FPS desactivado. SHA-256 INI: `b2ad1c460e08bec079cc5e733d5a478022acb3e9b526d95373e655fd6c160d75`. SHA-256 ruta: `6fa6b9ae9f5fc79f9839a11433370a2fd52b9f48952faf2298af65e7cf5cf576`.

| Par | FPS referencia | FPS candidato | Cambio FPS | Cambio p95 de intervalo |
|---|---:|---:|---:|---:|
| 1 | 32,537 | 32,532 | −0,018 % | +0,008 % |
| 2 | 32,320 | 32,535 | +0,666 % | −0,755 % |
| 3 | 32,389 | 32,360 | −0,090 % | +0,138 % |

**Mediana del cambio emparejado: −0,0177 %**, prácticamente sin diferencia en esta escena limitada. No acredita una subida de FPS ni garantiza 30 FPS en juego; tampoco aparece una regresión del 3 % en esta muestra. La frecuencia observada pertenece al replay de título y no certifica velocidad temporal de combate/cinemáticas.

Datos originales, hashes de CSV y resumen: `build/ab-present/results.json` y seis CSV adyacentes. Ese JSON se recalculó con el intervalo anterior; es el resultado autoritativo frente al resumen inicial de consola que todavía incluía el intervalo de captura. La herramienta ya excluye ese intervalo al repetir la prueba:

```powershell
.venv/Scripts/python -X utf8 tools/benchmark_native.py --reference build/xcde_reference.exe --candidate build/xcde.exe --ini build/qa/balanced.ini --present --output build/ab-repeat
```

El directorio de salida debe ser nuevo. El observador está presente en ambos ejecutables. Cambiar de configuración, driver, modo de instrumentación o escena invalida una comparación directa.

Una muestra separada con perfilado detallado, cuadros 300–360 (61 cuadros), registró **11 447 aciertos de caché de descriptores y 34 577 actualizaciones**: `build/qa/optimized-metrics.csv`, `metrics-summary.json`. Demuestra reutilización de sets y llamadas evitadas en esa muestra; no mide una ganancia global de rendimiento. El perfilador añade coste y no se mezcla con el A/B ligero.

## Interfaz

El launcher renderizó capturas nativas sin fallos a 720p/1080p y con escala 200 %, en español/inglés, Inicio/Gráficos/Controles/Diagnóstico. Registros: `build/ui/results.json`, `final-results.json`. Las capturas finales `home-final.png` y `dpi-final.png` corresponden al ejecutable instalado, con Jugar disponible. Se verificaron texto legible, controles apilados con poco ancho efectivo, scroll y pie fijo. F1 usa el mismo tema negro/verde, radios y controles compartidos.

Las capturas de UI usan el mecanismo `--shot` existente y no escriben preferencias del usuario. No sustituyen una sesión de navegación manual con mando ni una prueba prolongada del launcher.

## Siguientes comprobaciones, en orden de utilidad

1. Registrar una ruta corta reproducible de campo y otra de combate desde guardados de prueba. Comparar el binario final con referencia, tanto Equilibrado como Patata, midiendo FPS nativos, p95/p99, GPU y bytes de staging. Separar compilación fría de caché caliente.
2. Ejecutar una sesión prolongada con cambios de mapa y cinemáticas, comprobando memoria de proceso/driver, texturas compartidas, descargas de recursos y velocidad temporal. Incluir Future Connected y el comportamiento a 30/60 FPS antes de recomendar frecuencias mayores.
3. Repetir la matriz en Intel integrada, AMD de otra generación y NVIDIA, incluyendo una laptop híbrida con elección explícita de GPU. Usar la ruta/API negociada y las capacidades informadas, sin forzar extensiones ausentes. DLSS requiere RTX y su propia aceptación visual.
4. Evaluar por separado los cinco recortes decorativos opcionales en escenas donde cada efecto sea visible. Mantenerlos desactivados como recortes predeterminados hasta comprobar sombras, reflejos, niebla, transparencias, HUD y cambios de resolución.
5. Decidir la siguiente optimización a partir del cuello medido: si predomina GPU, resolución/efectos; si predomina CPU, trazas de traducción/servicios y llamadas de dibujo; si crecen esperas/subidas, investigar staging y readbacks. Eliminar barreras o reescribir todo el direccionamiento sin esa evidencia añadiría riesgo sin una mejora demostrada.

En esta máquina, Patata + objetivo 30 es un punto de partida conservador para probar; elegirlo es explícito. No se sobrescribieron las preferencias ni los guardados del usuario.
