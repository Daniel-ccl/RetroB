#ifndef PAUSA_DECORACION_H
#define PAUSA_DECORACION_H

#include "raylib.h"
#include <vector>

class PausaDecoracion {
public:
    PausaDecoracion();

    void Actualizar(float dt);
    void Dibujar() const; 

private:
    struct Luna {
        float radioOrbita;   
        float alturaOrbita;  
        float velocidadOrbita; 
        float anguloActual;
        float radioLuna;     
        Color color;
    };

    float radioPlaneta;
    float anguloRotacionPlaneta;
    float velocidadRotacionPlaneta;

    float anguloRotacionAvion;
    float velocidadRotacionAvion;

    std::vector<Luna> lunas;

    void DibujarEsferaWire(Vector3 centro, float radio, Color color, int meridianos = 12, int paralelos = 8) const;
    void DibujarAvionDecorativo() const;
};

#endif
