#include "estela_misil.h"
#include "trayectoria.h"
#include "raymath.h"

unsigned int EstelaMisil::contadorSemillas = 5000;

EstelaMisil::EstelaMisil(Vector3 origenP, Vector3 direccionMisil, Color colorFuegoP) {
    origen = origenP;
    colorFuego = colorFuegoP;
    tiempo = 0.0f;
    duracionNucleo = 0.08f;
    duracionTotal  = 0.45f;
    semilla = contadorSemillas++;

    Vector3 atras = Vector3Scale(direccionMisil, -1.0f);
    destinoHumo = Vector3Add(origenP, Vector3Scale(atras, 0.6f));
}

void EstelaMisil::Actualizar(float dt) {
    tiempo += dt;
}

void EstelaMisil::Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                          const EfectoShaderLocs& locs) const {
    if (!EstaVivo()) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right   = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up      = Vector3CrossProduct(right, forward);
    SetShaderValue(shader, locs.camRight, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locs.camUp,    &up,    SHADER_UNIFORM_VEC3);

    if (tiempo < duracionNucleo) {
        float pNucleo = tiempo / duracionNucleo;
        Vector3 colorVec = { colorFuego.r / 255.0f, colorFuego.g / 255.0f, colorFuego.b / 255.0f };
        float pixelSize = 8.0f;
        float modo = 0.0f;
        SetShaderValue(shader, locs.color,     &colorVec,  SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, locs.pixelSize, &pixelSize, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, locs.progreso,  &pNucleo,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, locs.modo,      &modo,      SHADER_UNIFORM_FLOAT);
        DrawModel(modeloQuad, origen, 0.5f, WHITE);
    }

    float tHumo = Clamp(tiempo / duracionTotal, 0.0f, 1.0f);
    Vector3 pos = TrayectoriaAleatoria(origen, destinoHumo, tHumo, 0.15f, semilla);
    Vector3 colorHumo = { 0.55f, 0.55f, 0.6f };
    float pixelSizeHumo = 6.0f;
    float modoHumo = 1.0f;
    SetShaderValue(shader, locs.color,     &colorHumo,     SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locs.pixelSize, &pixelSizeHumo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, locs.progreso,  &tHumo,         SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, locs.modo,      &modoHumo,      SHADER_UNIFORM_FLOAT);
    float escalaHumo = 0.3f + 0.5f * tHumo;
    DrawModel(modeloQuad, pos, escalaHumo, WHITE);
}
