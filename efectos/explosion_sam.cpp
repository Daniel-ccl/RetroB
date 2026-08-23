#include "explosion_sam.h"
#include "trayectoria.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

unsigned int ExplosionSam::contadorSemillas = 90000;

static Vector3 DireccionExplosion(unsigned int semilla) {
    float a = RuidoHash(semilla) * 2.0f * PI;
    float b = RuidoHash(semilla * 13u + 7u);
    float z = b * 2.0f - 1.0f;
    float r = sqrtf(std::max(0.0f, 1.0f - z * z));
    return { r * cosf(a), z * 0.6f + 0.4f, r * sinf(a) };
}

ExplosionSam::ExplosionSam(Vector3 origenP, Color colorP) {
    color = colorP;
    tiempo = 0.0f;
    duracionTotal = 1.1f;
    distanciaConexion = 3.5f;

    int nFragmentos = 14;
    fragmentos.reserve(nFragmentos);
    for (int i = 0; i < nFragmentos; i++) {
        unsigned int semilla = contadorSemillas++;
        Vector3 dir = DireccionExplosion(semilla);
        float distancia = 3.0f + RuidoHash(semilla * 331u) * 5.0f;
        Vector3 destino = Vector3Add(origenP, Vector3Scale(dir, distancia));
        fragmentos.push_back({ origenP, destino, semilla });
    }
}

void ExplosionSam::Actualizar(float dt) {
    tiempo += dt;
}

void ExplosionSam::Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                           const EfectoShaderLocs& locs) const {
    if (!EstaVivo()) return;

    float t = Clamp(tiempo / duracionTotal, 0.0f, 1.0f);

    std::vector<Vector3> posiciones;
    posiciones.reserve(fragmentos.size());
    for (const auto& f : fragmentos) {
        posiciones.push_back(TrayectoriaAleatoria(f.origen, f.destino, t, 0.6f, f.semilla));
    }

    float alphaGlobal = 1.0f - t;
    float radioConexion = distanciaConexion * (1.0f - t * 0.6f);

    Color colorLinea = color;
    colorLinea.a = (unsigned char)(220 * alphaGlobal);
    Color colorTriangulo = color;
    colorTriangulo.a = (unsigned char)(70 * alphaGlobal);

    for (const auto& p : posiciones) {
        DrawSphere(p, 0.12f * (1.0f - t * 0.5f), WHITE);
    }

    for (size_t i = 0; i < posiciones.size(); i++) {
        for (size_t j = i + 1; j < posiciones.size(); j++) {
            float dij = Vector3Distance(posiciones[i], posiciones[j]);
            if (dij > radioConexion) continue;
            DrawLine3D(posiciones[i], posiciones[j], colorLinea);

            for (size_t k = j + 1; k < posiciones.size(); k++) {
                float dik = Vector3Distance(posiciones[i], posiciones[k]);
                float djk = Vector3Distance(posiciones[j], posiciones[k]);
                if (dik <= radioConexion && djk <= radioConexion) {
                    DrawTriangle3D(posiciones[i], posiciones[j], posiciones[k], colorTriangulo);
                }
            }
        }
    }
}
