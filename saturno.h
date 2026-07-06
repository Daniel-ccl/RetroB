
#ifndef SATURNO_H
#define SATURNO_H

#include "raylib.h"

class Saturno {
private:
    Vector3 posicion;      
    float radioPlaneta;    
    float radioAnillo;    
    float grosorAnillo;    
    float anguloRot;       
    float velocidadRot;    
    float inclinacion;    
    Color colorPlaneta;   
    Color colorAnillo;     

public:
    Saturno(Vector3 pos, float rPlaneta, float rAnillo, float gAnillo, float vRot,
            float inc = 25.0f,
            Color cPlaneta = ORANGE, Color cAnillo = LIGHTGRAY);

    void Actualizar(float dt);
    void Dibujar();

    Vector3 GetPosicion() const { return posicion; }
    float GetRadio() const { return radioPlaneta; }

    Vector3 GetNorthPole() const;
};

#endif
