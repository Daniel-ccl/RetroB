#ifndef ESCARABAJO_H
#define ESCARABAJO_H

#include "raylib.h"

class Escarabajo {
private:
    Vector3 posicionBase;
    Vector3 posicionActual;
    float tamano;
    float tiempo;
    Color colorEsmeralda;
    Color colorDorado;

    void DibujarTriangulo(Vector3 v1, Vector3 v2, Vector3 v3, Color color);
    void DibujarPata(Vector3 origen, Vector3 codo, Vector3 fin);

public:
    Escarabajo(Vector3 pos, float tam);
    void Actualizar(float dt);
    void Dibujar();
};

#endif
