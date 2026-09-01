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
#include "ui.h"
#include "enemy.h"
#include "level_data.h"
#include "level_io.h"
#include "efectos/efectos_manager.h"
#include "ataque_enjambre.h"
#include "postproceso_enjambre.h"
#include <vector>
#include <memory>
#include <cstdlib>
#include <string>

enum GameState { MENU, FREE_ROOM, LEVELS, LEVEL_SELECT, EDITOR, PAUSA };

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

    struct CierreVentana {
        ~CierreVentana() {
            CloseWindow();
        }
    } cierreVentana;

    SetExitKey(KEY_NULL);

    if (!UI::Inicializar("assets/fonts/SyneMono-Regular.ttf")) {
	    TraceLog(LOG_WARNING, "No se pudo cargar la fuente de la interfaz");
    }

    RenderTexture2D escenaRender = LoadRenderTexture(
        screenWidth,
        screenHeight
    );

    SetTextureFilter(
        escenaRender.texture,
        TEXTURE_FILTER_POINT
    );

    PostProcesoEnjambre postProcesoEnjambre;
    postProcesoEnjambre.Cargar(
        screenWidth,
        screenHeight
    );

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
    AtaqueEnjambre enjambreJugador;

    Notificaciones notificaciones;

    auto IntentarDisparoEnjambre = [&](Enemy* objetivo) {
        if (!objetivo || !objetivo->EstaVivo()) {
            notificaciones.Empujar("ENJAMBRE: SIN OBJETIVO", NOTIF_INFO);
            return;
        }

        if (!enjambreJugador.ListoParaDisparar()) {
            int segundos = (int)enjambreJugador.GetCooldownRestante() + 1;
            notificaciones.Empujar(
                "ENJAMBRE RECARGANDO: " + std::to_string(segundos) + "s",
                NOTIF_INFO
            );
            return;
        }

        float distancia = Vector3Distance(avion.GetPosicion(), objetivo->GetPosicion());
        if (distancia < AtaqueEnjambre::DISTANCIA_MINIMA_ENJAMBRE) {
            int faltantes = (int)(AtaqueEnjambre::DISTANCIA_MINIMA_ENJAMBRE - distancia) + 1;
            notificaciones.Empujar(
                "ENJAMBRE: ALEJATE " + std::to_string(faltantes) + "m",
                NOTIF_INFO
            );
            return;
        }

        if (enjambreJugador.Disparar(avion.GetPosicion(), objetivo)) {
            notificaciones.Empujar("ENJAMBRE DESPLEGADO", NOTIF_EXITO);
        }
    };

    bool danoActivado = false;

    Mapa     mapa(400, 40);
    Ambiente ambiente(mapa);
    ambiente.CargarShader();
    EfectosManager::Instancia().CargarShader();
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
    std::vector<Portal>  portalesNivel;
    std::vector<Portal>  portalesRetornoNivel;
    std::vector<Saturno> planetasNivel;
    int  planetaActualIdx  = -1;
    bool enPlanetaNivel    = false;
    int  objetivoActualIdx = 0;   
    bool nivelCargado      = false;
    bool nivelCompletado   = false;

    std::string rutaNivelActual = "";
    std::vector<std::string> nivelesDisponibles;

    auto EscanearNiveles = [&]() {
        nivelesDisponibles.clear();
        FilePathList archivos = LoadDirectoryFiles("Levels_editor/levels");
        for (unsigned int i = 0; i < archivos.count; i++) {
            if (IsFileExtension(archivos.paths[i], ".lvl")) {
                nivelesDisponibles.push_back(archivos.paths[i]);
            }
        }
        UnloadDirectoryFiles(archivos);
    };

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

        portalesNivel.clear();
        for (const auto& p : nivelActual.portales) {
            portalesNivel.push_back(Portal(p.posicion, p.radius, BLUE));
        }

        planetasNivel.clear();
        portalesRetornoNivel.clear();
        for (const auto& p : nivelActual.planetas) {
            Saturno s(p.posicion, p.radius, p.radius * 1.6f, p.radius * 0.2f, 10.0f);
            planetasNivel.push_back(s);
            portalesRetornoNivel.push_back(Portal(s.GetNorthPole(), 2.0f, RED));
        }

        enPlanetaNivel   = false;
        planetaActualIdx = -1;

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

        EfectosManager::Instancia().Actualizar(dt);

        if (currentState == MENU) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, {500, 200, 280, 50})) currentState = FREE_ROOM;
                if (CheckCollisionPointRec(mousePos, {500, 300, 280, 50})) {
                    EscanearNiveles();
                    currentState = LEVEL_SELECT;
                }
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

        else if (currentState == LEVEL_SELECT) {
            if (IsKeyPressed(KEY_ESCAPE)) currentState = MENU;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                for (int i = 0; i < (int)nivelesDisponibles.size(); i++) {
                    Rectangle r = { 400.0f, 180.0f + i * 60.0f, 480.0f, 50.0f };
                    if (CheckCollisionPointRec(mousePos, r)) {
                        rutaNivelActual = nivelesDisponibles[i];
                        nivelCargado = false;
                        currentState = LEVELS;
                        break;
                    }
                }
            }
        }

        else if (currentState == LEVELS) {
            if (!nivelCargado) CargarNivel(rutaNivelActual);

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

                for (auto& p : portalesNivel) p.Actualizar(dt);
                for (auto& p : portalesRetornoNivel) p.Actualizar(dt);
                for (auto& s : planetasNivel) s.Actualizar(dt);

                Vector3 posAvionNivel = avion.GetPosicion();
                if (!enPlanetaNivel) {
                    for (int i = 0; i < (int)portalesNivel.size() && i < (int)planetasNivel.size(); i++) {
                        if (portalesNivel[i].DetectarEntrada(posAvionNivel)) {
                            avion.SetPlanetaActual(&planetasNivel[i]);
                            Vector3 llegada = planetasNivel[i].GetNorthPole();
                            llegada.x += 3.0f;
                            avion.SetPosicion(llegada);
                            avion.SetEscala(0.2f);
                            enPlanetaNivel   = true;
                            planetaActualIdx = i;
                            notificaciones.Empujar("ENTRANDO AL PLANETA", NOTIF_INFO);
                            break;
                        }
                    }
                } else if (planetaActualIdx >= 0 && planetaActualIdx < (int)portalesRetornoNivel.size()) {
                    if (portalesRetornoNivel[planetaActualIdx].DetectarEntrada(posAvionNivel)) {
                        avion.SetPosicion(portalesNivel[planetaActualIdx].GetPosicion());
                        avion.SetPlanetaActual(nullptr);
                        avion.SetEscala(1.0f);
                        enPlanetaNivel   = false;
                        planetaActualIdx = -1;
                        notificaciones.Empujar("REGRESANDO", NOTIF_INFO);
                    }
                }

                for (auto& e : enemigosNivel) {
                    bool impacto = e->Actualizar(dt, avion.GetPosicion());
                    if (impacto) {
                        if (Sam* s = dynamic_cast<Sam*>(e.get()))
                            avion.RecibirDano(s->GetDanoMisil());
                    }
                    if (e->EstaVivo() && avion.RevisarImpacto(e->GetPosicion(), 1.8f))
                        e->RecibirDano(20.0f);
                }

		std::vector<Enemy*> enemigosParaEnjambre;
		for (auto& e : enemigosNivel) if (e->EstaVivo()) enemigosParaEnjambre.push_back(e.get());

		if (IsKeyPressed(KEY_M)) {
			IntentarDisparoEnjambre(
				enemigosParaEnjambre.empty() ? nullptr : enemigosParaEnjambre[0]
			);
		}
		enjambreJugador.Actualizar(dt, enemigosParaEnjambre);

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

	    if (IsKeyPressed(KEY_M)) {
		    IntentarDisparoEnjambre(samVigia.EstaVivo() ? &samVigia : nullptr);
	    }
	    std::vector<Enemy*> enemigosParaEnjambre;
	    if (samVigia.EstaVivo()) enemigosParaEnjambre.push_back(&samVigia);
	    enjambreJugador.Actualizar(dt, enemigosParaEnjambre);

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

	const bool habilitarPostProcesoEnjambre =
		currentState == LEVELS ||
		currentState == FREE_ROOM;

	postProcesoEnjambre.Actualizar(
			enjambreJugador,
			camera,
			screenWidth,
			screenHeight,
			habilitarPostProcesoEnjambre
			);

	BeginTextureMode(escenaRender);
	ClearBackground(BLACK);


	bool dibujarComo_MENU         = (currentState == MENU);
	bool dibujarComo_LEVEL_SELECT = (currentState == LEVEL_SELECT);
	bool dibujarComo_LEVELS    = (currentState == LEVELS)    || (currentState == PAUSA && estadoAntesDePausa == LEVELS);
        bool dibujarComo_EDITOR    = (currentState == EDITOR)    || (currentState == PAUSA && estadoAntesDePausa == EDITOR);
        bool dibujarComo_FREE_ROOM = (currentState == FREE_ROOM) || (currentState == PAUSA && estadoAntesDePausa == FREE_ROOM);
	const bool mostrarDiagnostico = !ambiente.GetModoNatural();

	if (dibujarComo_MENU) {
		UI::DibujarTextoCentrado(
				"RETRO-B",
				{0.0f, 100.0f, (float)screenWidth, 60.0f},
				60.0f,
				SKYBLUE,
				2.0f
				);

		const Rectangle botonFreeRoom{500.0f, 200.0f, 280.0f, 50.0f};
		const Rectangle botonLevels{500.0f, 300.0f, 280.0f, 50.0f};
		const Rectangle botonEditor{500.0f, 400.0f, 280.0f, 50.0f};
		const Rectangle botonExit{500.0f, 500.0f, 280.0f, 50.0f};

		DrawRectangleRec(botonFreeRoom, DARKGRAY);
		DrawRectangleRec(botonLevels, DARKGRAY);
		DrawRectangleRec(botonEditor, DARKGRAY);
		DrawRectangleRec(botonExit, DARKGRAY);

		UI::DibujarTextoCentrado("FREE ROOM", botonFreeRoom, 20.0f, WHITE);
		UI::DibujarTextoCentrado("LEVELS", botonLevels, 20.0f, WHITE);
		UI::DibujarTextoCentrado("EDITOR", botonEditor, 20.0f, WHITE);
		UI::DibujarTextoCentrado("EXIT", botonExit, 20.0f, WHITE);
	}
	else if (dibujarComo_LEVEL_SELECT) {
		UI::DibujarTexto(
				"SELECCIONA UN NIVEL",
				{400.0f, 100.0f},
				30.0f,
				SKYBLUE
				);

		if (nivelesDisponibles.empty()) {
			UI::DibujarTexto(
					"no hay niveles guardados todavía",
					{400.0f, 200.0f},
					20.0f,
					LIGHTGRAY
					);

			UI::DibujarTexto(
					"usa el Editor para crear uno en Levels_editor/levels/",
					{400.0f, 230.0f},
					16.0f,
					GRAY
					);
		} else {
			for (int i = 0; i < (int)nivelesDisponibles.size(); i++) {
				Rectangle r{
					400.0f,
						180.0f + i * 60.0f,
						480.0f,
						50.0f
				};

				DrawRectangleRec(r, DARKGRAY);
				DrawRectangleLinesEx(r, 1.0f, SKYBLUE);

				UI::DibujarTextoCentrado(
						nivelesDisponibles[i].c_str(),
						r,
						18.0f,
						WHITE
						);
			}
		}

		UI::DibujarTexto(
				"[ESC] volver",
				{400.0f, 650.0f},
				16.0f,
				LIGHTGRAY
				);
	}
	else if (dibujarComo_LEVELS) {
		if (nivelCargado) {
			ambiente.DibujarCielo(screenWidth, screenHeight);
			BeginMode3D(camera);
			mapa.Dibujar3D(ambiente.GetModoNatural());
			ambiente.Dibujar(camera);
			for (auto& s : planetasNivel) s.Dibujar();
			for (auto& p : portalesNivel) p.Dibujar();
			for (auto& p : portalesRetornoNivel) p.Dibujar();
			for (const auto& e : enemigosNivel) {
				e->Dibujar();
				e->DibujarBarraSalud();
				if (mostrarDiagnostico) {
					if (Sam* s = dynamic_cast<Sam*>(e.get())) s->DibujarCono();
				}
			}
			if (mostrarDiagnostico) avion.DibujarConoLock();
			avion.Dibujar();
			EfectosManager::Instancia().Dibujar(camera);
			enjambreJugador.Dibujar();
			EndMode3D();

			avion.DibujarMira();
			notificaciones.Dibujar(screenWidth, screenHeight);

			if (!nivelCompletado && objetivoActualIdx < (int)nivelActual.objetivos.size()) {
				const std::string& desc = nivelActual.objetivos[objetivoActualIdx].descripcion;
				UI::DibujarTexto(
						desc.c_str(),
						{10.0f, 40.0f},
						18.0f,
						YELLOW
						);
			}
			DrawFPS(10, 10);
		} else {
			UI::DibujarTexto(
					"cargando nivel...",
					{400.0f, 340.0f},
					24.0f,
					LIGHTGRAY
					);
		}
	}
	else if (dibujarComo_EDITOR) {
		mapaEditor.Dibujar(mapa, screenWidth, screenHeight,
				portalPlano.GetPosicion(), saturno.GetPosicion());
	}
	else if (dibujarComo_FREE_ROOM) {
		ambiente.DibujarCielo(screenWidth, screenHeight);
		BeginMode3D(camera);
		mapa.Dibujar3D(ambiente.GetModoNatural());
		ambiente.Dibujar(camera);
		saturno.Dibujar();
		portalPlano.Dibujar();
		portalSaturno.Dibujar();
		samVigia.Dibujar();
		samVigia.DibujarBarraSalud();
		if (mostrarDiagnostico) samVigia.DibujarCono();
		if (mostrarDiagnostico) avion.DibujarConoLock();
		avion.Dibujar();
		EfectosManager::Instancia().Dibujar(camera);
		enjambreJugador.Dibujar();
		EndMode3D();

		avion.DibujarMira();

		Hud::DibujarPanelPrincipal(screenWidth, screenHeight, avion.GetPorcentajeSalud(),
				avion.GetFraccionBoost(), avion.GetMisilListo(),
				/*municionInfinita=*/true, avion.GetNumMisilesActivos());
		notificaciones.Dibujar(screenWidth, screenHeight);

		DrawFPS(10, 10);
		const char* modoLabel = ambiente.GetModoNatural() ? "[TAB] Nat" : "[TAB] Retro";
		UI::DibujarTexto(
				modoLabel,
				{10.0f, 60.0f},
				18.0f,
				LIGHTGRAY
				);
	}

	if (currentState == PAUSA) {
		DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 160}); 

		rlViewport(screenWidth / 2, 0, screenWidth / 2, screenHeight);
		BeginMode3D(camPausa);
		decoracionPausa.Dibujar();
		EndMode3D();
		rlViewport(0, 0, screenWidth, screenHeight);

		UI::DibujarTexto(
				"PAUSA",
				{80.0f, 130.0f},
				50.0f,
				SKYBLUE
				);

		BotonesPausa btn = LayoutBotonesPausa();

		DrawRectangleRec(btn.resume, DARKGRAY);
		DrawRectangleRec(btn.restart, DARKGRAY);
		DrawRectangleRec(btn.mainMenu, DARKGRAY);
		DrawRectangleRec(btn.quit, DARKGRAY);

		UI::DibujarTextoCentrado("RESUME", btn.resume, 20.0f, WHITE);
		UI::DibujarTextoCentrado("RESTART", btn.restart, 20.0f, WHITE);
		UI::DibujarTextoCentrado("MAIN MENU", btn.mainMenu, 20.0f, WHITE);
		UI::DibujarTextoCentrado("QUIT", btn.quit, 20.0f, WHITE);
	}

	EndTextureMode();

	BeginDrawing();
	ClearBackground(BLACK);

	postProcesoEnjambre.Dibujar(
			escenaRender
			);

	EndDrawing();
    }

    ambiente.DescargarShader();
    EfectosManager::Instancia().DescargarShader();

    postProcesoEnjambre.Descargar();
    UnloadRenderTexture(escenaRender);
    UI::Finalizar();

    return 0;
}
