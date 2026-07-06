#ifndef PARCHE_FLORES_H
#define PARCHE_FLORES_H

#include "raylib.h"
#include <vector>

struct Flor {
    Vector3 base;
    float altura;          
    float alturaMax;      
    float crecimiento;    
    float tiempoVida;    
    Color colorRamas;   
    Color colorPetalos;   
};

class ParcheFlores {
public:
    ParcheFlores(); 

    void Generar(Vector3 origen, Vector3 normalSuperficie = {0.0f, 1.0f, 0.0f});

    bool Actualizar(float dt);

    void Dibujar() const;

    bool EstaVacio() const { return flores.empty(); }

private:
    std::vector<Flor> flores;

    Vector3 upDir;
    Vector3 rightDir;
    Vector3 forwardDir;
};

#endif
