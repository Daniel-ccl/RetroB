#include "editor.h"
#include <cmath>
#include <cstdlib>

static const float SIDEBAR_W = 72.0f;   
static const float SWATCH_W  = 50.0f;
static const float SWATCH_H  = 50.0f;
static const float SWATCH_PAD = 10.0f;
static const float SWATCH_X  = (SIDEBAR_W - SWATCH_W) / 2.0f;  

static Color SwatchColor(int i) {
    switch(i) {
        case 0: return BLUE;
        case 1: return GREEN;
        case 2: return YELLOW;
        case 3: return ORANGE;
        case 4: return RED;
        default: return DARKGRAY;   
    }
}


static float GridStep(int screenHeight, const Mapa& mapa) {
    return (float)screenHeight / mapa.GetCeldas();
}

static float GridOffsetX() {
    return SIDEBAR_W;
}

Rectangle Editor::SwatchRect(int i, int screenWidth, int screenHeight) const {
    float startY = 80.0f + i * (SWATCH_H + SWATCH_PAD);
    return { SWATCH_X, startY, SWATCH_W, SWATCH_H };
}

bool Editor::MouseAGrid(Vector2 mousePos, int screenWidth, int screenHeight,
                        const Mapa& mapa, int& outX, int& outZ) const {
    int   celdas  = mapa.GetCeldas();
    float paso    = GridStep(screenHeight, mapa);
    float offX    = GridOffsetX();

    if (mousePos.x < offX || mousePos.x >= offX + paso * celdas) return false;
    if (mousePos.y < 0    || mousePos.y >= paso * celdas)         return false;

    outX = (int)((mousePos.x - offX) / paso);
    outZ = (int)(mousePos.y / paso);
    return (outX >= 0 && outX < celdas && outZ >= 0 && outZ < celdas);
}

Editor::Editor() {}

void Editor::PintarCelda(Mapa& mapa, int gridX, int gridZ,
                          Vector3 portalPlano, Vector3 saturnoPos) const {
    int celdas = mapa.GetCeldas();
    int pPlanoX, pPlanoZ, satX, satZ;
    mapa.PosicionAIndice(portalPlano, pPlanoX, pPlanoZ);
    mapa.PosicionAIndice(saturnoPos,  satX,    satZ);

    int half = brushSize / 2;
    for (int dx = -half; dx <= half; dx++) {
        for (int dz = -half; dz <= half; dz++) {
            int cx = gridX + dx;
            int cz = gridZ + dz;
            if (cx < 0 || cx >= celdas || cz < 0 || cz >= celdas) continue;

            bool zonaSegura = (abs(cx - pPlanoX) <= 1 && abs(cz - pPlanoZ) <= 1) ||
                              (abs(cx - satX)    <= 2 && abs(cz - satZ)    <= 2);
            if (zonaSegura) continue;

            mapa.SetAltura(cx, cz, modoErase ? 0 : alturaActiva);
        }
    }
}

bool Editor::Actualizar(Mapa& mapa, Vector2 mousePos, int screenWidth, int screenHeight,
                         Vector3 portalPlano, Vector3 saturnoPos) {
    float scroll = GetMouseWheelMove();
    if (scroll > 0) brushSize = std::min(brushSize + 1, 5);
    if (scroll < 0) brushSize = std::max(brushSize - 1, 1);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int i = 0; i < N_SWATCHES; i++) {
            if (CheckCollisionPointRec(mousePos, SwatchRect(i, screenWidth, screenHeight))) {
                if (i < 5) { alturaActiva = i + 1; modoErase = false; }
                else        { modoErase = true; }
                return false;
            }
        }
        Rectangle playBtn = { (float)screenWidth - 110, 10.0f, 100.0f, 36.0f };
        if (CheckCollisionPointRec(mousePos, playBtn)) return true;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        int gx, gz;
        if (MouseAGrid(mousePos, screenWidth, screenHeight, mapa, gx, gz))
            PintarCelda(mapa, gx, gz, portalPlano, saturnoPos);
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        int gx, gz;
        if (MouseAGrid(mousePos, screenWidth, screenHeight, mapa, gx, gz)) {
            bool prev = modoErase;
            modoErase = true;
            PintarCelda(mapa, gx, gz, portalPlano, saturnoPos);
            modoErase = prev;
        }
    }

    return false;
}

void Editor::Dibujar(Mapa& mapa, int screenWidth, int screenHeight,
                     Vector3 portalPlano, Vector3 saturnoPos) const {
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){10, 10, 20, 255});

    int   celdas = mapa.GetCeldas();
    float paso   = GridStep(screenHeight, mapa);
    float offX   = GridOffsetX();

    int pPlanoX, pPlanoZ, satX, satZ;
    mapa.PosicionAIndice(portalPlano, pPlanoX, pPlanoZ);
    mapa.PosicionAIndice(saturnoPos,  satX,    satZ);

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            Rectangle rect = { offX + x * paso, z * paso, paso, paso };
            int altura = mapa.GetAltura(x, z);

            Color fill = (Color){20, 20, 30, 255};
            if (altura == 1) fill = BLUE;
            if (altura == 2) fill = GREEN;
            if (altura == 3) fill = YELLOW;
            if (altura == 4) fill = ORANGE;
            if (altura == 5) fill = RED;

            if (abs(x - pPlanoX) <= 1 && abs(z - pPlanoZ) <= 1) fill = DARKGREEN;
            if (abs(x - satX)    <= 2 && abs(z - satZ)    <= 2) fill = PURPLE;

            DrawRectangleRec(rect, fill);
            DrawRectangleLinesEx(rect, 1.0f, (Color){0, 255, 255, 60});

            if (x == pPlanoX && z == pPlanoZ)
                DrawCircle(rect.x + paso/2, rect.y + paso/2, paso/4, YELLOW);
            if (x == satX && z == satZ)
                DrawCircle(rect.x + paso/2, rect.y + paso/2, paso/4, PINK);
        }
    }

    Vector2 mousePos = GetMousePosition();
    int hx, hz;
    if (MouseAGrid(mousePos, screenWidth, screenHeight, mapa, hx, hz)) {
        int half = brushSize / 2;
        for (int dx = -half; dx <= half; dx++) {
            for (int dz = -half; dz <= half; dz++) {
                int cx = hx + dx, cz = hz + dz;
                if (cx < 0 || cx >= celdas || cz < 0 || cz >= celdas) continue;
                Rectangle r = { offX + cx * paso, cz * paso, paso, paso };
                Color hc = modoErase ? (Color){255, 50, 50, 80} : (Color){255, 255, 255, 60};
                DrawRectangleRec(r, hc);
                DrawRectangleLinesEx(r, 2.0f, modoErase ? RED : WHITE);
            }
        }
    }

    DrawRectangle(0, 0, (int)SIDEBAR_W, screenHeight, (Color){15, 15, 25, 255});
    DrawLine((int)SIDEBAR_W, 0, (int)SIDEBAR_W, screenHeight, (Color){0, 255, 255, 80});

    DrawText("H", (int)(SIDEBAR_W/2) - 5, 12, 18, LIGHTGRAY);
    DrawText("E", (int)(SIDEBAR_W/2) - 5, 30, 18, LIGHTGRAY);
    DrawLine(8, 58, (int)SIDEBAR_W - 8, 58, (Color){0,255,255,60});

    for (int i = 0; i < N_SWATCHES; i++) {
        Rectangle sr  = SwatchRect(i, screenWidth, screenHeight);
        Color     col = SwatchColor(i);

        DrawRectangleRec(sr, col);

        bool selected = (i < 5) ? (!modoErase && alturaActiva == i + 1) : modoErase;
        if (selected) {
            DrawRectangleLinesEx(sr, 3.0f, WHITE);
            DrawRectangle((int)sr.x - 6, (int)sr.y, 4, (int)sr.height, WHITE);
        } else {
            DrawRectangleLinesEx(sr, 1.0f, (Color){150, 150, 150, 100});
        }

        const char* label = (i < 5) ? TextFormat("%d", i + 1) : "E";
        DrawText(label, (int)(sr.x + sr.width/2 - 5), (int)(sr.y + sr.height/2 - 8), 16, BLACK);
    }

    float brushY = 80.0f + N_SWATCHES * (SWATCH_H + SWATCH_PAD) + 16.0f;
    DrawLine(8, (int)brushY - 8, (int)SIDEBAR_W - 8, (int)brushY - 8, (Color){0,255,255,60});
    DrawText("KEKW", (int)(SIDEBAR_W/2) - 10, (int)brushY, 14, LIGHTGRAY);
    DrawText(TextFormat("%d", brushSize), (int)(SIDEBAR_W/2) - 5, (int)brushY + 18, 20, WHITE);
    DrawText("^v", (int)(SIDEBAR_W/2) - 8, (int)brushY + 42, 12, (Color){0,255,255,180});

    Rectangle playBtn = { (float)screenWidth - 110, 10.0f, 100.0f, 36.0f };
    DrawRectangleRec(playBtn, DARKGREEN);
    DrawRectangleLinesEx(playBtn, 2.0f, LIME);
    DrawText("PLAY", (int)playBtn.x + 22, (int)playBtn.y + 9, 20, WHITE);

    DrawText("puto el que lo lea",
             (int)SIDEBAR_W + 8, screenHeight - 20, 13, (Color){120, 120, 140, 255});
}
