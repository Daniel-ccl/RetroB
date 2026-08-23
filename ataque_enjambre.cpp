#include "ataque_enjambre.h"
#include "raymath.h"
#include "efectos/trayectoria.h"
#include <cstdlib>
#include <algorithm>

unsigned int AtaqueEnjambre::contadorSemillas = 40000;

static float RandRango(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

AtaqueEnjambre::AtaqueEnjambre() {
    cooldownRestante = 0.0f;
}

bool AtaqueEnjambre::Disparar(Vector3 origen, Enemy* objetivoInicial) {
    if (!ListoParaDisparar() || !objetivoInicial || !objetivoInicial->EstaVivo()) return false;
    if (Vector3Distance(origen, objetivoInicial->GetPosicion()) < DISTANCIA_MINIMA_ENJAMBRE) return false;

    for (int i = 0; i < PARTICULAS_INICIALES; i++) {
        Particula p;

        Vector3 offsetSpawn = { RandRango(-RADIO_SPAWN_ENJAMBRE, RADIO_SPAWN_ENJAMBRE),
                                 RandRango(-RADIO_SPAWN_ENJAMBRE, RADIO_SPAWN_ENJAMBRE),
                                 RandRango(-RADIO_SPAWN_ENJAMBRE, RADIO_SPAWN_ENJAMBRE) };
        p.posicion = Vector3Add(origen, offsetSpawn);
        p.origenReferencia = p.posicion;
        p.distanciaReferencia = Vector3Distance(p.posicion, objetivoInicial->GetPosicion());
        p.semilla = contadorSemillas++;

        p.estado = ATACANDO;
        p.objetivo = objetivoInicial;
        p.velocidad = { 0.0f, 0.0f, 0.0f };
        p.tiempoBusqueda = 0.0f;
        p.vida = 0.0f;
        p.activa = true;
        particulas.push_back(p);
    }

    cooldownRestante = COOLDOWN_ENJAMBRE;
    return true;
}

void AtaqueEnjambre::ActualizarParticula(Particula& p, float dt, std::vector<Enemy*>& enemigosVivos,
                                          std::vector<Particula>& nuevas) {
    switch (p.estado) {
        case ATACANDO: {
            if (!p.objetivo || !p.objetivo->EstaVivo()) {
                p.estado = BUSCANDO;
                p.tiempoBusqueda = 0.0f;
                break;
            }

            Vector3 objetivoPos = p.objetivo->GetPosicion();
            float distReal = Vector3Distance(p.posicion, objetivoPos);

            if (distReal <= RADIO_IMPACTO) {
                p.objetivo->RecibirDano(DANO_POR_PARTICULA);
                p.activa = false;
                break;
            }

            float t = 1.0f - Clamp(distReal / p.distanciaReferencia, 0.0f, 1.0f);
            Vector3 puntoVisual = TrayectoriaAleatoria(p.origenReferencia, objetivoPos, t, VARIACION_ENJAMBRE, p.semilla);

            Vector3 dir = Vector3Subtract(puntoVisual, p.posicion);
            float len = Vector3Length(dir);
            if (len > 0.001f) p.velocidad = Vector3Scale(dir, VELOCIDAD_PARTICULA / len);
            p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, dt));
            break;
        }
        case BUSCANDO: {
            p.tiempoBusqueda += dt;
            p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, dt));

            p.velocidad = Vector3Add(p.velocidad, (Vector3){
                RandRango(-4.0f, 4.0f), RandRango(-1.0f, 1.0f), RandRango(-4.0f, 4.0f)
            });
            float velLen = Vector3Length(p.velocidad);
            if (velLen > VELOCIDAD_PARTICULA) p.velocidad = Vector3Scale(p.velocidad, VELOCIDAD_PARTICULA / velLen);
            if (velLen < 0.001f) p.velocidad = { RandRango(-1.0f, 1.0f), RandRango(-0.3f, 0.3f), RandRango(-1.0f, 1.0f) };

            Enemy* masCercano = nullptr;
            float distMin = RADIO_BUSQUEDA_SECUNDARIA;
            for (Enemy* e : enemigosVivos) {
                if (!e || !e->EstaVivo()) continue;
                float d = Vector3Distance(p.posicion, e->GetPosicion());
                if (d < distMin) { distMin = d; masCercano = e; }
            }

            if (masCercano) {
                p.objetivo = masCercano;
                p.origenReferencia = p.posicion;
                p.distanciaReferencia = Vector3Distance(p.posicion, masCercano->GetPosicion());
                p.estado = ATACANDO;
            } else if (p.tiempoBusqueda >= TIEMPO_BUSQUEDA_SECUNDARIA) {
                Fragmentar(p, nuevas);
                p.activa = false;
            }
            break;
        }
        case ESTETICA: {
            p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, dt));
            p.vida -= dt;
            if (p.vida <= 0.0f) p.activa = false;
            break;
        }
    }
}

void AtaqueEnjambre::Fragmentar(const Particula& original, std::vector<Particula>& nuevas) const {
    for (int i = 0; i < PARTICULAS_POR_FRAGMENTACION; i++) {
        Particula np;
        np.posicion = original.posicion;

        Vector3 dirAleatoria = { RandRango(-1.0f, 1.0f), RandRango(-1.0f, 1.0f), RandRango(-1.0f, 1.0f) };
        float len = Vector3Length(dirAleatoria);
        if (len > 0.001f) dirAleatoria = Vector3Scale(dirAleatoria, 1.0f / len);

        np.velocidad = Vector3Scale(dirAleatoria, VELOCIDAD_PARTICULA * 0.4f);
        np.estado = ESTETICA;
        np.objetivo = nullptr;
        np.origenReferencia = np.posicion;
        np.distanciaReferencia = 1.0f;
        np.semilla = contadorSemillas++;
        np.tiempoBusqueda = 0.0f;
        np.vida = VIDA_ESTETICA;
        np.activa = true;
        nuevas.push_back(np);
    }
}

void AtaqueEnjambre::Actualizar(float dt, std::vector<Enemy*>& enemigosVivos) {
    if (cooldownRestante > 0.0f) cooldownRestante -= dt;

    std::vector<Particula> nuevas;
    size_t n = particulas.size();
    for (size_t i = 0; i < n; i++) {
        ActualizarParticula(particulas[i], dt, enemigosVivos, nuevas);
    }
    for (auto& np : nuevas) particulas.push_back(np);

    particulas.erase(std::remove_if(particulas.begin(), particulas.end(),
        [](const Particula& p) { return !p.activa; }), particulas.end());
}

bool AtaqueEnjambre::ParticulaConectable(const Particula& p) const {
    if (p.estado == ATACANDO && p.objetivo) {
        return Vector3Distance(p.posicion, p.objetivo->GetPosicion()) > RADIO_DESCONEXION_BLOB;
    }
    return true;
}

void AtaqueEnjambre::Dibujar() const {
    if (particulas.empty()) return;

    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& p : particulas) {
        Color c = (p.estado == ESTETICA) ? (Color){255, 140, 255, 190} : (Color){0, 255, 255, 255};
        float escala = (p.estado == ESTETICA) ? 0.20f : 0.35f;
        DrawSphere(p.posicion, escala, c);
    }

    for (size_t i = 0; i < particulas.size(); i++) {
        const Particula& pi = particulas[i];
        if (!ParticulaConectable(pi)) continue;

        for (size_t j = i + 1; j < particulas.size(); j++) {
            const Particula& pj = particulas[j];
            if (EsOfensiva(pi.estado) != EsOfensiva(pj.estado)) continue;
            if (!ParticulaConectable(pj)) continue;

            float d = Vector3Distance(pi.posicion, pj.posicion);
            if (d > DIST_MAX_CONEXION) continue;

            Color lc = EsOfensiva(pi.estado) ? (Color){0, 220, 255, 180} : (Color){255, 120, 255, 120};
            DrawLine3D(pi.posicion, pj.posicion, lc);
        }
    }

    EndBlendMode();
}
