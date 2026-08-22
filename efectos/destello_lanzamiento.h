#ifndef DESTELLO_LANZAMIENTO_H
#define DESTELLO_LANZAMIENTO_H

#include "efecto.h"
#include <vector>

class DestelloLanzamiento : public Efecto {
public:
    DestelloLanzamiento(Vector3 origen, Vector3 direccion, Color color = SKYBLUE);

    void Actualizar(float dt) override;
    void Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                const EfectoShaderLocs& locs) const override;
    bool EstaVivo() const override { return tiempo < duracionTotal; }

private:
    struct Ascua {
        Vector3 origen;
        Vector3 destino;
        unsigned int semilla;
    };

    Vector3 posicion;
    Vector3 direccion;
    Color   color;
    float   tiempo;
    float   duracionFlash;
    float   duracionTotal;

    std::vector<Ascua> ascuas;

    static unsigned int contadorSemillas;
};

#endif
