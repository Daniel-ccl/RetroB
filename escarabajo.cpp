#include "escarabajo.h"
#include "raymath.h"
#include <cmath>

Escarabajo::Escarabajo(Vector3 pos, float tam) {
    posicionBase = pos;
    posicionActual = pos;
    tamano = tam;
    tiempo = 0.0f;
    
    colorEsmeralda = (Color){ 0, 180, 150, 255 }; 
    colorDorado = (Color){ 255, 215, 0, 255 };    
}

void Escarabajo::Actualizar(float dt) {
    tiempo += dt;
    posicionActual.y = posicionBase.y + sinf(tiempo * 1.5f) * 3.0f; 
}

void Escarabajo::DibujarTriangulo(Vector3 v1, Vector3 v2, Vector3 v3, Color color) {
    Vector3 p1 = Vector3Add(posicionActual, Vector3Scale(v1, tamano));
    Vector3 p2 = Vector3Add(posicionActual, Vector3Scale(v2, tamano));
    Vector3 p3 = Vector3Add(posicionActual, Vector3Scale(v3, tamano));

    DrawTriangle3D(p1, p2, p3, color);
    DrawTriangle3D(p1, p3, p2, color); 

    DrawLine3D(p1, p2, Fade(BLACK, 0.6f));
    DrawLine3D(p2, p3, Fade(BLACK, 0.6f));
    DrawLine3D(p3, p1, Fade(BLACK, 0.6f));
}

void Escarabajo::DibujarPata(Vector3 origen, Vector3 codo, Vector3 fin) {
    Vector3 pOrigen = Vector3Add(posicionActual, Vector3Scale(origen, tamano));
    Vector3 pCodo = Vector3Add(posicionActual, Vector3Scale(codo, tamano));
    Vector3 pFin = Vector3Add(posicionActual, Vector3Scale(fin, tamano));

    DrawCylinderEx(pOrigen, pCodo, tamano * 0.03f, tamano * 0.03f, 5, colorDorado);
    DrawCylinderEx(pCodo, pFin, tamano * 0.03f, tamano * 0.01f, 5, colorDorado);
}


void Escarabajo::Dibujar() {

    // cabeza y cuerno
    Vector3 picoFrente = { 0.0f,  0.00f,  1.45f };

    // segmen
    Vector3 cuernoBaseIzq = {-0.18f, 0.40f, 0.95f};
    Vector3 cuernoBaseDer = { 0.18f, 0.40f, 0.95f};
    Vector3 cuernoMedioIzq = {-0.12f, 0.90f, 1.10f};
    Vector3 cuernoMedioDer = { 0.12f, 0.90f, 1.10f};
    Vector3 cuernoPuntaIzq = {-0.05f, 1.20f, 0.85f};
    Vector3 cuernoPuntaDer = { 0.05f, 1.20f, 0.85f};

    // anchuta
    Vector3 cabArr     = { 0.0f,  0.35f,  0.75f };
    Vector3 cabAbj     = { 0.0f, -0.18f,  0.70f };
    Vector3 cabIzq     = {-0.80f,  0.10f,  0.75f };
    Vector3 cabDer     = { 0.80f,  0.10f,  0.75f };

    // ojos
    Vector3 ojoIzqExt  = {-0.95f, 0.15f, 0.72f};
    Vector3 ojoDerExt  = { 0.95f, 0.15f, 0.72f};
    Color colorOjos = RED;

    // mandibula
    Vector3 mandIzqBaseInt = {-0.15f, -0.05f, 1.10f};
    Vector3 mandIzqBaseExt = {-0.45f, -0.02f, 1.00f};
    Vector3 mandIzqPunta   = {-0.28f, -0.08f, 1.75f};
    Vector3 mandDerBaseInt = { 0.15f, -0.05f, 1.10f};
    Vector3 mandDerBaseExt = { 0.45f, -0.02f, 1.00f};
    Vector3 mandDerPunta   = { 0.28f, -0.08f, 1.75f};

    // torax
    Vector3 toraxArr   = { 0.0f,  0.38f,  0.10f };
    Vector3 toraxIzq   = {-1.10f,  0.10f,  0.15f };
    Vector3 toraxDer   = { 1.10f,  0.10f,  0.15f };
    Vector3 toraxAbj   = { 0.0f, -0.25f,  0.10f };

    // caparazon
    Vector3 centroEspalda = { 0.0f, 0.45f, -0.35f };

    // secicon a
    Vector3 abdArrIzqA = {-0.35f, 0.45f, -0.55f };
    Vector3 abdArrDerA = { 0.35f, 0.45f, -0.55f };

    // seccion b
    Vector3 abdIzqMid = {-1.30f, 0.20f, -0.95f };
    Vector3 abdDerMid = { 1.30f, 0.20f, -0.95f };

    // seccion c
    Vector3 abdIzqBack = {-0.95f, 0.10f, -1.65f };
    Vector3 abdDerBack = { 0.95f, 0.10f, -1.65f };

    Vector3 abdAbj = { 0.0f, -0.20f, -1.10f };

    // cola
    Vector3 cola = { 0.0f, -0.08f, -2.10f };

    // mandibula
    DibujarTriangulo(mandIzqBaseInt, mandIzqBaseExt, mandIzqPunta, colorDorado);
    DibujarTriangulo(mandDerBaseInt, mandDerPunta, mandDerBaseExt, colorDorado);

    // cabeza
    DibujarTriangulo(picoFrente, cabIzq, cabArr, colorDorado);
    DibujarTriangulo(picoFrente, cabArr, cabDer, colorDorado);

    DibujarTriangulo(picoFrente, cabAbj, cabIzq, colorDorado);
    DibujarTriangulo(picoFrente, cabDer, cabAbj, colorDorado);

    // segmentacion
    DibujarTriangulo(cuernoBaseIzq, cuernoBaseDer, cuernoMedioIzq, colorDorado);
    DibujarTriangulo(cuernoBaseDer, cuernoMedioDer, cuernoMedioIzq, colorDorado);

    DibujarTriangulo(cuernoMedioIzq, cuernoMedioDer, cuernoPuntaIzq, colorDorado);
    DibujarTriangulo(cuernoMedioDer, cuernoPuntaDer, cuernoPuntaIzq, colorDorado);

    // ojo
    DibujarTriangulo(cabIzq, ojoIzqExt, cabArr, colorOjos);
    DibujarTriangulo(cabDer, cabArr, ojoDerExt, colorOjos);

    // torax
    DibujarTriangulo(cabArr, toraxIzq, toraxArr, colorEsmeralda);
    DibujarTriangulo(cabArr, toraxArr, toraxDer, colorEsmeralda);

    DibujarTriangulo(cabArr, cabIzq, toraxIzq, colorEsmeralda);
    DibujarTriangulo(cabArr, toraxDer, cabDer, colorEsmeralda);

    // caparazon izq
    DibujarTriangulo(toraxArr, toraxIzq, abdIzqMid, colorEsmeralda);
    DibujarTriangulo(toraxArr, abdIzqMid, abdArrIzqA, colorEsmeralda);
    DibujarTriangulo(abdArrIzqA, abdIzqMid, abdIzqBack, colorEsmeralda);
    DibujarTriangulo(toraxArr, abdArrIzqA, centroEspalda, colorEsmeralda);

    // cap derecho
    DibujarTriangulo(toraxArr, abdDerMid, toraxDer, colorEsmeralda);
    DibujarTriangulo(toraxArr, abdArrDerA, abdDerMid, colorEsmeralda);

    DibujarTriangulo(abdArrDerA, abdDerBack, abdDerMid, colorEsmeralda);

    DibujarTriangulo(toraxArr, centroEspalda, abdArrDerA, colorEsmeralda);

    // booty
    DibujarTriangulo(abdIzqBack, abdDerBack, cola, colorDorado);

    DibujarTriangulo(centroEspalda, abdIzqBack, cola, colorDorado);
    DibujarTriangulo(centroEspalda, cola, abdDerBack, colorDorado);

    // vientre
    DibujarTriangulo(cabAbj, toraxAbj, toraxIzq, colorEsmeralda);
    DibujarTriangulo(cabAbj, toraxDer, toraxAbj, colorEsmeralda);

    DibujarTriangulo(toraxAbj, abdAbj, toraxIzq, colorEsmeralda);
    DibujarTriangulo(toraxAbj, toraxDer, abdAbj, colorEsmeralda);

    DibujarTriangulo(abdAbj, abdIzqBack, cola, colorEsmeralda);
    DibujarTriangulo(abdAbj, cola, abdDerBack, colorEsmeralda);

    // patas
    // enfrente
    DibujarPata(
        toraxIzq,
        {-1.45f, 0.10f, 0.55f},
        {-1.90f, -0.70f, 0.95f}
    );

    DibujarPata(
        toraxDer,
        { 1.45f, 0.10f, 0.55f},
        { 1.90f, -0.70f, 0.95f}
    );

    // de enemdio
    DibujarPata(
        abdIzqMid,
        {-1.60f, 0.05f, -0.10f},
        {-2.00f, -0.75f, -0.10f}
    );

    DibujarPata(
        abdDerMid,
        { 1.60f, 0.05f, -0.10f},
        { 2.00f, -0.75f, -0.10f}
    );

    // traseras
    DibujarPata(
        abdIzqBack,
        {-1.30f, 0.00f, -1.10f},
        {-1.75f, -0.75f, -1.55f}
    );

    DibujarPata(
        abdDerBack,
        { 1.30f, 0.00f, -1.10f},
        { 1.75f, -0.75f, -1.55f}
    );
}






