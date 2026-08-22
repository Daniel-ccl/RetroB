#include "destello_lanzamiento.h"
#include "trayectoria.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

unsigned int DestelloLanzamiento::contadorSemillas = 1;

static Vector3 DireccionAleatoria(unsigned int semilla) {
    float a = RuidoHash(semilla) * 2.0f * PI;
    float b = RuidoHash(semilla * 13u + 7u);
    float z = b * 2.0f - 1.0f;
    float r = sqrtf(std::max(0.0f, 1.0f - z * z));
    return { r * cosf(a), z, r * sinf(a) };
}

DestelloLanzamiento::DestelloLanzamiento(Vector3 origenP, Vector3 direccionP, Color colorP) {
    posicion = origenP;
    direccion = direccionP;
    color = colorP;
    tiempo = 0.0f;
    duracionFlash = 0.12f;
    duracionTotal = 0.35f;

    int nAscuas = 6;
    ascuas.reserve(nAscuas);
    for (int i = 0; i < nAscuas; i++) {
        unsigned int semilla = contadorSemillas++;
        Vector3 dirAleatoria = DireccionAleatoria(semilla);
        Vector3 sesgo = Vector3Scale(direccion, -0.4f); // ascuas se quedan un poco atras del disparo
        Vector3 destinoDir = Vector3Normalize(Vector3Add(dirAleatoria, sesgo));
        float distancia = 0.8f + RuidoHash(semilla * 331u) * 1.2f;
        Vector3 destino = Vector3Add(origenP, Vector3Scale(destinoDir, distancia));
        ascuas.push_back({ origenP, destino, semilla });
    }
}

void DestelloLanzamiento::Actualizar(float dt) {
    tiempo += dt;
}

void DestelloLanzamiento::Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                                  const EfectoShaderLocs& locs) const {
    if (!EstaVivo()) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right   = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up      = Vector3CrossProduct(right, forward);
    SetShaderValue(shader, locs.camRight, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locs.camUp,    &up,    SHADER_UNIFORM_VEC3);

    Vector3 colorVec = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f };
    float pixelSize = 10.0f;
    SetShaderValue(shader, locs.color,     &colorVec,  SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locs.pixelSize, &pixelSize, SHADER_UNIFORM_FLOAT);
    float modoChispa = 0.0f;
    SetShaderValue(shader, locs.modo, &modoChispa, SHADER_UNIFORM_FLOAT);

    if (tiempo < duracionFlash) {
        float progresoFlash = tiempo / duracionFlash;
        SetShaderValue(shader, locs.progreso, &progresoFlash, SHADER_UNIFORM_FLOAT);
        float escalaFlash = 1.6f * (1.0f - progresoFlash * 0.4f);
        DrawModel(modeloQuad, posicion, escalaFlash, WHITE);
    }

    float tAscua = Clamp(tiempo / duracionTotal, 0.0f, 1.0f);
    SetShaderValue(shader, locs.progreso, &tAscua, SHADER_UNIFORM_FLOAT);
    for (const auto& a : ascuas) {
        Vector3 p = TrayectoriaAleatoria(a.origen, a.destino, tAscua, 0.3f, a.semilla);
        float escala = 0.35f * (1.0f - tAscua);
        if (escala > 0.01f) DrawModel(modeloQuad, p, escala, WHITE);
    }
}
