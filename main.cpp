#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "avion.h"
#include "bomba.h"
#include "mapa.h"
#include "saturno.h"
#include "portal.h"
#include "editor.h"
#include "escarabajo.h"
#include "ambiente.h"
#include "sam.h"
#include "pausa_decoracion.h"
#include "notificaciones.h"
#include "hud.h"
#include "enemy.h"
#include "level_data.h"
#include "level_io.h"
#include <vector>
#include <memory>
#include <cstdlib>

enum GameState { MENU, FREE_ROOM, LEVELS, EDITOR, PAUSA };

struct BotonesPausa {
    Rectangle resume, restart, mainMenu, quit;
};

static BotonesPausa LayoutBotonesPausa() {
    const float x0 = 80.0f, y0 = 220.0f;
    const float pasoX = 35.0f, pasoY = 75.0f; 
    const float w = 280.0f, h = 50.0f;

    BotonesPausa b;
    b.resume   = { x0 + pasoX*0, y0 + pasoY*0, w, h };
    b.restart  = { x0 + pasoX*1, y0 + pasoY*1, w, h };
    b.mainMenu = { x0 + pasoX*2, y0 + pasoY*2, w, h };
    b.quit     = { x0 + pasoX*3, y0 + pasoY*3, w, h };
    return b;
}

static Vector3 EncontrarPosicionSam(const Mapa& mapa, Vector3 portalPlanoPos, Vector3 saturnoPos) {
    int celdas = mapa.GetCeldas();
    float paso = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;

    const float distanciaMinimaPortal = 20.0f; 

    auto celdaAMundo = [&](int cx, int cz) -> Vector3 {
        float worldX = (cx * paso) - offset + paso * 0.5f;
        float worldZ = (cz * paso) - offset + paso * 0.5f;
        return { worldX, 0.0f, worldZ };
    };

    auto demasiadoCerca = [&](Vector3 wp) -> bool {
        Vector3 a = { wp.x, 0.0f, wp.z };
        Vector3 b1 = { portalPlanoPos.x, 0.0f, portalPlanoPos.z };
        Vector3 b2 = { saturnoPos.x, 0.0f, saturnoPos.z };
        return Vector3Distance(a, b1) < distanciaMinimaPortal ||
               Vector3Distance(a, b2) < distanciaMinimaPortal;
    };

    for (int tierBuscado = 5; tierBuscado >= 1; tierBuscado--) {
        std::vector<std::pair<int,int>> candidatas;
        for (int x = 0; x < celdas; x++) {
            for (int z = 0; z < celdas; z++) {
                if (mapa.GetAltura(x, z) != tierBuscado) continue;
                if (demasiadoCerca(celdaAMundo(x, z))) continue;
                candidatas.push_back({x, z});
            }
        }
        if (!candidatas.empty()) {
            auto [cx, cz] = candidatas[rand() % candidatas.size()];
            Vector3 wp = celdaAMundo(cx, cz);
            wp.y = mapa.AlturaSuperficie(wp.x, wp.z); 
            return wp;
        }
    }

    return { 30.0f, 0.0f, 30.0f };
}

int main() {
    int screenWidth  = 1280;
    int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "RetroB");

    SetExitKey(KEY_NULL);

    GameState currentState = MENU;
    GameState estadoAntesDePausa = MENU; 

    Camera3D camera = {0};
    camera.position   = {0.0f, 20.0f, 20.0f};
    camera.target     = {0.0f, 0.0f, 0.0f};
    camera.up         = {0.0f, 1.0f, 0.0f};
    camera.fovy       = 100.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Avion avion;
    avion.SetPosicion({0.0f, 5.0f, 15.0f});

    Notificaciones notificaciones;

    bool danoActivado = false;

    Mapa     mapa(400, 40);
    Ambiente ambiente(mapa);
    ambiente.CargarShader();
    Editor   mapaEditor;

    Saturno saturno({50.0f, 10.0f, 0.0f}, 10.0f, 16.0f, 2.0f, 10.0f);

    Portal portalPlano ({0.0f, 0.5f, 0.0f},       5.0f, BLUE);
    Portal portalSaturno(saturno.GetNorthPole(),   2.0f, RED);

    Sam samVigia(EncontrarPosicionSam(mapa, portalPlano.GetPosicion(), saturno.GetPosicion()));
    bool samPosicionada = false; 

    bool enSaturno = false;
    SetTargetFPS(60);

    LevelData nivelActual;
    std::vector<std::unique_ptr<Enemy>> enemigosNivel;
    int  objetivoActualIdx = 0;   
    bool nivelCargado      = false;
    bool nivelCompletado   = false;

    auto CargarNivel = [&](const std::string& ruta) {
        if (!LevelIO::Cargar(ruta, nivelActual)) return;

        Mapa mapaNivel(nivelActual.tamano, nivelActual.celdas);
        for (int x = 0; x < nivelActual.celdas; x++)
            for (int z = 0; z < nivelActual.celdas; z++)
                if (x < (int)nivelActual.alturas.size() && z < (int)nivelActual.alturas[x].size())
                    mapaNivel.SetAltura(x, z, nivelActual.alturas[x][z]);
        mapa = mapaNivel;
        ambiente.RecalcularAgua();

        enemigosNivel.clear();
        for (const auto& spawn : nivelActual.enemigos) {
            if (spawn.tipo == "sam") {
                enemigosNivel.push_back(
                    std::make_unique<Sam>(spawn.posicion, spawn.id, spawn.config)
                );
            }
        }

        avion.SetPosicion(nivelActual.jugadorSpawn);
        avion.SetPlanetaActual(nullptr);
        avion.SetEscala(1.0f);
        objetivoActualIdx = 0;
        nivelCompletado   = false;
        nivelCargado      = true;
    };

    Escarabajo escarabajoGigante({-80.0f, 40.0f, -80.0f}, 15.0f);

    PausaDecoracion decoracionPausa;
    Camera3D camPausa = {0};
    camPausa.position   = {0.0f, 8.0f, 22.0f};
    camPausa.target     = {0.0f, 1.0f, 0.0f};
    camPausa.up         = {0.0f, 1.0f, 0.0f};
    camPausa.fovy       = 45.0f;
    camPausa.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float   dt       = GetFrameTime();
        Vector2 mousePos = GetMousePosition();

        if (currentState == MENU) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, {500, 200, 280, 50})) currentState = FREE_ROOM;
                if (CheckCollisionPointRec(mousePos, {500, 300, 280, 50})) currentState = LEVELS;
                if (CheckCollisionPointRec(mousePos, {500, 400, 280, 50})) currentState = EDITOR;
                if (CheckCollisionPointRec(mousePos, {500, 500, 280, 50})) break; 
            }
        }

        else if (currentState == PAUSA) {
            decoracionPausa.Actualizar(dt);

            BotonesPausa btn = LayoutBotonesPausa();
            Rectangle& btnResume   = btn.resume;
            Rectangle& btnRestart  = btn.restart;
            Rectangle& btnMainMenu = btn.mainMenu;
            Rectangle& btnQuit     = btn.quit;

            if (IsKeyPressed(KEY_ESCAPE)) currentState = estadoAntesDePausa; 

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, btnResume)) {
                    currentState = estadoAntesDePausa;
                }
                else if (CheckCollisionPointRec(mousePos, btnRestart)) {
                    if (estadoAntesDePausa == FREE_ROOM) {
                        avion.SetPosicion({0.0f, 5.0f, 15.0f});
                        avion.SetPlanetaActual(nullptr);
                        avion.SetEscala(1.0f);
                        enSaturno = false;
                        samVigia = Sam(EncontrarPosicionSam(mapa, portalPlano.GetPosicion(), saturno.GetPosicion()));
                        currentState = FREE_ROOM;
                    } else if (estadoAntesDePausa == LEVELS) {
                        nivelCargado = false; 
                        currentState = LEVELS;
                    } else {
                        currentState = estadoAntesDePausa;
                    }
                }
                else if (CheckCollisionPointRec(mousePos, btnMainMenu)) {
                    currentState = MENU;
                }
                else if (CheckCollisionPointRec(mousePos, btnQuit)) {
                    break; 
                }
            }
        }

        else if (currentState == LEVELS) {
            if (!nivelCargado) CargarNivel("levels/nivel_01.lvl");

            if (IsKeyPressed(KEY_ESCAPE)) { estadoAntesDePausa = LEVELS; currentState = PAUSA; }

            if (nivelCargado && !nivelCompletado) {
                avion.ActualizarMira(dt, camera, {}, false, screenWidth, screenHeight);
                for (auto& e : enemigosNivel) {
                    if (e->EstaVivo()) {
                        avion.ActualizarMira(dt, camera, e->GetPosicion(), true, screenWidth, screenHeight);
                        break;
                    }
                }
                avion.Actualizar(dt, {}, false);
                notificaciones.Actualizar(dt);
                ambiente.Actualizar(dt, camera);

                for (auto& e : enemigosNivel) {
                    bool impacto = e->Actualizar(dt, avion.GetPosicion());
                    if (impacto) {
                        if (Sam* s = dynamic_cast<Sam*>(e.get()))
                            avion.RecibirDano(s->GetDanoMisil());
                    }
                    if (e->EstaVivo() && avion.RevisarImpacto(e->GetPosicion(), 1.8f))
                        e->RecibirDano(20.0f);
                }

                if (objetivoActualIdx < (int)nivelActual.objetivos.size()) {
                    const Objective& obj = nivelActual.objetivos[objetivoActualIdx];

                    bool completado = false;
                    if (obj.tipo == "eliminar") {
                        completado = true;
                        for (const auto& tid : obj.targetIds) {
                            for (auto& e : enemigosNivel) {
                                if (e->GetId() == tid && e->EstaVivo()) { completado = false; break; }
                            }
                            if (!completado) break;
                        }
                    } else if (obj.tipo == "alcanzar") {
                        completado = Vector3Distance(avion.GetPosicion(), obj.location) <= obj.radius;
                    } else if (obj.tipo == "sobrevivir") {
                    }

                    if (completado) {
                        notificaciones.Empujar(obj.descripcion + " - COMPLETADO", NOTIF_EXITO);
                        objetivoActualIdx++;
                        if (objetivoActualIdx >= (int)nivelActual.objetivos.size()) {
                            nivelCompletado = true;
                            notificaciones.Empujar("MISION COMPLETADA", NOTIF_EXITO);
                        }
                    }
                }

                float rotY = avion.GetRotacionY() * DEG2RAD;
                Vector3 dirAdelante = { sinf(rotY), 0.0f, cosf(rotY) };
                Vector3 camDeseada  = Vector3Add(avion.GetPosicion(), Vector3Scale(dirAdelante, -10.0f));
                camDeseada.y += 8.0f;
                camera.position = Vector3Lerp(camera.position, camDeseada, 0.1f);
                camera.target   = Vector3Lerp(camera.target, avion.GetPosicion(), 0.1f);
            }
        }

        else if (currentState == EDITOR) {
            bool irAFreeRoom = mapaEditor.Actualizar(mapa, mousePos, screenWidth, screenHeight,
                                                     portalPlano.GetPosicion(), saturno.GetPosicion());
            ambiente.RecalcularAgua();
            if (irAFreeRoom) currentState = FREE_ROOM;
            if (IsKeyPressed(KEY_ESCAPE)) { estadoAntesDePausa = EDITOR; currentState = PAUSA; }
        }

        else if (currentState == FREE_ROOM) {
            if (!samPosicionada) {
                samVigia.SetPosicion(EncontrarPosicionSam(mapa, portalPlano.GetPosicion(), saturno.GetPosicion()));
                samPosicionada = true;
            }

            bool teniaLockAntes = avion.GetMiraTrabada();
            avion.ActualizarMira(dt, camera, samVigia.GetPosicion(), samVigia.EstaVivo(),
                                 screenWidth, screenHeight);
            if (!teniaLockAntes && avion.GetMiraTrabada()) {
                notificaciones.Empujar("LOCK ADQUIRIDO", NOTIF_EXITO);
            }

            avion.Actualizar(dt, samVigia.GetPosicion(), samVigia.EstaVivo());
            saturno.Actualizar(dt);
            portalPlano.Actualizar(dt);
            portalSaturno.Actualizar(dt);
            ambiente.Actualizar(dt, camera);
            notificaciones.Actualizar(dt);

            EstadoSam estadoSamAntes = samVigia.GetEstado();
            bool huboImpactoSam = samVigia.Actualizar(dt, avion.GetPosicion());
            if (estadoSamAntes != SAM_BLOQUEADO && samVigia.GetEstado() == SAM_BLOQUEADO) {
                notificaciones.Empujar("PELIGRO: SAM TE TIENE EN LA MIRA", NOTIF_PELIGRO);
            }

            if (danoActivado && huboImpactoSam) {
                avion.RecibirDano(samVigia.GetDanoMisil());
            }

            // escarabajoGigante.Actualizar(dt);

            bool samVivoAntes = samVigia.EstaVivo();
            if (samVigia.EstaVivo() && avion.RevisarImpacto(samVigia.GetPosicion(), 1.8f)) {
                samVigia.RecibirDano(20.0f); 
                if (samVivoAntes && !samVigia.EstaVivo()) {
                    notificaciones.Empujar("SAM DESTRUIDO", NOTIF_EXITO);
                }
            }

            Vector3 posAvion = avion.GetPosicion();

            if (IsKeyPressed(KEY_TAB)) ambiente.ToggleMode();

            if (!enSaturno && portalPlano.DetectarEntrada(posAvion)) {
                avion.SetPlanetaActual(&saturno);
                Vector3 llegadaSaturno  = saturno.GetNorthPole();
                llegadaSaturno.x       += 3.0f;
                avion.SetPosicion(llegadaSaturno);
                avion.SetEscala(0.2f);
                enSaturno = true;
                notificaciones.Empujar("ENTRANDO A SATURNO", NOTIF_INFO);
            }
            else if (enSaturno && portalSaturno.DetectarEntrada(posAvion)) {
                avion.SetPosicion({0.0f, 5.0f, 15.0f});
                avion.SetPlanetaActual(nullptr);
                avion.SetEscala(1.0f);
                enSaturno = false;
                notificaciones.Empujar("REGRESANDO A FREE ROOM", NOTIF_INFO);
            }

            float   suavizado = 0.1f;
            Vector3 camDeseada;

            if (enSaturno) {
                Vector3 normal     = Vector3Normalize(Vector3Subtract(posAvion, saturno.GetPosicion()));
                camera.up          = normal;
                float   rotY       = avion.GetRotacionY() * DEG2RAD;
                Vector3 dirAdelante = { sinf(rotY), 0.0f, cosf(rotY) };
                camDeseada         = Vector3Add(posAvion, Vector3Scale(dirAdelante, -4.0f));
                camDeseada         = Vector3Add(camDeseada, Vector3Scale(normal, 2.0f));
            } else {
                camera.up           = {0.0f, 1.0f, 0.0f};
                float   rotY        = avion.GetRotacionY() * DEG2RAD;
                Vector3 dirAdelante = { sinf(rotY), 0.0f, cosf(rotY) };
                camDeseada          = Vector3Add(posAvion, Vector3Scale(dirAdelante, -10.0f));
                camDeseada.y       += 8.0f;
            }

            camera.position = Vector3Lerp(camera.position, camDeseada, suavizado);
            camera.target   = Vector3Lerp(camera.target,   posAvion,   suavizado);

            if (IsKeyPressed(KEY_ESCAPE)) { estadoAntesDePausa = FREE_ROOM; currentState = PAUSA; }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        bool dibujarComo_MENU      = (currentState == MENU);
        bool dibujarComo_LEVELS    = (currentState == LEVELS)    || (currentState == PAUSA && estadoAntesDePausa == LEVELS);
        bool dibujarComo_EDITOR    = (currentState == EDITOR)    || (currentState == PAUSA && estadoAntesDePausa == EDITOR);
        bool dibujarComo_FREE_ROOM = (currentState == FREE_ROOM) || (currentState == PAUSA && estadoAntesDePausa == FREE_ROOM);

        if (dibujarComo_MENU) {
            DrawText("RETRO-B", 500, 100, 60, SKYBLUE);
            DrawRectangle(500, 200, 280, 50, DARKGRAY); DrawText("FREE ROOM", 550, 215, 20, WHITE);
            DrawRectangle(500, 300, 280, 50, DARKGRAY); DrawText("LEVELS",    580, 315, 20, WHITE);
            DrawRectangle(500, 400, 280, 50, DARKGRAY); DrawText("EDITOR",    580, 415, 20, WHITE);
            DrawRectangle(500, 500, 280, 50, DARKGRAY); DrawText("EXIT",      600, 515, 20, WHITE);
        }
        else if (dibujarComo_LEVELS) {
            if (nivelCargado) {
                BeginMode3D(camera);
                    mapa.Dibujar3D(camera.position, camera.target, camera.fovy,
                                   (float)screenWidth / (float)screenHeight, 80.0f);
                    ambiente.Dibujar(camera);
                    for (const auto& e : enemigosNivel) {
                        e->Dibujar();
                        e->DibujarBarraSalud();
                        if (ambiente.GetModoNatural()) {
                            if (Sam* s = dynamic_cast<Sam*>(e.get())) s->DibujarCono();
                        }
                    }
                    avion.Dibujar();
                EndMode3D();

                avion.DibujarMira();
                notificaciones.Dibujar(screenWidth, screenHeight);

                if (!nivelCompletado && objetivoActualIdx < (int)nivelActual.objetivos.size()) {
                    const std::string& desc = nivelActual.objetivos[objetivoActualIdx].descripcion;
                    DrawText(desc.c_str(), 10, 40, 18, YELLOW);
                }
                DrawFPS(10, 10);
            } else {
                DrawText("cargando nivel...", 400, 340, 24, LIGHTGRAY);
            }
        }
        else if (dibujarComo_EDITOR) {
            mapaEditor.Dibujar(mapa, screenWidth, screenHeight,
                               portalPlano.GetPosicion(), saturno.GetPosicion());
        }
        else if (dibujarComo_FREE_ROOM) {
            BeginMode3D(camera);
                mapa.Dibujar3D(camera.position, camera.target, camera.fovy,
                               (float)screenWidth / (float)screenHeight, 60.0f);
                ambiente.Dibujar(camera);
                saturno.Dibujar();
                portalPlano.Dibujar();
                portalSaturno.Dibujar();
                samVigia.Dibujar();
                samVigia.DibujarBarraSalud();
                if (ambiente.GetModoNatural()) samVigia.DibujarCono();
                if (ambiente.GetModoNatural()) avion.DibujarConoLock();
                avion.Dibujar();
            EndMode3D();

            avion.DibujarMira();

            Hud::DibujarPanelPrincipal(screenWidth, screenHeight, avion.GetPorcentajeSalud(),
                                        avion.GetFraccionBoost(), avion.GetMisilListo(),
                                        /*municionInfinita=*/true, avion.GetNumMisilesActivos());
            notificaciones.Dibujar(screenWidth, screenHeight);

            DrawFPS(10, 10);
            const char* modoLabel = ambiente.GetModoNatural() ? "[TAB] Retro" : "[TAB] Natural";
            DrawText(modoLabel, 10, 60, 18, LIGHTGRAY);
        }

        if (currentState == PAUSA) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 160}); 

            rlViewport(screenWidth / 2, 0, screenWidth / 2, screenHeight);
            BeginMode3D(camPausa);
                decoracionPausa.Dibujar();
            EndMode3D();
            rlViewport(0, 0, screenWidth, screenHeight);

            DrawText("PAUSA", 80, 130, 50, SKYBLUE);

            BotonesPausa btn = LayoutBotonesPausa();

            DrawRectangleRec(btn.resume,   DARKGRAY); DrawText("RESUME",     btn.resume.x+70,   btn.resume.y+15,   20, WHITE);
            DrawRectangleRec(btn.restart,  DARKGRAY); DrawText("RESTART",    btn.restart.x+65,  btn.restart.y+15,  20, WHITE);
            DrawRectangleRec(btn.mainMenu, DARKGRAY); DrawText("MAIN MENU",  btn.mainMenu.x+50, btn.mainMenu.y+15, 20, WHITE);
            DrawRectangleRec(btn.quit,     DARKGRAY); DrawText("QUIT",       btn.quit.x+90,     btn.quit.y+15,    20, WHITE);
        }

        EndDrawing();
    }

    ambiente.DescargarShader();
    CloseWindow();
    return 0;
}
