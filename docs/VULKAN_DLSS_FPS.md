# Selector Vulkan, DLSS y contador FPS

Continuación local del 2026-11-04. Estos cambios necesitan reconstruir el juego con sus archivos 2.0.0; copiar las DLL por sí solo no añade las opciones a un ejecutable antiguo.

## Opciones

En **Gráficos / GPU** del launcher y **GPU / Vulkan** del menú F1:

- **Ruta Vulkan:** Automático, 1.2 (KHR), 1.3 y 1.4. Las opciones incompatibles con la GPU seleccionada o el loader instalado aparecen deshabilitadas. Automático escoge la mayor ruta soportada hasta 1.4. Si cambia la máquina o el driver y una preferencia guardada deja de estar disponible, se registra el retorno a una ruta compatible. Requiere reiniciar.
- El selector elige la ruta de API del backend. La instancia permite consultar las capacidades del driver; la ruta activa se muestra por separado de la versión máxima anunciada por la GPU. 1.4 utiliza el renderer existente y SPIR-V compatible; la opción no añade efectos exclusivos de 1.4. La ruta 1.2 conserva el escalado espacial y desactiva DLSS y los SDK temporales de esta implementación.
- **Escalado Automático:** predeterminado para configuraciones nuevas. En la GPU NVIDIA elegida, DLSS SR se activa cuando el SDK confirma compatibilidad de GPU, driver, features y colas; en los demás casos se usa FSR 1. Se respeta una selección explícita anterior de bilineal/FSR/DLSS. Para activar este comportamiento en una configuración existente, elegir **Automático** y reiniciar.
- **DLSS manual:** deshabilitado en el launcher si la GPU seleccionada no es NVIDIA, faltan las DLL o se usa la ruta 1.2. La comprobación definitiva la realiza `slIsFeatureSupported` sobre el adaptador activo. El menú muestra el resultado/motivo; detectar una NVIDIA por fabricante no basta para activar DLSS.
- **Contador FPS en pantalla:** habilitado por defecto, visible dentro de la imagen incluso con F1 cerrado y a pantalla completa. Muestra FPS nativos y milisegundos por cuadro, actualizados cada medio segundo. Se conserva el checkbox independiente de FPS en la barra de título. Los presents repetidos y la generación de cuadros no incrementan el contador. Los replays deterministas omiten el overlay para conservar las comparaciones de imagen.

Claves nuevas de `xcde.ini`:

```ini
vulkan_api = auto
upscaler = auto
fps_overlay = 1
```

`vulkan_api` también admite `1.2`, `1.3`, `1.4`; `fps_overlay` admite `0/1`. Ambas interfaces conservan estas claves y las de otras versiones.

## Runtime oficial NVIDIA instalado

Fuente: [Streamline SDK 2.14.16, release de NVIDIA-RTX](https://github.com/NVIDIA-RTX/Streamline/releases/tag/v2.14.16), coincidente con las cabeceras del proyecto. Se descargó el SDK x64 de producción y se verificó su SHA-256:

```text
32041e51db0cab709a271b72a352281b690c2abbd669a827809401a5dd23956b
```

Se instalaron en `build/`: `sl.interposer.dll`, `sl.common.dll`, `sl.dlss.dll`, `nvngx_dlss.dll` y `NvLowLatencyVk.dll`. Las cuatro primeras DLL dieron firma Authenticode **Valid**, con firmante **NVIDIA Corporation**. Streamline informa versión 2.14.16.0; `nvngx_dlss.dll`, 310.9.16.0. El manifiesto de origen/hashes queda en `build/dlss-runtime.json`; las licencias originales se conservan en `sdk/sl/licenses/` y `build/dlss-licenses/`. Los binarios permanecen fuera de Git.

Para reinstalar el mismo paquete oficial:

```powershell
python tools/install_dlss.py
```

El instalador verifica el archivo antes de desplegarlo, selecciona exclusivamente las DLL de producción `bin/x64` y rechaza un paquete incompleto, duplicado o con hash incorrecto. El empaquetador local conserva DLL, licencias y manifiesto. Las dependencias Microsoft Visual C++ x64 importadas por Streamline están presentes en System32 en esta máquina; un equipo que no las tenga recibe el error de carga y escalado alternativo.

## Correcciones de integración DLSS

- El chequeo inicial usa la GPU elegida por UUID/índice/env; una RTX presente en otra ranura no activa DLSS sobre una integrada seleccionada.
- Se verifica la firma/publisher del interposer antes de cargarlo. La búsqueda de dependencias usa su propio directorio y System32. Se comprueban todas las funciones del SDK que después se invocan.
- Si el SDK pide cero colas adicionales, los índices graphics/compute usan la cola 0 real; anteriormente se enviaban índices 1 aunque la aplicación solo hubiera creado una cola.
- Se conecta el hook de creación Win32 de superficie, además de los proxies de swapchain/present existentes. Se comprueban límites y capacidades antes de agregar requisitos opcionales.
- Se conservan los vectores de movimiento de valor cero como válidos; se activa exposición automática; los tags de recursos usan la vida útil de evaluación; después del SDK se invalida/restaura el estado de comandos del renderer. Se agrega apagado ordenado de Streamline.
- Los fallos de inicialización, soporte o despacho aparecen en el estado y log, con escalado alternativo; no se informa DLSS activo solo por detectar un nombre RTX.

Referencias de implementación: [guía oficial de integración manual](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.16/docs/ProgrammingGuideManualHooking.md), [guía DLSS](https://github.com/NVIDIA-RTX/Streamline/blob/v2.14.16/docs/ProgrammingGuideDLSS.md).

## Verificación y límites

- Se añadieron pruebas sin GPU para selección 1.2/1.3/1.4, fallback, índices de colas SL sin extras y contador a 30/60/120/144/165 FPS. `test_display_options.exe` se ejecutó y aprobó.
- Se ampliaron los tests de round-trip del INI y los del instalador oficial. La prueba C++ de firma compila, pero Windows bloqueó su ejecución con WinError 4551; la comprobación Authenticode independiente con PowerShell sí se ejecutó y fue válida.
- Los módulos del runtime y el launcher se compilan con `python tools/validate_port.py --build-ui --compile-only`; este modo informa las pruebas nativas como **no ejecutadas**, sin contarlas como aprobadas. La CI usa el modo normal para ejecutarlas cuando el entorno lo permite.
- La máquina disponible tiene una AMD integrada. Falta ejecutar el juego reconstruido en una RTX, comprobar el estado «DLSS Super Resolution disponible», la salida visual a distintas resoluciones y el comportamiento del HUD durante gameplay/FG. No se declara probada la reconstrucción DLSS real ni el juego completo sin ese hardware y los archivos del juego.
