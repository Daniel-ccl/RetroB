#ifndef EFECTO_H
#define EFECTO_H

#include "raylib.h"

struct EfectoShaderLocs {
    int camRight, camUp, color, progreso, pixelSize, modo;
};

class Efecto {
public:
    virtual ~Efecto() = default;
    virtual void Actualizar(float dt) = 0;
    virtual void Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                         const EfectoShaderLocs& locs) const = 0;
    virtual bool EstaVivo() const = 0;
};

#endif
