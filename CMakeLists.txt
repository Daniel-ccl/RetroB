cmake_minimum_required(VERSION 3.20)

project(RetroB LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)

set(DIRECTORIO_EJECUCION_JUEGO "${CMAKE_CURRENT_SOURCE_DIR}")
set(DIRECTORIO_EJECUCION_EDITOR "${CMAKE_BINARY_DIR}/tools/level_editor")

set(FUENTES_RETROB
    main.cpp
    ambiente.cpp
    ataque_enjambre.cpp
    avion.cpp
    bomba.cpp
    editor.cpp
    escarabajo.cpp
    hud.cpp
    mapa.cpp
    notificaciones.cpp
    parche_flores.cpp
    pausa_decoracion.cpp
    portal.cpp
    postproceso_enjambre.cpp
    proyectil.cpp
    salud.cpp
    sam.cpp
    saturno.cpp
    ui.cpp
    efectos/destello_lanzamiento.cpp
    efectos/efectos_manager.cpp
    efectos/estela_misil.cpp
    efectos/explosion_sam.cpp
    efectos/trayectoria.cpp
)

set(FUENTES_EDITOR_NIVELES
    Levels_editor/main.cpp
    Levels_editor/ui_textbox.cpp
    ui.cpp
)


add_library(RetroBLevels STATIC
    shared/levels/level_io.cpp
)

target_compile_features(RetroBLevels PUBLIC cxx_std_17)

target_include_directories(RetroBLevels PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/shared"
)

target_link_libraries(RetroBLevels PUBLIC
    PkgConfig::RAYLIB
)

add_executable(RetroB ${FUENTES_RETROB})

target_compile_features(RetroB PRIVATE cxx_std_17)
target_include_directories(RetroB PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(RetroB PRIVATE PkgConfig::RAYLIB RetroBLevels)

set_target_properties(RetroB PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${DIRECTORIO_EJECUCION_JUEGO}"
)

add_executable(EditorNiveles EXCLUDE_FROM_ALL ${FUENTES_EDITOR_NIVELES})

target_compile_features(EditorNiveles PRIVATE cxx_std_17)
target_include_directories(EditorNiveles PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Levels_editor"
)
target_link_libraries(EditorNiveles PRIVATE PkgConfig::RAYLIB RetroBLevels)

set_target_properties(EditorNiveles PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${DIRECTORIO_EJECUCION_EDITOR}"
)

function(enlazar_directorio_fuente origen destino)
    get_filename_component(directorio_padre "${destino}" DIRECTORY)
    file(MAKE_DIRECTORY "${directorio_padre}")

    if(IS_SYMLINK "${destino}")
        file(READ_SYMLINK "${destino}" destino_actual)
        if("${destino_actual}" STREQUAL "${origen}")
            return()
        endif()
        file(REMOVE "${destino}")
    elseif(EXISTS "${destino}")
        message(FATAL_ERROR "${destino} existe y no es un enlace simbolico")
    endif()

    file(CREATE_LINK "${origen}" "${destino}" SYMBOLIC)
endfunction()

enlazar_directorio_fuente(
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
    "${DIRECTORIO_EJECUCION_EDITOR}/assets/fonts"
)

enlazar_directorio_fuente(
    "${CMAKE_CURRENT_SOURCE_DIR}/Levels_editor/levels"
    "${DIRECTORIO_EJECUCION_EDITOR}/levels"
)
