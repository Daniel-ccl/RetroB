# ADR-001: Módulo compartido de niveles

- Estado: Aceptado
- Fecha: 2026-09-02
- Proyecto: RetroB

## Contexto

RetroB contiene dos ejecutables que trabajan con niveles:

- `RetroB`, el juego principal.
- `EditorNiveles`, la herramienta de creación de niveles.

Actualmente cada ejecutable utiliza su propia copia del modelo de datos
y del serializador.

El juego utiliza:

- `level_data.h`
- `level_io.h`
- `level_io.cpp`

El editor utiliza:

- `Levels_editor/level_data.h`
- `Levels_editor/level_io.h`
- `Levels_editor/level_io.cpp`

Aunque CMake incluye el directorio raíz para `EditorNiveles`, las
inclusiones con comillas buscan primero en el directorio del archivo que
las realiza. Por ello, `Levels_editor/main.cpp` encuentra las copias
locales del editor antes que los archivos de la raíz.

En el punto de partida `636a6dd`, las dos copias del modelo y del
serializador son idénticas. Sin embargo, mantenerlas separadas permite
que diverjan al añadir nuevas entidades o propiedades.

Este problema ya apareció al incorporar la serialización de `PORTAL` y
`PLANET`.

Los futuros módulos —editor de enemigos, armas, efectos y pruebas de
sinergia— necesitarán compartir estructuras, serialización y validación
con el juego.

## Inventario inicial verificado

Commit de referencia:

```text
636a6dd Add 3D player ship model
```

Archivos idénticos:

| Archivo raíz | Copia del editor | SHA-256 |
|---|---|---|
| `level_data.h` | `Levels_editor/level_data.h` | `e8da7e34efe7fd6f68a4c3176a3c5d5172988b3af3d6b5cc75d4c798a8c4b110` |
| `level_io.h` | `Levels_editor/level_io.h` | `9533d5a12039d80a8b2615c631a3575c944f56a59f7e10559874f598195ccdcf` |
| `level_io.cpp` | `Levels_editor/level_io.cpp` | `55a681b5fc0076b3e2785fdef07e18568738eabd6730e35570f7e33804f9ade5` |

Dependencias directas del juego:

- `main.cpp` incluye `level_data.h` y `level_io.h`.
- `main.cpp` utiliza `LevelData`, `LevelIO` y `Objective`.
- `sam.h` incluye `level_data.h`.
- `sam.cpp` utiliza `EnemyConfig`.
- CMake compila `level_io.cpp` dentro de `RetroB`.

Dependencias directas del editor:

- `Levels_editor/main.cpp` incluye sus copias locales de
  `level_data.h` y `level_io.h`.
- El editor utiliza `LevelData`, `LevelIO`, `PortalSpawn` y
  `PlanetSpawn`.
- CMake compila `Levels_editor/level_io.cpp` dentro de
  `EditorNiveles`.

Conclusión del inventario:

> El juego y el editor comparten actualmente el formato conceptual,
> pero no comparten físicamente su modelo de datos ni su implementación
> de serialización.

## Decisión

Crear un único módulo compartido para los datos de nivel:

```text
shared/
└── levels/
    ├── level_data.h
    ├── level_io.h
    └── level_io.cpp
``

El juego y el editor utilizarán exactamente los mismos archivos.

CMake expondrá el módulo mediante una biblioteca interna llamada
`RetroBLevels`.

```text
RetroB ──────────┐
                 ├── RetroBLevels
EditorNiveles ───┘
```

El target `RetroBLevels` será responsable de compilar
`shared/levels/level_io.cpp` y de exponer `shared/` como directorio
público de inclusión. Los consumidores incluirán los archivos mediante
`levels/level_data.h` y `levels/level_io.h`.

## Límites del módulo

El módulo será responsable de:

- Las estructuras de datos de nivel.
- Cargar archivos `.lvl`.
- Guardar archivos `.lvl`.
- Posteriormente, validar datos de nivel.

El módulo no será responsable de:

- Renderizado.
- Entrada del jugador.
- Interfaz gráfica.
- Estado interno del editor.
- Crear enemigos o planetas durante la ejecución del juego.

## Compatibilidad

Esta primera refactorización no cambiará:

- El formato de los archivos `.lvl`.
- Los nombres de los campos existentes.
- Los valores predeterminados.
- La lógica de carga o guardado.
- El comportamiento del juego o del editor.

Los tipos `Vector3` de Raylib se conservarán temporalmente para reducir
el alcance del cambio. Eliminar esa dependencia será una decisión
posterior e independiente.

## Condiciones de aceptación

La refactorización estará completa cuando:

1. Exista una sola declaración de `LevelData`.
2. Exista una sola implementación de `LevelIO`.
3. `RetroB` y `EditorNiveles` enlacen el mismo módulo.
4. Ambos ejecutables compilen correctamente.
5. Los niveles existentes continúen cargando.
6. Un nivel pueda guardarse y volver a cargarse sin perder:
   - Terreno.
   - Posición del jugador.
   - Enemigos.
   - Objetivos.
   - Portales.
   - Planetas.
7. No cambie el formato observable de los archivos `.lvl`.

## Plan de implementación

1. Registrar el estado inicial.
2. Crear `shared/levels/`.
3. Mover el modelo de datos y el serializador raíz al módulo.
4. Crear el target `RetroBLevels` en CMake.
5. Enlazar el juego con `RetroBLevels`.
6. Enlazar el editor con `RetroBLevels`.
7. Comprobar que ambos ejecutables compilan usando el módulo.
8. Eliminar las copias duplicadas de `Levels_editor/`.
9. Compilar nuevamente ambos ejecutables.
10. Ejecutar una prueba de carga, guardado y recarga.
11. Comparar el resultado y documentar la verificación.

## Riesgos

- Romper rutas de inclusión existentes.
- Compilar accidentalmente dos definiciones de `LevelIO`.
- Cambiar el formato `.lvl` durante una refactorización estructural.
- Mezclar cambios del nivel de prueba con los cambios arquitectónicos.
- Eliminar las copias del editor antes de comprobar el módulo común.

## Estrategia de seguridad

Las copias duplicadas del editor permanecerán intactas hasta que ambos
ejecutables compilen correctamente con `RetroBLevels`.

Cada etapa se verificará por separado. No se combinarán cambios de
comportamiento con el movimiento arquitectónico.

El archivo local `Levels_editor/levels/nivel_nuevo.lvl` permanecerá
fuera de esta refactorización.

## Control del alcance

Esta fase solamente consolidará el código existente.

La validación avanzada, el versionado del formato y los nuevos tipos de
entidades se implementarán en fases posteriores.

## Verificación

La refactorización fue verificada el 2 de septiembre de 2026.

Resultados:

- `RetroBLevels` compila correctamente.
- `RetroB` enlaza `RetroBLevels` y compila sin su antiguo serializador.
- `EditorNiveles` enlaza `RetroBLevels` y compila sin su copia local.
- El juego carga correctamente un nivel existente.
- El editor guarda y vuelve a cargar correctamente el nivel probado.
- El terreno, los portales y los planetas permanecen después de la recarga.
- Solo permanecen los archivos ubicados en `shared/levels/`.
- `git diff --check` no reporta errores.

El archivo `Levels_editor/levels/nivel_nuevo.lvl` conserva cambios locales
independientes y no forma parte de esta refactorización.
