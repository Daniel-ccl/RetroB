#ifndef ESTELA_MISIL_H
#define ESTELA_MISIL_H

#include "efecto.h"

class EstelaMisil : public Efecto {
public:
    EstelaMisil(Vector3 origen, Vector3 direccionMisil, Color colorFuego);

    void Actualizar(float dt) override;
    void Dibujar(const Camera3D& cam, Shader shader, Model modeloQuad,
                const EfectoShaderLocs& locs) const override;
    bool EstaVivo() const override { return tiempo < duracionTotal; }

private:
    Vector3 origen;
    Vector3 destinoHumo;
    Color   colorFuego;
    float   tiempo;
    float   duracionNucleo;
    float   duracionTotal;
    unsigned int semilla;

    static unsigned int contadorSemillas;
};

#endif
