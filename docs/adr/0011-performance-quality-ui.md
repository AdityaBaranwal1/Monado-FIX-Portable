# ADR 0011 — Optimización conservadora y calidad explícita

Estado: implementado, aceptación jugable ampliada pendiente. Fecha: 2026-11-04. Tareas: T259–T261.

Se amplían las cachés/filtrado de estado existentes, conservando claves completas y la vida útil GPU. Los recursos temporales se reutilizan en CPU y los índices auxiliares sustituyen búsquedas lineales sin cambiar su precedencia. No se eliminan barreras, clears ni escrituras observables para reducir llamadas.

La calidad es una acción compartida por ambos frontends, independiente del objetivo FPS. La resolución dinámica usa límites absolutos para que una salida 4K no convierta un perfil de integrada en renderizado 1080p mínimo. No se aplican perfiles al cargar archivos antiguos ni se recortan nuevos efectos por defecto sin aceptación visual.

Se conserva Dear ImGui/SDL y se comparte un tema sencillo. No se añade una segunda plataforma UI, texturas decorativas ni animaciones que consuman recursos mientras se juega.

La medición distingue cuadros nativos, instrumentación detallada y observación ligera. Se exige igualdad visual y comparación reproducible antes de afirmar mejoras de FPS. Clang local sirve para verificar este entorno; no sustituye la decisión del compilador predeterminado de ADR 0007.
