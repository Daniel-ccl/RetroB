#ifndef PROYECTIL_H
#define PROYECTIL_H

#include "raylib.h"
#include <vector>

struct ParticulaLanzamiento {
    Vector3 posicion;
    Vector3 velocidad;
    float vida;
    float vidaMax;
};

class Proyectil {
public:
    Proyectil(Vector3 origen, Vector3 objetivoInicial, float velocidadEscalar,
              bool homing, Color color, float alcanceMax = 500.0f, float radioImpacto = 1.5f,
              float tamano = 0.3f, float tiempoErrante = 0.0f, float tiempoRecto = 0.0f);

    bool Actualizar(float dt, Vector3 objetivoActual);

    void Dibujar() const;

    bool EstaActivo() const { return activo; }
    Vector3 GetPosicion() const { return posicion; }

    void Desactivar() { activo = false; }

private:
    Vector3 posicion;
    Vector3 origen;
    Vector3 velocidad;
    float velocidadEscalar;
    bool homing;
    Color color;
    float alcanceMax;
    float radioImpacto;
    bool activo;

    float tamano;

    float tiempoErranteRestante;
    Vector3 posicionLanzamiento;

    float tiempoRectoRestante;

    static const int TRAIL_PUNTOS = 8;
    Vector3 trail[TRAIL_PUNTOS];
    int trailCount; 

    std::vector<ParticulaLanzamiento> burst;

    void ActualizarBurst(float dt);
    void DibujarBurst() const;
    void EmpujarTrail(Vector3 p);
};

#endif
