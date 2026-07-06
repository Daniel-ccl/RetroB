#include "pausa_decoracion.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

PausaDecoracion::PausaDecoracion() {
    radioPlaneta = 3.0f;
    anguloRotacionPlaneta = 0.0f;
    velocidadRotacionPlaneta = 0.3f; 

    anguloRotacionAvion = 0.0f;
    velocidadRotacionAvion = 0.6f;

    lunas.push_back({  6.0f,  0.0f, 0.5f, 0.0f,  0.45f, (Color){0, 255, 255, 255}   }); 
    lunas.push_back({  8.0f,  1.8f, 0.32f, 1.4f, 0.6f,  (Color){255, 0, 255, 255}   }); 
    lunas.push_back({ 10.5f, -1.2f, 0.22f, 3.1f, 0.7f,  (Color){255, 255, 0, 255}   }); 
    lunas.push_back({  4.5f,  2.6f, 0.8f,  5.0f, 0.3f,  (Color){0, 255, 120, 255}   }); 
}

void PausaDecoracion::Actualizar(float dt) {
    anguloRotacionPlaneta += velocidadRotacionPlaneta * dt;
    anguloRotacionAvion   += velocidadRotacionAvion * dt;

    for (auto& l : lunas) {
        l.anguloActual += l.velocidadOrbita * dt;
        if (l.anguloActual > 2.0f * PI) l.anguloActual -= 2.0f * PI;
    }

    if (anguloRotacionPlaneta > 2.0f * PI) anguloRotacionPlaneta -= 2.0f * PI;
    if (anguloRotacionAvion   > 2.0f * PI) anguloRotacionAvion   -= 2.0f * PI;
}

void PausaDecoracion::DibujarEsferaWire(Vector3 centro, float radio, Color color, int meridianos, int paralelos) const {
    for (int i = 0; i < meridianos; i++) {
        float theta1 = (i / (float)meridianos) * 2.0f * PI;
        float theta2 = ((i + 1) / (float)meridianos) * 2.0f * PI;

        for (int j = 0; j <= paralelos; j++) {
            float phi = (j / (float)paralelos) * PI;

            Vector3 p1 = {
                radio * sinf(phi) * cosf(theta1),
                radio * cosf(phi),
                radio * sinf(phi) * sinf(theta1)
            };
            Vector3 p2 = {
                radio * sinf(phi) * cosf(theta2),
                radio * cosf(phi),
                radio * sinf(phi) * sinf(theta2)
            };

            DrawLine3D(Vector3Add(centro, p1), Vector3Add(centro, p2), color);
        }
    }
}

void PausaDecoracion::DibujarAvionDecorativo() const {
    Vector3 posicionDecorativa = { 0.0f, 8.0f, 0.0f };
    float escala = 1.1f;

    float ancho = 2.0f * escala;
    float largo = 4.0f * escala;
    float grosor = 0.5f * escala;

    Vector3 noseTop  = { 0.0f, grosor, largo };
    Vector3 leftTop  = { -ancho, grosor, -largo };
    Vector3 rightTop = { ancho, grosor, -largo };
    Vector3 noseBot  = { 0.0f, -grosor, largo };
    Vector3 leftBot  = { -ancho, -grosor, -largo };
    Vector3 rightBot = { ancho, -grosor, -largo };

    Color colorNeon = (Color){ 255, 0, 255, 255 };

    rlPushMatrix();
        rlTranslatef(posicionDecorativa.x, posicionDecorativa.y, posicionDecorativa.z);
        rlRotatef(anguloRotacionAvion * RAD2DEG, 0, 1, 0);

        DrawTriangle3D(leftTop, noseTop, rightTop, colorNeon);
        DrawTriangle3D(leftBot, rightBot, noseBot, colorNeon);

        DrawTriangle3D(leftTop, leftBot, noseTop, colorNeon);
        DrawTriangle3D(leftBot, noseBot, noseTop, colorNeon);

        DrawTriangle3D(rightTop, noseTop, rightBot, colorNeon);
        DrawTriangle3D(rightBot, noseBot, noseTop, colorNeon);

        DrawTriangle3D(leftTop, rightTop, rightBot, colorNeon);
        DrawTriangle3D(leftTop, rightBot, leftBot, colorNeon);
    rlPopMatrix();
}

void PausaDecoracion::Dibujar() const {
    Vector3 centroPlaneta = { 0.0f, 0.0f, 0.0f };

    rlPushMatrix();
        rlTranslatef(centroPlaneta.x, centroPlaneta.y, centroPlaneta.z);
        rlRotatef(anguloRotacionPlaneta * RAD2DEG, 0, 1, 0);
        DibujarEsferaWire({0,0,0}, radioPlaneta, (Color){0, 200, 255, 200});
    rlPopMatrix();

    for (const auto& l : lunas) {
        Vector3 posLuna = {
            cosf(l.anguloActual) * l.radioOrbita,
            l.alturaOrbita,
            sinf(l.anguloActual) * l.radioOrbita
        };
        DibujarEsferaWire(posLuna, l.radioLuna, l.color, 8, 6);
    }

    DibujarAvionDecorativo();
}
