#include "raylib.h"
#include "level_data.h"
#include "level_io.h"
#include "ui_textbox.h"
#include <vector>
#include <string>
#include <cstdlib>

enum ModoEditor { MODO_TERRENO, MODO_PORTAL, MODO_PLANETA };

static const int SIDEBAR_W  = 140;
static const int TOPBAR_H   = 50;
static const int N_SWATCHES = 6;

static Color SwatchColor(int i) {
    switch (i) {
        case 0: return BLUE;
        case 1: return GREEN;
        case 2: return YELLOW;
        case 3: return ORANGE;
        case 4: return RED;
        default: return DARKGRAY;
    }
}

static Rectangle ModoBtnRect(int i) {
    float y = TOPBAR_H + 10.0f + i * 38.0f;
    return { 10.0f, y, (float)(SIDEBAR_W - 20), 32.0f };
}

static Rectangle SwatchRect(int i) {
    float y = TOPBAR_H + 140.0f + i * 58.0f;
    return { 20.0f, y, 60.0f, 50.0f };
}

static Rectangle RadioRect() {
    return { 20.0f, TOPBAR_H + 140.0f + N_SWATCHES * 58.0f + 30.0f, (float)(SIDEBAR_W - 40), 28.0f };
}

static LevelData NivelNuevo() {
    LevelData l;
    l.nombre = "nivel_nuevo";
    l.tamano = 400;
    l.celdas = 40;
    l.alturas.assign(l.celdas, std::vector<int>(l.celdas, 0));
    l.jugadorSpawn    = {0.0f, 0.0f, 0.0f};
    l.jugadorRotacion = 0.0f;
    return l;
}

static bool MouseAGrid(Vector2 m, int screenH, const LevelData& lvl, int& outX, int& outZ) {
    float paso = (screenH - TOPBAR_H) / (float)lvl.celdas;
    float offX = SIDEBAR_W;
    float offY = TOPBAR_H;

    if (m.x < offX || m.x >= offX + paso * lvl.celdas) return false;
    if (m.y < offY || m.y >= offY + paso * lvl.celdas) return false;

    outX = (int)((m.x - offX) / paso);
    outZ = (int)((m.y - offY) / paso);
    return outX >= 0 && outX < lvl.celdas && outZ >= 0 && outZ < lvl.celdas;
}

static Vector3 GridAMundo(const LevelData& lvl, int gx, int gz) {
    float worldPaso = lvl.tamano / (float)lvl.celdas;
    float offset    = lvl.tamano / 2.0f;
    float wx = (gx * worldPaso) - offset + worldPaso / 2.0f;
    float wz = (gz * worldPaso) - offset + worldPaso / 2.0f;
    return { wx, 0.0f, wz };
}

static bool DentroRadio(Vector3 a, Vector3 b, float radio) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return (dx * dx + dz * dz) <= (radio * radio);
}

static bool EnZonaReservada(const LevelData& lvl, int gx, int gz) {
    Vector3 centro = GridAMundo(lvl, gx, gz);
    for (const auto& p : lvl.portales)
        if (DentroRadio(p.posicion, centro, p.radius)) return true;
    for (const auto& p : lvl.planetas)
        if (DentroRadio(p.posicion, centro, p.radius)) return true;
    return false;
}

static void LimpiarZona(LevelData& lvl, Vector3 centro, float radio) {
    for (int x = 0; x < lvl.celdas; x++) {
        for (int z = 0; z < lvl.celdas; z++) {
            Vector3 p = GridAMundo(lvl, x, z);
            if (DentroRadio(centro, p, radio)) lvl.alturas[x][z] = 0;
        }
    }
}

static void Pintar(LevelData& lvl, int gx, int gz, int brushSize, int altura, bool erase) {
    int half = brushSize / 2;
    for (int dx = -half; dx <= half; dx++) {
        for (int dz = -half; dz <= half; dz++) {
            int cx = gx + dx;
            int cz = gz + dz;
            if (cx < 0 || cx >= lvl.celdas || cz < 0 || cz >= lvl.celdas) continue;
            if (EnZonaReservada(lvl, cx, cz)) continue;
            lvl.alturas[cx][cz] = erase ? 0 : altura;
        }
    }
}

static Vector2 MundoAPixel(const LevelData& lvl, Vector3 pos, int screenHeight) {
    float scale  = (screenHeight - TOPBAR_H) / (float)lvl.tamano;
    float offset = lvl.tamano / 2.0f;
    return { SIDEBAR_W + (pos.x + offset) * scale, TOPBAR_H + (pos.z + offset) * scale };
}

static float MundoAPixelRadio(const LevelData& lvl, float radio, int screenHeight) {
    float scale = (screenHeight - TOPBAR_H) / (float)lvl.tamano;
    return radio * scale;
}

static void DibujarBoton(Rectangle r, const char* texto, Vector2 mousePos) {
    bool hover = CheckCollisionPointRec(mousePos, r);
    DrawRectangleRec(r, hover ? (Color){60, 60, 80, 255} : (Color){35, 35, 50, 255});
    DrawRectangleLinesEx(r, 1.0f, (Color){0, 220, 255, 180});
    int tw = MeasureText(texto, 14);
    DrawText(texto, (int)(r.x + (r.width - tw) / 2.0f), (int)(r.y + (r.height - 14) / 2.0f), 14, WHITE);
}

int main() {
    int screenWidth  = 1280;
    int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "RetroB - editor de niveles");
    SetTargetFPS(60);

    LevelData nivel = NivelNuevo();
    ModoEditor modo = MODO_TERRENO;

    int  brushSize    = 1;
    int  alturaActiva = 1;
    bool modoErase    = false;

    TextBox tbNombre       = { {70.0f,  TOPBAR_H / 2.0f + 4, 200, 28}, nivel.nombre, false, 40 };
    TextBox tbArchivo      = { {380.0f, TOPBAR_H / 2.0f + 4, 240, 28}, "levels/nivel_nuevo.lvl", false, 80 };
    TextBox tbRadioPortal  = { RadioRect(), "5",  false, 6 };
    TextBox tbRadioPlaneta = { RadioRect(), "20", false, 6 };

    std::string statusMsg;
    float statusTimer = 0.0f;

    Rectangle btnGuardar = {670, TOPBAR_H / 2.0f + 4, 90, 28};
    Rectangle btnCargar  = {770, TOPBAR_H / 2.0f + 4, 90, 28};
    Rectangle btnNuevo   = {870, TOPBAR_H / 2.0f + 4, 90, 28};

    while (!WindowShouldClose()) {
        float   dt       = GetFrameTime();
        Vector2 mousePos = GetMousePosition();
        bool    click    = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        bool    clickDer = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);

        if (click) {
            TextBoxClick(tbNombre, mousePos);
            if (!tbNombre.activo) TextBoxClick(tbArchivo, mousePos);
            bool otroActivo = tbNombre.activo || tbArchivo.activo;

            if (!otroActivo && modo == MODO_PORTAL)  TextBoxClick(tbRadioPortal, mousePos);
            else tbRadioPortal.activo = false;

            if (!otroActivo && modo == MODO_PLANETA) TextBoxClick(tbRadioPlaneta, mousePos);
            else tbRadioPlaneta.activo = false;
        }
        TextBoxTeclado(tbNombre);
        TextBoxTeclado(tbArchivo);
        TextBoxTeclado(tbRadioPortal);
        TextBoxTeclado(tbRadioPlaneta);

        bool escribiendo = tbNombre.activo || tbArchivo.activo || tbRadioPortal.activo || tbRadioPlaneta.activo;

        if (!escribiendo) {
            float scroll = GetMouseWheelMove();
            if (modo == MODO_TERRENO) {
                if (scroll > 0) brushSize = (brushSize < 5) ? brushSize + 1 : 5;
                if (scroll < 0) brushSize = (brushSize > 1) ? brushSize - 1 : 1;
            }

            if (click) {
                bool tocoUI = false;

                for (int i = 0; i < 3; i++) {
                    if (CheckCollisionPointRec(mousePos, ModoBtnRect(i))) {
                        modo = (ModoEditor)i;
                        tbRadioPortal.activo  = false;
                        tbRadioPlaneta.activo = false;
                        tocoUI = true;
                    }
                }

                if (modo == MODO_TERRENO) {
                    for (int i = 0; i < N_SWATCHES; i++) {
                        if (CheckCollisionPointRec(mousePos, SwatchRect(i))) {
                            if (i < 5) { alturaActiva = i + 1; modoErase = false; }
                            else        modoErase = true;
                            tocoUI = true;
                        }
                    }
                } else {
                    if (CheckCollisionPointRec(mousePos, RadioRect())) tocoUI = true;
                }

                if (!tocoUI) {
                    int gx, gz;
                    if (MouseAGrid(mousePos, screenHeight, nivel, gx, gz)) {
                        if (modo == MODO_TERRENO) {
                            Pintar(nivel, gx, gz, brushSize, alturaActiva, modoErase);
                        } else if (modo == MODO_PORTAL) {
                            Vector3 centro = GridAMundo(nivel, gx, gz);
                            float radio = (float)atof(tbRadioPortal.text.c_str());
                            if (radio <= 0.0f) radio = 5.0f;
                            PortalSpawn p; p.posicion = centro; p.radius = radio;
                            nivel.portales.push_back(p);
                            LimpiarZona(nivel, centro, radio);
                        } else if (modo == MODO_PLANETA) {
                            Vector3 centro = GridAMundo(nivel, gx, gz);
                            float radio = (float)atof(tbRadioPlaneta.text.c_str());
                            if (radio <= 0.0f) radio = 20.0f;
                            PlanetSpawn p; p.posicion = centro; p.radius = radio;
                            nivel.planetas.push_back(p);
                            LimpiarZona(nivel, centro, radio);
                        }
                    }
                }
            }

            if (modo == MODO_TERRENO && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                int gx, gz;
                if (MouseAGrid(mousePos, screenHeight, nivel, gx, gz))
                    Pintar(nivel, gx, gz, brushSize, alturaActiva, modoErase);
            }

            if (clickDer) {
                int gx, gz;
                if (MouseAGrid(mousePos, screenHeight, nivel, gx, gz)) {
                    Vector3 punto = GridAMundo(nivel, gx, gz);
                    if (modo == MODO_PORTAL) {
                        for (size_t i = 0; i < nivel.portales.size(); i++) {
                            if (DentroRadio(nivel.portales[i].posicion, punto, nivel.portales[i].radius)) {
                                nivel.portales.erase(nivel.portales.begin() + i);
                                break;
                            }
                        }
                    } else if (modo == MODO_PLANETA) {
                        for (size_t i = 0; i < nivel.planetas.size(); i++) {
                            if (DentroRadio(nivel.planetas[i].posicion, punto, nivel.planetas[i].radius)) {
                                nivel.planetas.erase(nivel.planetas.begin() + i);
                                break;
                            }
                        }
                    }
                }
            }
        }

        bool clickGuardar = click && CheckCollisionPointRec(mousePos, btnGuardar);
        bool clickCargar  = click && CheckCollisionPointRec(mousePos, btnCargar);
        bool clickNuevo   = click && CheckCollisionPointRec(mousePos, btnNuevo);

        if (clickGuardar) {
            nivel.nombre = tbNombre.text;
            bool ok = LevelIO::Guardar(tbArchivo.text, nivel);
            statusMsg   = ok ? "guardado ok" : "error (existe la carpeta?)";
            statusTimer = 2.5f;
        }
        if (clickCargar) {
            LevelData cargado;
            bool ok = LevelIO::Cargar(tbArchivo.text, cargado);
            if (ok) {
                nivel = cargado;
                tbNombre.text = nivel.nombre;
                statusMsg = "cargado ok";
            } else {
                statusMsg = "error al cargar";
            }
            statusTimer = 2.5f;
        }
        if (clickNuevo) {
            nivel = NivelNuevo();
            tbNombre.text = nivel.nombre;
            statusMsg   = "nivel nuevo";
            statusTimer = 2.5f;
        }

        if (statusTimer > 0.0f) statusTimer -= dt;

        BeginDrawing();
        ClearBackground((Color){10, 10, 18, 255});

        float paso = (screenHeight - TOPBAR_H) / (float)nivel.celdas;
        for (int x = 0; x < nivel.celdas; x++) {
            for (int z = 0; z < nivel.celdas; z++) {
                Rectangle r = { SIDEBAR_W + x * paso, TOPBAR_H + z * paso, paso, paso };
                int h = nivel.alturas[x][z];
                Color fill = (Color){20, 20, 30, 255};
                if (h == 1) fill = BLUE;
                if (h == 2) fill = GREEN;
                if (h == 3) fill = YELLOW;
                if (h == 4) fill = ORANGE;
                if (h == 5) fill = RED;
                DrawRectangleRec(r, fill);
                DrawRectangleLinesEx(r, 1.0f, (Color){0, 255, 255, 50});
            }
        }

        for (const auto& p : nivel.portales) {
            Vector2 c = MundoAPixel(nivel, p.posicion, screenHeight);
            float   r = MundoAPixelRadio(nivel, p.radius, screenHeight);
            DrawCircle((int)c.x, (int)c.y, r, Fade(DARKGREEN, 0.35f));
            DrawCircleLines((int)c.x, (int)c.y, r, YELLOW);
            DrawCircle((int)c.x, (int)c.y, 4.0f, YELLOW);
        }
        for (const auto& p : nivel.planetas) {
            Vector2 c = MundoAPixel(nivel, p.posicion, screenHeight);
            float   r = MundoAPixelRadio(nivel, p.radius, screenHeight);
            DrawCircle((int)c.x, (int)c.y, r, Fade(PURPLE, 0.35f));
            DrawCircleLines((int)c.x, (int)c.y, r, PINK);
            DrawCircle((int)c.x, (int)c.y, 4.0f, PINK);
        }

        DrawRectangle(0, TOPBAR_H, SIDEBAR_W, screenHeight - TOPBAR_H, (Color){15, 15, 25, 255});

        const char* nombresModo[3] = { "TERRENO", "PORTAL", "PLANETA" };
        for (int i = 0; i < 3; i++) {
            Rectangle r = ModoBtnRect(i);
            bool activo = (modo == (ModoEditor)i);
            DrawRectangleRec(r, activo ? (Color){60, 60, 90, 255} : (Color){35, 35, 50, 255});
            DrawRectangleLinesEx(r, activo ? 2.0f : 1.0f, activo ? WHITE : (Color){0, 220, 255, 150});
            int tw = MeasureText(nombresModo[i], 14);
            DrawText(nombresModo[i], (int)(r.x + (r.width - tw) / 2.0f), (int)(r.y + 8), 14, WHITE);
        }

        if (modo == MODO_TERRENO) {
            for (int i = 0; i < N_SWATCHES; i++) {
                Rectangle sr = SwatchRect(i);
                DrawRectangleRec(sr, SwatchColor(i));
                bool sel = (i < 5) ? (!modoErase && alturaActiva == i + 1) : modoErase;
                DrawRectangleLinesEx(sr, sel ? 3.0f : 1.0f, sel ? WHITE : (Color){150, 150, 150, 100});
                const char* label = (i < 5) ? TextFormat("%d", i + 1) : "E";
                DrawText(label, (int)(sr.x + sr.width / 2 - 5), (int)(sr.y + sr.height / 2 - 8), 16, BLACK);
            }
            DrawText(TextFormat("brush: %d", brushSize), 20, (int)(SwatchRect(N_SWATCHES - 1).y + 60), 14, LIGHTGRAY);
        } else if (modo == MODO_PORTAL) {
            TextBoxDibujar(tbRadioPortal, "radio portal", (Color){0, 220, 255, 150});
            DrawText(TextFormat("%d activos", (int)nivel.portales.size()), 20, (int)(RadioRect().y + 40), 12, LIGHTGRAY);
            DrawText("click: coloca", 20, (int)(RadioRect().y + 60), 12, GRAY);
            DrawText("click der: borra", 20, (int)(RadioRect().y + 76), 12, GRAY);
        } else if (modo == MODO_PLANETA) {
            TextBoxDibujar(tbRadioPlaneta, "radio planeta", (Color){0, 220, 255, 150});
            DrawText(TextFormat("%d activos", (int)nivel.planetas.size()), 20, (int)(RadioRect().y + 40), 12, LIGHTGRAY);
            DrawText("click: coloca", 20, (int)(RadioRect().y + 60), 12, GRAY);
            DrawText("click der: borra", 20, (int)(RadioRect().y + 76), 12, GRAY);
        }

        DrawRectangle(0, 0, screenWidth, TOPBAR_H, (Color){15, 15, 25, 255});
        DrawLine(0, TOPBAR_H, screenWidth, TOPBAR_H, (Color){0, 255, 255, 80});
        TextBoxDibujar(tbNombre,  "nombre",  (Color){0, 220, 255, 150});
        TextBoxDibujar(tbArchivo, "archivo", (Color){0, 220, 255, 150});
        DibujarBoton(btnGuardar, "guardar", mousePos);
        DibujarBoton(btnCargar,  "cargar",  mousePos);
        DibujarBoton(btnNuevo,   "nuevo",   mousePos);

        if (statusTimer > 0.0f) {
            DrawText(statusMsg.c_str(), 980, (int)(TOPBAR_H / 2.0f + 4), 16, (Color){0, 255, 140, 255});
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
