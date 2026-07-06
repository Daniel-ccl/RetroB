#include "bomba.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>
#include <algorithm> 

Bomba::Bomba(Vector3 pos, Vector3 vel) {
    posicion = pos;
    velocidad = vel;
    explotada = false;
}

bool Bomba::Actualizar(float dt, float sueloY, bool enPlaneta, Vector3 centroPlaneta, float radioPlaneta) {
    if (!explotada) {
        bool impacto = false;
        Vector3 upDir = {0.0f, 1.0f, 0.0f}; 

        if (enPlaneta) {
            Vector3 dirGravedad = Vector3Normalize(Vector3Subtract(centroPlaneta, posicion));
            velocidad.x += dirGravedad.x * 9.81f * dt;
            velocidad.y += dirGravedad.y * 9.81f * dt;
            velocidad.z += dirGravedad.z * 9.81f * dt;

            posicion.x += velocidad.x * dt;
            posicion.y += velocidad.y * dt;
            posicion.z += velocidad.z * dt;

            if (Vector3Distance(posicion, centroPlaneta) <= radioPlaneta) {
                Vector3 dirSuperficie = Vector3Normalize(Vector3Subtract(posicion, centroPlaneta));
                posicion = Vector3Add(centroPlaneta, Vector3Scale(dirSuperficie, radioPlaneta));
                impacto = true;
                
                upDir = dirSuperficie;
            }
        } else {
            velocidad.y -= 9.81f * dt;
            posicion.x += velocidad.x * dt;
            posicion.y += velocidad.y * dt;
            posicion.z += velocidad.z * dt;

            if (posicion.y <= sueloY) {
                posicion.y = sueloY;
                impacto = true;
                
                upDir = {0.0f, 1.0f, 0.0f};
            }
        }

        if (impacto) {
            explotada = true;
            flores.Generar(posicion, upDir);
        }
        return false;
    } else {
        return flores.Actualizar(dt);
    }
}

void Bomba::Dibujar() {
    if (!explotada) {
        DrawSphere(posicion, 0.3f, RED);
    } else {
        flores.Dibujar();
    }
}
