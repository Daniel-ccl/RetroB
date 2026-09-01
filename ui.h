#pragma once

#include "raylib.h"

namespace UI {

bool Inicializar(const char* rutaFuente, int tamanoBase = 48);
void Finalizar();

Font ObtenerFuente();

Vector2 MedirTexto(
    const char* texto,
    float tamano,
    float espaciado = 1.0f
);

void DibujarTexto(
    const char* texto,
    Vector2 posicion,
    float tamano,
    Color color,
    float espaciado = 1.0f
);

void DibujarTextoCentrado(
    const char* texto,
    Rectangle area,
    float tamano,
    Color color,
    float espaciado = 1.0f
);

}
