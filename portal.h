
#ifndef PORTAL_H
#define PORTAL_H

#include "raylib.h"
#include <vector>

struct Particula {
    Vector3 posicion;
    Vector3 velocidad;
    float vida;
    Color   colorFuego;     
    float   tamano;         
    float   flickerTimer;   
};

class Portal {
private:
    Vector3 posicion;
    float radio;           
    Color colorAnillo;
    std::vector<Particula> particulas;
    float cooldown;        

    void GenerarParticulas(int cantidad);

public:
    Portal(Vector3 pos, float radio, Color color);

    bool DetectarEntrada(Vector3 posAvion); 
    void Actualizar(float dt);
    void Dibujar() const;

    Vector3 GetPosicion() const { return posicion; }
    float GetRadio() const { return radio; }
};

#endif
