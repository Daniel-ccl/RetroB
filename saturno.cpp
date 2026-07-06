
#include "saturno.h"
#include "rlgl.h"
#include <cmath>

Saturno::Saturno(Vector3 pos, float rPlaneta, float rAnillo, float gAnillo, float vRot,
                 float inc, Color cPlaneta, Color cAnillo) {
    posicion = pos;
    radioPlaneta = rPlaneta;
    radioAnillo = rAnillo;
    grosorAnillo = gAnillo;
    velocidadRot = vRot;
    anguloRot = 0.0f;
    inclinacion = inc;
    colorPlaneta = cPlaneta;
    colorAnillo = cAnillo;
}

void Saturno::Actualizar(float dt) {
    anguloRot += velocidadRot * dt;
    if (anguloRot > 360.0f) anguloRot -= 360.0f;
}

void Saturno::Dibujar() {
    const int meridianos = 24;  
    const int paralelos = 16;   

    for (int i = 0; i < meridianos; i++) {
        float theta1 = (i / (float)meridianos) * 2.0f * 3.14159f;
        float theta2 = ((i + 1) / (float)meridianos) * 2.0f * 3.14159f;

        for (int j = 0; j <= paralelos; j++) {
            float phi = (j / (float)paralelos) * 3.14159f; 

            Vector3 p1 = {
                radioPlaneta * sinf(phi) * cosf(theta1),
                radioPlaneta * cosf(phi),
                radioPlaneta * sinf(phi) * sinf(theta1)
            };

            Vector3 p2 = {
                radioPlaneta * sinf(phi) * cosf(theta2),
                radioPlaneta * cosf(phi),
                radioPlaneta * sinf(phi) * sinf(theta2)
            };

            float angRad = anguloRot * 3.14159f / 180.0f;
            Vector3 rp1 = {
                cosf(angRad) * p1.x - sinf(angRad) * p1.z,
                p1.y,
                sinf(angRad) * p1.x + cosf(angRad) * p1.z
            };
            Vector3 rp2 = {
                cosf(angRad) * p2.x - sinf(angRad) * p2.z,
                p2.y,
                sinf(angRad) * p2.x + cosf(angRad) * p2.z
            };

            rp1.x += posicion.x; rp1.y += posicion.y; rp1.z += posicion.z;
            rp2.x += posicion.x; rp2.y += posicion.y; rp2.z += posicion.z;

            DrawLine3D(rp1, rp2, colorPlaneta);
        }
    }

    rlPushMatrix();
        rlTranslatef(posicion.x, posicion.y, posicion.z);
        rlRotatef(inclinacion, 1, 0, 0);
        rlRotatef(anguloRot, 0, 1, 0);

        int lineCount = 64;
        for (int j = 0; j < lineCount; j++) {
            float angle = (360.0f / lineCount) * j * (3.14159f / 180.0f);
            Vector3 inner = { cosf(angle) * (radioAnillo - grosorAnillo * 0.5f), 0, sinf(angle) * (radioAnillo - grosorAnillo * 0.5f) };
            Vector3 outer = { cosf(angle) * (radioAnillo + grosorAnillo * 0.5f), 0, sinf(angle) * (radioAnillo + grosorAnillo * 0.5f) };
            DrawLine3D(inner, outer, colorAnillo);
        }
    rlPopMatrix();
}

Vector3 Saturno::GetNorthPole() const {
    return { posicion.x, posicion.y + radioPlaneta, posicion.z };
}















