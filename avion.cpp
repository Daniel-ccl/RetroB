#include "raylib.h"
#include "raymath.h"
#include "avion.h"
#include "bomba.h"
#include "saturno.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "rlgl.h"

Avion::Avion() {
    posicion = { 0.0f, 5.0f, 0.0f };
    rotacionY = 0.0f;
    rotacionVelocidad = 0.0f;
    velocidadBase = 0.2f;
    velocidadBoost = 0.6f;   
    velocidad = velocidadBase;

    anguloBanco = 0.0f;
    anguloPitch = 0.0f;
    velocidadVertical = 0.0f;

    boosteando = false;

    miraPos = {0.0f, 0.0f}; 
    miraTiempoSoft = 0.0f;
    miraTiempoHard = 0.0f;
    miraTrabada = false;
    miraSoftLock = false;
    miraTieneObjetivo = false;

    cooldownDisparo = 0.0f;

    for (int i = 0; i < MAX_TRAIL; i++) {
        trail[i] = posicion;
        trailJitter[i] = { 0.0f, 0.0f };
    }
    trailIndex = 0;

    planetaActual = nullptr;
    escala = 1.0f;
}

void Avion::ActualizarMira(float dt, const Camera3D& cam, Vector3 objetivoPos, bool objetivoExiste,
                            int screenWidth, int screenHeight) {
    Vector2 centro = { screenWidth / 2.0f, screenHeight / 2.0f };

    bool elegible = false;
    Vector2 puntoObjetivo = centro;

    if (objetivoExiste) {
        Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
        Vector3 haciaObjetivo = Vector3Subtract(objetivoPos, cam.position);
        float haciaLen = Vector3Length(haciaObjetivo);

        if (haciaLen > 0.001f) {
            float dot = Vector3DotProduct(forward, Vector3Scale(haciaObjetivo, 1.0f / haciaLen));
            if (dot > 0.0f) {
                Vector2 proyectado = GetWorldToScreen(objetivoPos, cam);
                float distPantalla = Vector2Distance(proyectado, centro);
                if (distPantalla <= MIRA_RADIO_REGION) {
                    elegible = true;
                    puntoObjetivo = proyectado;
                    miraObjetivoPos3D = objetivoPos;
                }
            }
        }
    }

    miraTieneObjetivo = elegible;

    float pasoMax = MIRA_VELOCIDAD_SEGUIMIENTO * dt;
    Vector2 destino = elegible ? puntoObjetivo : centro;
    Vector2 delta = Vector2Subtract(destino, miraPos);
    float distARecorrer = Vector2Length(delta);

    if (distARecorrer <= pasoMax) {
        miraPos = destino;
    } else {
        miraPos = Vector2Add(miraPos, Vector2Scale(delta, pasoMax / distARecorrer));
    }

    if (elegible) {
        if (!miraSoftLock) {
            miraTiempoSoft += dt;
            if (miraTiempoSoft >= MIRA_TIEMPO_SOFT_NECESARIO) {
                miraSoftLock = true;
            }
        } else {
            miraTiempoHard += dt;
            if (miraTiempoHard >= MIRA_TIEMPO_HARD_NECESARIO) {
                miraTrabada = true;
            }
        }
    } else {
        miraTiempoSoft = 0.0f;
        miraTiempoHard = 0.0f;
        miraSoftLock = false;
        miraTrabada = false;
    }
}

void Avion::DibujarMira() const {
    float tam = miraTrabada ? 26.0f : 18.0f;
    Color color;
    if (miraTrabada) {
    } else if (miraSoftLock) {
        float progreso = GetFraccionHardLock();
        color = (Color){
            (unsigned char)(255),
            (unsigned char)(255 * (1.0f - progreso)),
            0,
            255
        };
    } else if (miraTieneObjetivo) {
        color = YELLOW; 
    } else {
        color = (Color){0, 255, 255, 160}; 
    }

    Rectangle r = { miraPos.x - tam/2.0f, miraPos.y - tam/2.0f, tam, tam };
    DrawRectangleLinesEx(r, 2.0f, color);

    float esquina = tam * 0.3f;
    DrawLine((int)r.x, (int)r.y, (int)(r.x + esquina), (int)r.y, color);
    DrawLine((int)r.x, (int)r.y, (int)r.x, (int)(r.y + esquina), color);
    DrawLine((int)(r.x + r.width), (int)r.y, (int)(r.x + r.width - esquina), (int)r.y, color);
    DrawLine((int)(r.x + r.width), (int)r.y, (int)(r.x + r.width), (int)(r.y + esquina), color);
}

void Avion::DibujarConoLock() const {
    if (!miraTieneObjetivo) return; 

    Vector3 apex = Vector3Add(posicion, (Vector3){0.0f, 0.5f, 0.0f}); 

    if (miraTrabada) {
        DrawLine3D(apex, miraObjetivoPos3D, RED);
        return;
    }

    Vector3 dir = Vector3Subtract(miraObjetivoPos3D, apex);
    float dist = Vector3Length(dir);
    if (dist < 0.001f) return; 
    dir = Vector3Scale(dir, 1.0f / dist);

    Vector3 arriba = {0.0f, 1.0f, 0.0f};
    Vector3 right  = Vector3Normalize(Vector3CrossProduct(arriba, dir));
    Vector3 up     = Vector3CrossProduct(dir, right);

    const float anguloConoRad = 8.0f * DEG2RAD; 
    float radioBase = dist * tanf(anguloConoRad);

    float tiempoTotalNecesario = MIRA_TIEMPO_SOFT_NECESARIO + MIRA_TIEMPO_HARD_NECESARIO;
    float tiempoAcumulado = miraSoftLock
        ? (MIRA_TIEMPO_SOFT_NECESARIO + miraTiempoHard)
        : miraTiempoSoft;
    float progreso = Clamp(tiempoAcumulado / tiempoTotalNecesario, 0.0f, 1.0f);
    Color color = { (unsigned char)(0 + 255*progreso), (unsigned char)(255), (unsigned char)(255 - 255*progreso), 200 };

    const int N = 16;
    Vector3 puntosBase[N];
    for (int i = 0; i < N; i++) {
        float t = (float)i / N * 2.0f * PI;
        Vector3 offset = Vector3Add(
            Vector3Scale(right, cosf(t) * radioBase),
            Vector3Scale(up,    sinf(t) * radioBase)
        );
        puntosBase[i] = Vector3Add(miraObjetivoPos3D, offset);
    }

    for (int i = 0; i < N; i++) {
        DrawLine3D(apex, puntosBase[i], color);
    }
    for (int i = 0; i < N; i++) {
        DrawLine3D(puntosBase[i], puntosBase[(i + 1) % N], color);
    }
}


void Avion::ActualizarBoost(float dt) {
    boosteando = IsKeyDown(KEY_SPACE);

    if (boosteando) {
        velocidad += 6.0f * dt;
        if (velocidad > velocidadBase + velocidadBoost) velocidad = velocidadBase + velocidadBoost;
    } else {
        velocidad -= 3.0f * dt;
        if (velocidad < velocidadBase) velocidad = velocidadBase;
    }
}

void Avion::ActualizarOrbital(float dt) {
    Vector3 centro = planetaActual->GetPosicion();
    float radio = planetaActual->GetRadio();

    Vector3 delta = Vector3Subtract(posicion, centro);
    Vector3 normal = Vector3Normalize(delta);
    Vector3 tangent = Vector3Normalize(Vector3CrossProduct((Vector3){0,1,0}, normal));
    Vector3 bitangent = Vector3CrossProduct(normal, tangent);

    Vector3 move = {0,0,0};
    if (IsKeyDown(KEY_W)) move = Vector3Add(move, bitangent);
    if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, bitangent);
    if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, tangent);
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, tangent);

    float velocidadTotal = velocidad * (dt * 60.0f); 

    if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        delta = Vector3Add(delta, Vector3Scale(move, velocidadTotal));
        delta = Vector3Normalize(delta);
        posicion = Vector3Add(centro, Vector3Scale(delta, radio + 1.5f));
    }
}

void Avion::ActualizarVueloLibre(float dt) {
    float objetivoGiro = 0.0f;
    if (IsKeyDown(KEY_A)) objetivoGiro -= 90.0f;  
    if (IsKeyDown(KEY_D)) objetivoGiro += 90.0f;

    float aceleracionGiro = 6.0f; 
    rotacionVelocidad += (objetivoGiro - rotacionVelocidad) * std::min(1.0f, aceleracionGiro * dt);
    rotacionY += rotacionVelocidad * dt;

    float bancoObjetivo = Clamp(-rotacionVelocidad * 0.35f, -35.0f, 35.0f);
    anguloBanco += (bancoObjetivo - anguloBanco) * std::min(1.0f, 5.0f * dt);

    float velocidadTotal = velocidad;

    Vector3 direccion = { sinf(rotacionY * DEG2RAD) * velocidadTotal,
                          0,
                          cosf(rotacionY * DEG2RAD) * velocidadTotal };
    posicion.x += direccion.x * (dt * 60.0f); 
    posicion.z += direccion.z * (dt * 60.0f);

    float objetivoVertical = 0.0f;
    if (IsKeyDown(KEY_W)) objetivoVertical += 10.0f;
    if (IsKeyDown(KEY_S)) objetivoVertical -= 10.0f;

    float aceleracionVertical = 9.0f;
    velocidadVertical += (objetivoVertical - velocidadVertical) * std::min(1.0f, aceleracionVertical * dt);
    posicion.y += velocidadVertical * dt;

    float pitchObjetivo = Clamp(-velocidadVertical * 2.5f, -32.0f, 32.0f);
    anguloPitch += (pitchObjetivo - anguloPitch) * std::min(1.0f, 5.0f * dt);
}

void Avion::Actualizar(float dt, Vector3 objetivoPos, bool objetivoExiste) {
    ActualizarBoost(dt);

    if (planetaActual) {
        ActualizarOrbital(dt);
    } else {
        ActualizarVueloLibre(dt);
    }

    trail[trailIndex] = posicion;
    trailJitter[trailIndex] = {
        ((rand() % 100) / 200.0f) - 0.25f,
        ((rand() % 100) / 200.0f) - 0.25f
    };
    trailIndex = (trailIndex + 1) % MAX_TRAIL;

    if (IsKeyPressed(KEY_B)) {
        Vector3 velBomba = { sinf(rotacionY * DEG2RAD) * velocidad * 0.5f,
                             -0.1f,
                             cosf(rotacionY * DEG2RAD) * velocidad * 0.5f };
        bombas.push_back(Bomba(posicion, velBomba));
    }

    if (cooldownDisparo > 0.0f) cooldownDisparo -= dt;

    if (IsKeyPressed(KEY_N) && miraTrabada && cooldownDisparo <= 0.0f) {
        float rotRad = rotacionY * DEG2RAD;
        Vector3 right = { cosf(rotRad), 0.0f, -sinf(rotRad) };

        Vector3 origenIzq = Vector3Subtract(posicion, Vector3Scale(right, MISIL_SEPARACION_LANZAMIENTO));
        Vector3 origenDer = Vector3Add(posicion, Vector3Scale(right, MISIL_SEPARACION_LANZAMIENTO));

        misiles.push_back(Proyectil(origenIzq, miraObjetivoPos3D, MISIL_VELOCIDAD,
                                     /*homing=*/true, SKYBLUE, MISIL_ALCANCE_MAX, MISIL_RADIO_IMPACTO_INTERNO,
                                     MISIL_TAMANO, MISIL_TIEMPO_ERRANTE, MISIL_TIEMPO_RECTO));
        misiles.push_back(Proyectil(origenDer, miraObjetivoPos3D, MISIL_VELOCIDAD,
                                     /*homing=*/true, SKYBLUE, MISIL_ALCANCE_MAX, MISIL_RADIO_IMPACTO_INTERNO,
                                     MISIL_TAMANO, MISIL_TIEMPO_ERRANTE, MISIL_TIEMPO_RECTO));
        cooldownDisparo = MISIL_COOLDOWN;
    }

    Vector3 objetivoHoming = objetivoExiste ? objetivoPos : miraObjetivoPos3D;
    for (auto& m : misiles) {
        m.Actualizar(dt, objetivoHoming);
    }
    misiles.erase(std::remove_if(misiles.begin(), misiles.end(),
        [](const Proyectil& m) { return !m.EstaActivo(); }), misiles.end());

    for (auto &b : bombas) {
        if (planetaActual) {
            b.Actualizar(1.0f, 0.0f, true, planetaActual->GetPosicion(), planetaActual->GetRadio());
        } else {
            b.Actualizar(1.0f, 0.0f, false, {0,0,0}, 0.0f);
        }
    }
    
    bombas.erase(std::remove_if(bombas.begin(), bombas.end(),
        [this](Bomba &b){ 
            if (planetaActual) return b.Actualizar(1.0f, 0.0f, true, planetaActual->GetPosicion(), planetaActual->GetRadio());
            else return b.Actualizar(1.0f, 0.0f, false, {0,0,0}, 0.0f);
        }), bombas.end());
}

void Avion::Dibujar() {
    float ancho = 2.0f * escala;
    float largo = 4.0f * escala;
    float grosor = 0.5f * escala;

    Vector3 noseTop  = { 0.0f, grosor, largo };
    Vector3 leftTop  = { -ancho, grosor, -largo };
    Vector3 rightTop = { ancho, grosor, -largo };
    Vector3 noseBot  = { 0.0f, -grosor, largo };
    Vector3 leftBot  = { -ancho, -grosor, -largo };
    Vector3 rightBot = { ancho, -grosor, -largo };

    Color colorNeon = (Color){ 255, 0, 255, 255 };

    rlPushMatrix();
        rlTranslatef(posicion.x, posicion.y, posicion.z);
        rlRotatef(rotacionY, 0,1,0);
        if (!planetaActual) {
            rlRotatef(anguloPitch, 1,0,0);  
            rlRotatef(anguloBanco, 0,0,1);  
        }

        DrawTriangle3D(leftTop, noseTop, rightTop, colorNeon);
        DrawTriangle3D(leftBot, rightBot, noseBot, colorNeon);

        DrawTriangle3D(leftTop, leftBot, noseTop, colorNeon);
        DrawTriangle3D(leftBot, noseBot, noseTop, colorNeon);

        DrawTriangle3D(rightTop, noseTop, rightBot, colorNeon);
        DrawTriangle3D(rightBot, noseBot, noseTop, colorNeon);

        DrawTriangle3D(leftTop, rightTop, rightBot, colorNeon);
        DrawTriangle3D(leftTop, rightBot, leftBot, colorNeon);
    rlPopMatrix();

    for (int i = 0; i < MAX_TRAIL - 1; i++) {
        int idx1 = (trailIndex + i) % MAX_TRAIL;
        int idx2 = (trailIndex + i + 1) % MAX_TRAIL;
        Vector3 p1 = trail[idx1];
        Vector3 p2 = trail[idx2];

        p1.x += trailJitter[idx1].x; p1.z += trailJitter[idx1].y;
        p2.x += trailJitter[idx2].x; p2.z += trailJitter[idx2].y;

        unsigned char alpha = (unsigned char)(150 - i * 2);
        Color trailColor = (Color){ (unsigned char)(i*2), 255, (unsigned char)(255 - i*2), alpha };

        if (boosteando) {
            alpha = (unsigned char)std::min(255, (int)alpha + 90);
            trailColor = (Color){ (unsigned char)std::min(255, (int)trailColor.r + 80), 255,
                                   (unsigned char)std::min(255, (int)trailColor.b + 40), alpha };
        }

        DrawLine3D(p1, p2, trailColor);
    }

    if (boosteando) DibujarLineasDeVelocidad();

    for (auto &b : bombas) b.Dibujar();
    for (auto &m : misiles) m.Dibujar();
}

void Avion::DibujarLineasDeVelocidad() const {
    Vector3 dirViaje = { sinf(rotacionY * DEG2RAD), 0.0f, cosf(rotacionY * DEG2RAD) };

    Vector3 arriba = { 0.0f, 1.0f, 0.0f };
    Vector3 right  = Vector3CrossProduct(arriba, dirViaje);
    float rightLen = sqrtf(right.x*right.x + right.y*right.y + right.z*right.z);
    if (rightLen > 0.001f) right = Vector3Scale(right, 1.0f / rightLen);

    const int N_LINEAS = 10;
    for (int i = 0; i < N_LINEAS; i++) {
        float angulo = (rand() % 360) * DEG2RAD;
        float radioAnillo = 1.5f + (rand() % 100) / 100.0f * 2.5f;

        Vector3 offsetRadial = Vector3Add(
            Vector3Scale(right,  cosf(angulo) * radioAnillo),
            Vector3Scale(arriba, sinf(angulo) * radioAnillo)
        );

        float distAdelante = 2.0f + (rand() % 100) / 100.0f * 4.0f;
        float largoLinea   = 3.0f + (rand() % 100) / 100.0f * 5.0f;

        Vector3 inicio = Vector3Add(posicion, Vector3Add(offsetRadial, Vector3Scale(dirViaje,  distAdelante)));
        Vector3 fin    = Vector3Add(inicio,   Vector3Scale(dirViaje, -largoLinea));

        unsigned char alpha = (unsigned char)(120 + rand() % 100);
        Color colorLinea = (Color){ 200, 255, 255, alpha };
        DrawLine3D(inicio, fin, colorLinea);
    }
}

Vector3 Avion::GetPosicion() const { return posicion; }
float Avion::GetRotacionY() const { return rotacionY; }
void Avion::SetPosicion(const Vector3& pos) { posicion = pos; }
void Avion::SetPlanetaActual(Saturno* planeta) { planetaActual = planeta; }
void Avion::SetEscala(float esc) { escala = esc; }

bool Avion::RevisarImpacto(Vector3 objetivoPos, float radioObjetivo) {
    for (auto& m : misiles) {
        if (!m.EstaActivo()) continue;
        if (Vector3Distance(m.GetPosicion(), objetivoPos) <= radioObjetivo) {
            m.Desactivar();
            return true;
        }
    }
    return false;
}
