#include "parche_flores.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

ParcheFlores::ParcheFlores() {
    upDir = {0.0f, 1.0f, 0.0f};
    rightDir = {1.0f, 0.0f, 0.0f};
    forwardDir = {0.0f, 0.0f, 1.0f};
}

void ParcheFlores::Generar(Vector3 origen, Vector3 normalSuperficie) {
    upDir = normalSuperficie;

    if (std::abs(upDir.y) > 0.99f) {
        rightDir = Vector3Normalize(Vector3CrossProduct(upDir, {0.0f, 0.0f, 1.0f}));
    } else {
        rightDir = Vector3Normalize(Vector3CrossProduct(upDir, {0.0f, 1.0f, 0.0f}));
    }
    forwardDir = Vector3Normalize(Vector3CrossProduct(rightDir, upDir));

    int nFlores = 5 + rand() % 6;
    float radio = 2.0f;
    for (int i = 0; i < nFlores; i++) {
        Flor f;

        float angulo = (rand() % 360) * (3.14159f / 180.0f);
        float dist = ((rand() % 100) / 100.0f) * radio;

        Vector3 offsetX = Vector3Scale(rightDir, std::cos(angulo) * dist);
        Vector3 offsetZ = Vector3Scale(forwardDir, std::sin(angulo) * dist);
        f.base = Vector3Add(origen, Vector3Add(offsetX, offsetZ));

        f.altura = 0.0f;
        f.alturaMax = 3.0f + (rand() % 5);
        f.crecimiento = 0.005f + ((rand() % 30) / 100.0f);
        f.tiempoVida = 2000.0f;
        f.colorRamas = (Color){0, static_cast<unsigned char>(200 + rand()%56), 0, 255};
        f.colorPetalos = (Color){
            static_cast<unsigned char>(150 + rand()%106),
            static_cast<unsigned char>(rand()%256),
            static_cast<unsigned char>(150 + rand()%106),
            255
        };
        flores.push_back(f);
    }
}

bool ParcheFlores::Actualizar(float dt) {
    for (auto &f : flores) {
        if (f.altura < f.alturaMax) f.altura += f.crecimiento * dt;
        f.tiempoVida -= dt;
    }
    flores.erase(std::remove_if(flores.begin(), flores.end(),
        [](Flor &f){ return f.tiempoVida <= 0.0f; }), flores.end());

    return flores.empty();
}

void ParcheFlores::Dibujar() const {
    for (auto &f : flores) {
        Vector3 top = Vector3Add(f.base, Vector3Scale(upDir, f.altura));

        DrawLine3D(f.base, top, f.colorRamas);

        int nRamas = 2 + rand() % 3;
        for (int i = 1; i <= nRamas; i++) {
            float y = f.altura * i / (nRamas+1);

            Vector3 start = Vector3Add(f.base, Vector3Scale(upDir, y));

            float randX = ((rand()%100)/100.0f -0.5f);
            float randY = 0.5f;
            float randZ = ((rand()%100)/100.0f -0.5f);

            Vector3 endOffset = Vector3Add(
                Vector3Scale(rightDir, randX),
                Vector3Add(Vector3Scale(upDir, randY), Vector3Scale(forwardDir, randZ))
            );
            Vector3 end = Vector3Add(start, endOffset);

            DrawLine3D(start, end, f.colorRamas);
        }

        int nPetalos = 5 + rand() % 3;
        for (int i = 0; i < nPetalos; i++) {
            float angle = i * (360.0f / nPetalos);
            float rad = angle * (3.14159f / 180.0f);

            Vector3 p1Offset = Vector3Add(
                Vector3Scale(rightDir, std::cos(rad) * 0.5f),
                Vector3Scale(forwardDir, std::sin(rad) * 0.5f)
            );
            Vector3 p1 = Vector3Add(top, p1Offset);

            Vector3 p2Offset = Vector3Add(
                Vector3Scale(rightDir, std::cos(rad) * 0.2f),
                Vector3Add(Vector3Scale(upDir, 0.3f), Vector3Scale(forwardDir, std::sin(rad) * 0.2f))
            );
            Vector3 p2 = Vector3Add(top, p2Offset);

            DrawTriangle3D(top, p1, p2, f.colorPetalos);
        }
    }
}
