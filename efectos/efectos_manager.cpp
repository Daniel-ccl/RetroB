#include "estela_misil.h"
#include "efectos_manager.h"
#include "destello_lanzamiento.h"
#include <algorithm>

EfectosManager& EfectosManager::Instancia() {
    static EfectosManager instancia;
    return instancia;
}

void EfectosManager::CargarShader() {
    shaderSpark = LoadShader("shaders/spark.vs", "shaders/spark.fs");
    locs.camRight  = GetShaderLocation(shaderSpark, "camRight");
    locs.camUp     = GetShaderLocation(shaderSpark, "camUp");
    locs.color     = GetShaderLocation(shaderSpark, "colorBase");
    locs.progreso  = GetShaderLocation(shaderSpark, "progreso");
    locs.pixelSize = GetShaderLocation(shaderSpark, "pixelSize");
    locs.modo = GetShaderLocation(shaderSpark, "modo"); 
    shaderSpark.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shaderSpark, "matModel");

    Mesh quad = GenMeshPlane(1.0f, 1.0f, 1, 1);
    modeloQuad = LoadModelFromMesh(quad);
    modeloQuad.materials[0].shader = shaderSpark;

    shaderCargado = true;
}

void EfectosManager::DescargarShader() {
    if (shaderCargado) {
        UnloadShader(shaderSpark);
        UnloadModel(modeloQuad);
        shaderCargado = false;
    }
}

void EfectosManager::Actualizar(float dt) {
    for (auto& e : activos) e->Actualizar(dt);
    activos.erase(std::remove_if(activos.begin(), activos.end(),
        [](const std::unique_ptr<Efecto>& e) { return !e->EstaVivo(); }), activos.end());
}

void EfectosManager::Dibujar(const Camera3D& cam) const {
    if (!shaderCargado) return;
    for (const auto& e : activos) e->Dibujar(cam, shaderSpark, modeloQuad, locs);
}

void EfectosManager::EmitirLanzamiento(Vector3 origen, Vector3 direccion, Color color) {
    activos.push_back(std::make_unique<DestelloLanzamiento>(origen, direccion, color));
}

void EfectosManager::EmitirEstela(Vector3 origen, Vector3 direccionMisil, Color colorFuego) {
    activos.push_back(std::make_unique<EstelaMisil>(origen, direccionMisil, colorFuego));
}


