#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <string>

class Enemy {
public:
    virtual ~Enemy() = default;

    virtual bool Actualizar(float dt, Vector3 posJugador) = 0;

    virtual void Dibujar() const = 0;
    virtual void DibujarBarraSalud() const = 0;

    virtual Vector3 GetPosicion() const = 0;
    virtual void SetPosicion(Vector3 pos) = 0;

    virtual void RecibirDano(float cantidad) = 0;
    virtual bool EstaVivo() const = 0;
    virtual float GetPorcentajeSalud() const = 0;

    virtual const std::string& GetId() const = 0;
};

#endif
