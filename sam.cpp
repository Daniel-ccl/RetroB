#include "sam.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

Sam::Sam(Vector3 posBase, const std::string& idParam, const EnemyConfig& cfg) {
    id = idParam;
    posicionBase = posBase;
    posicionOjo  = Vector3Add(posBase, (Vector3){0.0f, 2.0f, 0.0f});

    anguloActual      = 0.0f;
    velocidadRotacion = 0.5f;

    anguloConoRad = (cfg.coneAngle >= 0 ? cfg.coneAngle : 25.0f) * DEG2RAD;
    alcance       =  cfg.coneRange >= 0 ? cfg.coneRange : 60.0f;

    estado               = SAM_BUSCANDO;
    tiempoLock           = 0.0f;
    tiempoLockNecesario  = 3.0f;
    cooldownDisparo      = 0.0f;

    radioImpacto = 1.5f;

    misilVelocidad  = cfg.missileSpeed  >= 0 ? cfg.missileSpeed  : 18.0f;
    misilesPorVolea = cfg.missileCount  >= 0 ? cfg.missileCount  : 3;
    danoMisil       = cfg.missileDamage >= 0 ? cfg.missileDamage : 30.0f;

    if (cfg.hp >= 0) salud = Salud(cfg.hp);
}

void Sam::SetPosicion(Vector3 posBase) {
    posicionBase = posBase;
    posicionOjo  = Vector3Add(posBase, (Vector3){0.0f, 2.0f, 0.0f});
}

Vector3 Sam::DireccionCono() const {
    return { sinf(anguloActual), 0.0f, cosf(anguloActual) };
}

static float AnguloHaciaAvion(Vector3 desde, Vector3 hacia) {
    float dx = hacia.x - desde.x;
    float dz = hacia.z - desde.z;
    return atan2f(dx, dz); 
}

static float DeltaAngular(float de, float a) {
    float delta = a - de;
    while (delta > PI)  delta -= 2.0f * PI;
    while (delta < -PI) delta += 2.0f * PI;
    return delta;
}

bool Sam::AvionEnCono(Vector3 posAvion) const {
    Vector3 hacia = Vector3Subtract(posAvion, posicionOjo);
    float dist = Vector3Length(hacia);
    if (dist > alcance || dist < 0.001f) return false;

    Vector3 haciaNorm = Vector3Scale(hacia, 1.0f / dist);
    Vector3 dir = DireccionCono();

    float cosAngulo = Vector3DotProduct(haciaNorm, dir);
    cosAngulo = Clamp(cosAngulo, -1.0f, 1.0f);
    float angulo = acosf(cosAngulo);

    return angulo <= anguloConoRad;
}

void Sam::DispararMisiles(Vector3 posAvion) {
    Vector3 dirCentral = Vector3Normalize(Vector3Subtract(posAvion, posicionOjo));

    for (int i = 0; i < misilesPorVolea; i++) {
        float angulo = 0.0f;
        if (misilesPorVolea > 1) {
            float t = (float)i / (float)(misilesPorVolea - 1); // 0..1
            angulo = (-MISIL_SPREAD_GRADOS + t * 2.0f * MISIL_SPREAD_GRADOS);
        }
        float rad = angulo * DEG2RAD;
        Vector3 dirRotada = {
            dirCentral.x * cosf(rad) + dirCentral.z * sinf(rad),
            dirCentral.y,
            -dirCentral.x * sinf(rad) + dirCentral.z * cosf(rad)
        };
        Vector3 objetivoFicticio = Vector3Add(posicionOjo, Vector3Scale(dirRotada, alcance));
        misiles.push_back(Proyectil(posicionOjo, objetivoFicticio, misilVelocidad,
                                     /*homing=*/true, (Color){255, 60, 30, 255}, MISIL_ALCANCE_MAX, radioImpacto,
                                     MISIL_TAMANO, MISIL_TIEMPO_ERRANTE, MISIL_TIEMPO_RECTO));
    }
}

bool Sam::ActualizarMisiles(float dt, Vector3 posAvion) {
    bool impactoEsteFrame = false;

    for (auto& m : misiles) {
        if (m.Actualizar(dt, posAvion)) impactoEsteFrame = true;
    }

    misiles.erase(std::remove_if(misiles.begin(), misiles.end(),
        [](const Proyectil& m) { return !m.EstaActivo(); }), misiles.end());

    return impactoEsteFrame;
}

bool Sam::Actualizar(float dt, Vector3 posAvion) {
    if (!salud.EstaVivo() && !floresGeneradas) {
        floresGeneradas = true;
        floresMuerte.Generar(posicionBase, {0.0f, 1.0f, 0.0f});
    }

    floresMuerte.Actualizar(dt);

    bool impacto = ActualizarMisiles(dt, posAvion);

    if (!salud.EstaVivo()) return impacto; 

    if (estado == SAM_BLOQUEADO) {
        float anguloObjetivo = AnguloHaciaAvion(posicionOjo, posAvion);
        float delta = DeltaAngular(anguloActual, anguloObjetivo);
        float maxPaso = velocidadRotacion * dt;
        if (fabsf(delta) <= maxPaso) anguloActual = anguloObjetivo;
        else anguloActual += (delta > 0.0f ? maxPaso : -maxPaso);
    } else {
        anguloActual += velocidadRotacion * dt;
    }
    if (anguloActual > 2.0f * PI) anguloActual -= 2.0f * PI;
    if (anguloActual < 0.0f)      anguloActual += 2.0f * PI;

    switch (estado) {
        case SAM_BUSCANDO: {
            if (AvionEnCono(posAvion)) {
                estado = SAM_BLOQUEADO;
                tiempoLock = 0.0f;
            }
            break;
        }
        case SAM_BLOQUEADO: {
            if (!AvionEnCono(posAvion)) {
                estado = SAM_BUSCANDO;
                tiempoLock = 0.0f;
                break;
            }
            tiempoLock += dt;
            if (tiempoLock >= tiempoLockNecesario) {
                DispararMisiles(posAvion);
                estado = SAM_DISPARADO;
                cooldownDisparo = 1.0f; 
            }
            break;
        }
        case SAM_DISPARADO: {
            cooldownDisparo -= dt;
            if (cooldownDisparo <= 0.0f) {
                estado = SAM_BUSCANDO;
                tiempoLock = 0.0f;
            }
            break;
        }
    }

    return impacto;
}

void Sam::Dibujar() const {
    if (salud.EstaVivo()) {
        Color colorCuerpo = (Color){ 255, 0, 255, 255 }; 

        DrawCylinderWires(posicionBase, 1.5f, 1.0f, 2.0f, 8, colorCuerpo);
        DrawSphere(posicionOjo, 0.4f, colorCuerpo);
    }

    DibujarMisiles();

    floresMuerte.Dibujar();
}

void Sam::DibujarBarraSalud() const {
    if (!salud.EstaVivo()) return; 

    float pct = salud.GetPorcentaje();

    float anchoTotal = 3.0f;
    float alto = 0.3f;
    Vector3 centro = Vector3Add(posicionOjo, (Vector3){0.0f, 2.5f, 0.0f});

    Vector3 bgIzq  = Vector3Add(centro, (Vector3){-anchoTotal/2.0f, 0.0f, 0.0f});
    Vector3 bgDer  = Vector3Add(centro, (Vector3){ anchoTotal/2.0f, 0.0f, 0.0f});
    Vector3 bgIzqB = Vector3Add(bgIzq,  (Vector3){0.0f, -alto, 0.0f});
    Vector3 bgDerB = Vector3Add(bgDer,  (Vector3){0.0f, -alto, 0.0f});
    Color colorFondo = (Color){40, 40, 40, 220};
    DrawTriangle3D(bgIzq, bgIzqB, bgDer, colorFondo);
    DrawTriangle3D(bgDer, bgIzqB, bgDerB, colorFondo);

    float anchoRelleno = anchoTotal * pct;
    Vector3 fIzq  = bgIzq;
    Vector3 fDer  = Vector3Add(bgIzq, (Vector3){anchoRelleno, 0.0f, 0.0f});
    Vector3 fIzqB = Vector3Add(fIzq, (Vector3){0.0f, -alto, 0.0f});
    Vector3 fDerB = Vector3Add(fDer, (Vector3){0.0f, -alto, 0.0f});

    unsigned char rojo  = (unsigned char)(255 * (1.0f - pct));
    unsigned char verde = (unsigned char)(255 * pct);
    Color colorRelleno = { rojo, verde, 0, 255 };

    DrawTriangle3D(fIzq, fIzqB, fDer, colorRelleno);
    DrawTriangle3D(fDer, fIzqB, fDerB, colorRelleno);
}

void Sam::DibujarMisiles() const {
    for (const auto& m : misiles) m.Dibujar();
}

void Sam::DibujarCono() const {
    if (!salud.EstaVivo()) return; 

    Color color;
    switch (estado) {
        case SAM_BUSCANDO:  color = GREEN;  break;
        case SAM_BLOQUEADO: color = YELLOW; break;
        case SAM_DISPARADO: color = RED;    break;
    }

    Vector3 dir = DireccionCono();

    Vector3 arriba = {0.0f, 1.0f, 0.0f};
    Vector3 right  = Vector3Normalize(Vector3CrossProduct(arriba, dir));
    Vector3 up     = Vector3CrossProduct(dir, right);

    float radioBase = alcance * tanf(anguloConoRad);
    Vector3 centroBase = Vector3Add(posicionOjo, Vector3Scale(dir, alcance));

    const int N = 16;
    Vector3 puntosBase[N];
    for (int i = 0; i < N; i++) {
        float t = (float)i / N * 2.0f * PI;
        Vector3 offset = Vector3Add(
            Vector3Scale(right, cosf(t) * radioBase),
            Vector3Scale(up,    sinf(t) * radioBase)
        );
        puntosBase[i] = Vector3Add(centroBase, offset);
    }

    for (int i = 0; i < N; i++) {
        DrawLine3D(posicionOjo, puntosBase[i], color);
    }
    for (int i = 0; i < N; i++) {
        DrawLine3D(puntosBase[i], puntosBase[(i + 1) % N], color);
    }
}
