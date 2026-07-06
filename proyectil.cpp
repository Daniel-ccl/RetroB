#include "proyectil.h"
#include "raymath.h"
#include <cstdlib>

static float RandRango(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

Proyectil::Proyectil(Vector3 origen, Vector3 objetivoInicial, float velocidadEscalar,
                      bool homing, Color color, float alcanceMax, float radioImpacto,
                      float tamano, float tiempoErrante, float tiempoRecto) {
    this->posicion = origen;
    this->origen = origen;
    this->velocidadEscalar = velocidadEscalar;
    this->homing = homing;
    this->color = color;
    this->alcanceMax = alcanceMax;
    this->radioImpacto = radioImpacto;
    this->activo = true;
    this->tamano = tamano;

    this->tiempoErranteRestante = tiempoErrante;
    this->tiempoRectoRestante = tiempoRecto;
    this->posicionLanzamiento = origen;

    this->trailCount = 0;
    for (int i = 0; i < TRAIL_PUNTOS; i++) trail[i] = origen;

    Vector3 dir = Vector3Subtract(objetivoInicial, origen);
    float len = Vector3Length(dir);
    if (len > 0.001f) dir = Vector3Scale(dir, 1.0f / len);
    else dir = { 0.0f, 0.0f, 1.0f };

    this->velocidad = Vector3Scale(dir, velocidadEscalar);

    int nParticulas = 10;
    burst.reserve(nParticulas);
    for (int i = 0; i < nParticulas; i++) {
        ParticulaLanzamiento p;
        p.posicion = origen;
        Vector3 dirAleatoria = { RandRango(-1.0f, 1.0f), RandRango(-1.0f, 1.0f), RandRango(-1.0f, 1.0f) };
        float dlen = Vector3Length(dirAleatoria);
        if (dlen > 0.001f) dirAleatoria = Vector3Scale(dirAleatoria, 1.0f / dlen);
        float velPart = RandRango(2.0f, 6.0f) * tamano / 0.3f; 
        p.velocidad = Vector3Scale(dirAleatoria, velPart);
        p.vidaMax = RandRango(0.2f, 0.35f);
        p.vida = p.vidaMax;
        burst.push_back(p);
    }
}

void Proyectil::EmpujarTrail(Vector3 p) {
    for (int i = TRAIL_PUNTOS - 1; i > 0; i--) trail[i] = trail[i - 1];
    trail[0] = p;
    if (trailCount < TRAIL_PUNTOS) trailCount++;
}

void Proyectil::ActualizarBurst(float dt) {
    for (auto& p : burst) {
        if (p.vida <= 0.0f) continue;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, dt));
        p.vida -= dt;
    }
}

bool Proyectil::Actualizar(float dt, Vector3 objetivoActual) {
    ActualizarBurst(dt);

    if (!activo) return false;

    if (tiempoErranteRestante > 0.0f) {
        tiempoErranteRestante -= dt;

        float radioJitter = 0.4f * (tamano / 0.3f);
        posicion = Vector3Add(posicionLanzamiento, (Vector3){
            RandRango(-radioJitter, radioJitter),
            RandRango(-radioJitter, radioJitter),
            RandRango(-radioJitter, radioJitter)
        });

        EmpujarTrail(posicion);
        return false; 
    }

    bool enFaseRecta = (tiempoRectoRestante > 0.0f);
    if (enFaseRecta) {
        tiempoRectoRestante -= dt;
    } else if (homing) {
        Vector3 dir = Vector3Subtract(objetivoActual, posicion);
        float len = Vector3Length(dir);
        if (len > 0.001f) {
            dir = Vector3Scale(dir, 1.0f / len);
            velocidad = Vector3Scale(dir, velocidadEscalar);
        }
    }

    posicion = Vector3Add(posicion, Vector3Scale(velocidad, dt));
    EmpujarTrail(posicion);

    bool impacto = Vector3Distance(posicion, objetivoActual) <= radioImpacto;
    if (impacto) {
        activo = false;
        return true;
    }

    if (Vector3Distance(posicion, origen) > alcanceMax) {
        activo = false;
    }

    return false;
}

void Proyectil::DibujarBurst() const {
    for (const auto& p : burst) {
        if (p.vida <= 0.0f) continue;
        float fraccion = p.vida / p.vidaMax;
        unsigned char alpha = (unsigned char)(255 * fraccion);
        Color c = color;
        c.a = alpha;
        DrawSphere(p.posicion, tamano * 0.25f * fraccion, c);
    }
}

void Proyectil::Dibujar() const {
    DibujarBurst(); 

    if (!activo) return;

    Color glowExterior = color;
    glowExterior.a = 90;
    DrawSphere(posicion, tamano * 1.8f, glowExterior);
    DrawSphere(posicion, tamano, color);

    for (int i = 0; i < trailCount - 1; i++) {
        float fraccion = 1.0f - ((float)i / (float)TRAIL_PUNTOS);
        Color c = color;
        c.a = (unsigned char)(180 * fraccion);
        DrawLine3D(trail[i], trail[i + 1], c);
    }
}
