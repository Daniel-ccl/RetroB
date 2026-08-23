#ifndef EFECTOS_MANAGER_H
#define EFECTOS_MANAGER_H

#include "raylib.h"
#include "efecto.h"
#include <vector>
#include <memory>

class EfectosManager {
public:
    static EfectosManager& Instancia();

    void CargarShader();
    void DescargarShader();

    void Actualizar(float dt);
    void Dibujar(const Camera3D& cam) const;

    void EmitirLanzamiento(Vector3 origen, Vector3 direccion, Color color = SKYBLUE);
    void EmitirEstela(Vector3 origen, Vector3 direccionMisil, Color colorFuego = ORANGE);
    void EmitirExplosionSam(Vector3 origen, Color color = (Color){255, 60, 30, 255});

private:
    EfectosManager() = default;

    std::vector<std::unique_ptr<Efecto>> activos;

    Shader shaderSpark{};
    Model  modeloQuad{};
    EfectoShaderLocs locs{};
    bool shaderCargado = false;
};

#endif
