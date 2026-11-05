# Optimización de rendimiento — cambios aplicados a todas las GPU

1. **Hilos de compilación de shaders** (`runtime/resource_policy.h`): T251 limitaba a 2 en iGPU / perfil "Rendimiento". Los draws
   esperan a sus pipelines, así que cada zona nueva se atascaba. Ahora cpus-4 (cpus-2 hasta 6 CPU) con tope de RAM.
   Override: `set XCDE_SHADER_WORKERS=<n>`.
2. **Presupuestos de librerías/pipelines** (`exec.c`): el perfil "Rendimiento" ya no los divide a la mitad (evicciones y
   recompilaciones constantes). En iGPU tampoco se recorta un 25 % por "presión de heap" (el heap UMA siempre parece lleno).
3. **Feedback directo del render target por defecto en todas las GPU** (V4: 23.8 → 27.7 fps en batalla; T252 lo había apagado).
   `set LARECOMP_FEEDBACK_DIRECT=0` vuelve a los snapshots si hay texturas azules o parpadeo.
4. **Barrera ALL_COMMANDS→HOST** solo en submits con lectura GPU→CPU (antes en todos).
5. **Import del heap invitado**: se prefiere memoria coherente pero ya no se rechaza si no existe (si fallaba, todos los shaders
   con memoria global copiaban el heap entero).

Tests: `tests/native/test_port_core.c` actualizado para la política de hilos.

6. **Cierre en Intel HD (`FATAL [gmem] heap 3220 MiB exceeds maxStorageBufferRange 1023 MiB`)**: el juego se cerraba al primer shader
   con memoria global. Ahora es un aviso y los shaders ven solo la primera ventana del heap (límite del driver). No es un
   direccionamiento por ventanas completo: puede haber fallos gráficos en efectos de cómputo, pero ya no se cierra.
