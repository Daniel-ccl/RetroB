#ifndef EXPLOSION_SAM_H
#define EXPLOSION_SAM_H

#include "efecto.h"
#include <vector>

class ExplosionSam : public Efecto {
public:
    ExplosionSam(Vector3 origen, Color color = (Color){255, 60, 30, 255});

    void Actualizar(float dt) override;
    void Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                const EfectoShaderLocs& locs) const override;
    bool EstaVivo() const override { return tiempo < duracionTotal; }

private:
    struct Fragmento {
        Vector3 origen;
        Vector3 destino;
        unsigned int semilla;
    };

    Color color;
    float tiempo;
    float duracionTotal;
    float distanciaConexion;

    std::vector<Fragmento> fragmentos;

    static unsigned int contadorSemillas;
};

#endif
