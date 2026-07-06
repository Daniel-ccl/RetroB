#ifndef MAPA_H
#define MAPA_H

#include "raylib.h"
#include <vector>

class Mapa {
private:
    int tamaño;
    int celdas;
    float paso;

    std::vector<std::vector<int>> alturas;

public:
    Mapa(int tamaño = 400, int celdas = 20);

    void Dibujar3D(Vector3 camPos = {0,0,0}, Vector3 camTarget = {0,0,1},
                   float fovyDeg = 100.0f, float aspect = 16.0f/9.0f,
                   float distanciaCull = -1.0f);
    bool PosicionAIndice(Vector3 pos, int& gridX, int& gridZ);

    float AlturaSuperficie(float worldX, float worldZ) const;

    int   GetCeldas()  const { return celdas; }
    int   GetTamaño()  const { return tamaño; }   
    float GetPaso()    const { return paso; }      
    int   GetAltura(int x, int z) const { return alturas[x][z]; }
    void  SetAltura(int x, int z, int v) { alturas[x][z] = v; }

private:
    float AlturaSubCubo(int x, int z, int sx, int sz) const;
};

#endif
