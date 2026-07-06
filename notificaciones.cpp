#include "notificaciones.h"
#include <algorithm>

void Notificaciones::Empujar(const std::string& mensaje, TipoNotificacion tipo) {
    Entrada e;
    e.mensaje = mensaje;
    e.tipo = tipo;
    e.tiempoVida = DURACION_TOTAL;
    e.tiempoVidaMax = DURACION_TOTAL;
    entradas.push_back(e); 

    if ((int)entradas.size() > MAX_VISIBLES) {
        entradas.erase(entradas.begin());
    }
}

void Notificaciones::Actualizar(float dt) {
    for (auto& e : entradas) {
        e.tiempoVida -= dt;
    }
    entradas.erase(std::remove_if(entradas.begin(), entradas.end(),
        [](const Entrada& e) { return e.tiempoVida <= 0.0f; }), entradas.end());
}

void Notificaciones::Dibujar(int screenWidth, int screenHeight) const {
    if (entradas.empty()) return;

    float anchoBox = 420.0f;
    float altoBox  = 36.0f;
    float espacio  = 8.0f;
    float margenDerecho = 20.0f;
    float yInicial = 140.0f; 

    float x = screenWidth - anchoBox - margenDerecho;

    float y = yInicial;
    for (auto it = entradas.rbegin(); it != entradas.rend(); ++it) {
        const Entrada& e = *it;

        Color colorBase;
        switch (e.tipo) {
            case NOTIF_EXITO:   colorBase = (Color){0, 255, 140, 255};  break;
            case NOTIF_PELIGRO: colorBase = (Color){255, 60, 60, 255};  break;
            default:             colorBase = (Color){0, 220, 255, 255}; break; 
        }

        float alphaFactor = 1.0f;
        if (e.tiempoVida < DURACION_FADE) {
            alphaFactor = e.tiempoVida / DURACION_FADE;
            if (alphaFactor < 0.0f) alphaFactor = 0.0f;
        }
        unsigned char alpha = (unsigned char)(255 * alphaFactor);

        Color colorBorde = colorBase; colorBorde.a = alpha;
        Color colorTexto = WHITE;     colorTexto.a = alpha;
        Color colorFondo = (Color){10, 10, 18, (unsigned char)(180 * alphaFactor)};

        Rectangle r = { x, y, anchoBox, altoBox };
        DrawRectangleRec(r, colorFondo);
        DrawRectangleLinesEx(r, 1.5f, colorBorde);

        DrawText(e.mensaje.c_str(), (int)(x + 14), (int)(y + altoBox/2.0f - 8), 16, colorTexto);

        y += altoBox + espacio;
    }
}
