#ifndef BOMBA_H
#define BOMBA_H

#include "raylib.h"
#include "parche_flores.h"
#include <vector>

class Bomba {
private:
    Vector3 posicion;
    Vector3 velocidad;
    bool explotada;
    ParcheFlores flores;

public:
    Bomba(Vector3 pos, Vector3 vel);
    bool Actualizar(float dt, float sueloY, bool enPlaneta = false, Vector3 centroPlaneta = {0,0,0}, float radioPlaneta = 0.0f);
    void Dibujar();
};

#endif
