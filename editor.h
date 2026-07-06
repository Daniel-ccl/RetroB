#ifndef EDITOR_H
#define EDITOR_H

#include "raylib.h"
#include "mapa.h"

class Editor {
public:
    Editor();

    bool Actualizar(Mapa& mapa, Vector2 mousePos, int screenWidth, int screenHeight,
                    Vector3 portalPlano, Vector3 saturnoPos);

    void Dibujar(Mapa& mapa, int screenWidth, int screenHeight,
                 Vector3 portalPlano, Vector3 saturnoPos) const;

private:
    int  brushSize    = 1;   
    int  alturaActiva = 1;   
    bool modoErase    = false;

    static const int N_SWATCHES = 6;   
    Rectangle SwatchRect(int i, int screenWidth, int screenHeight) const;

    void PintarCelda(Mapa& mapa, int gridX, int gridZ,
                     Vector3 portalPlano, Vector3 saturnoPos) const;

    bool MouseAGrid(Vector2 mousePos, int screenWidth, int screenHeight,
                    const Mapa& mapa, int& outX, int& outZ) const;
};

#endif
