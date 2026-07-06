#include "hud.h"
#include "raymath.h"
#include <cstdio>

static void DibujarBarraSegmentada(float x, float y, float ancho, float alto,
                                    int segmentos, float fraccion, Color color) {
    float espacioEntreSegmentos = 3.0f;
    float anchoSegmento = (ancho - espacioEntreSegmentos * (segmentos - 1)) / (float)segmentos;

    int segmentosLlenos = (int)roundf(Clamp(fraccion, 0.0f, 1.0f) * segmentos);

    for (int i = 0; i < segmentos; i++) {
        float sx = x + i * (anchoSegmento + espacioEntreSegmentos);
        Rectangle r = { sx, y, anchoSegmento, alto };

        if (i < segmentosLlenos) {
            DrawRectangleRec(r, color);
        }
        Color colorBorde = color; colorBorde.a = 90;
        DrawRectangleLinesEx(r, 1.0f, colorBorde);
    }
}

namespace Hud {

void DibujarPanelPrincipal(int screenWidth, int screenHeight, float pctSalud, float fraccionBoost,
                            bool misilListo, bool municionInfinita, int numMisilesActivos) {
    float margen = 20.0f;
    float anchoPanel = 280.0f;
    float x = screenWidth - anchoPanel - margen;
    float y = margen;

    Color colorMarco = (Color){0, 220, 255, 180}; 

    float altoPanelTotal = 100.0f;
    Rectangle marcoPanel = { x - 10, y - 10, anchoPanel + 20, altoPanelTotal };
    DrawRectangleLinesEx(marcoPanel, 1.0f, (Color){colorMarco.r, colorMarco.g, colorMarco.b, 60});

    Color colorHP = pctSalud > 0.5f ? (Color){0, 255, 140, 255}
                   : pctSalud > 0.2f ? (Color){255, 200, 0, 255}
                                      : (Color){255, 60, 60, 255};
    DrawText("HP", (int)x, (int)y, 14, colorMarco);
    DibujarBarraSegmentada(x + 40, y, anchoPanel - 40, 16, 20, pctSalud, colorHP);

    char bufHP[16];
    snprintf(bufHP, sizeof(bufHP), "%d%%", (int)(pctSalud * 100.0f));
    DrawText(bufHP, (int)(x + anchoPanel - 36), (int)(y + 18), 12, colorHP);

    float yBoost = y + 34.0f;
    Color colorBoost = (Color){200, 0, 255, 255}; 
    DrawText("BOOST", (int)x, (int)yBoost, 14, colorMarco);
    DibujarBarraSegmentada(x + 60, yBoost, anchoPanel - 60, 16, 14, fraccionBoost, colorBoost);

    float yMisil = y + 68.0f;
    Color colorMisil = misilListo ? (Color){0, 220, 255, 255} : (Color){90, 90, 100, 255};
    DrawText("MISIL", (int)x, (int)yMisil, 14, colorMarco);

    char bufMisil[32];
    if (municionInfinita) {
        snprintf(bufMisil, sizeof(bufMisil), "%s", misilListo ? "[ LISTO ]  oo" : "[ RECARGANDO ]  oo");
    } else {
        snprintf(bufMisil, sizeof(bufMisil), "%s", misilListo ? "[ LISTO ]" : "[ RECARGANDO ]");
    }
    DrawText(bufMisil, (int)(x + 60), (int)yMisil, 14, colorMisil);

    if (numMisilesActivos > 0) {
        char bufActivos[16];
        snprintf(bufActivos, sizeof(bufActivos), "(%d en vuelo)", numMisilesActivos);
        DrawText(bufActivos, (int)(x + 60), (int)(yMisil + 16), 11, (Color){150, 150, 160, 255});
    }
}

}
