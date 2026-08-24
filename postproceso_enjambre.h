#ifndef POSTPROCESO_ENJAMBRE_H
#define POSTPROCESO_ENJAMBRE_H

#include "raylib.h"
#include "ataque_enjambre.h"

class PostProcesoEnjambre {
public:
    PostProcesoEnjambre();

    bool Cargar(int ancho, int alto);
    void Descargar();

    void Actualizar(
        const AtaqueEnjambre& ataque,
        const Camera3D& camera,
        int ancho,
        int alto,
        bool habilitado
    );

    void Dibujar(
        const RenderTexture2D& escena
    ) const;

private:
    struct UbicacionesShader {
        int resolucion = -1;
        int centroEnjambre = -1;
        int direccionEnjambre = -1;
        int tiempo = -1;
        int fase = -1;
        int progreso = -1;
        int intensidad = -1;
    };

    Shader shader;
    UbicacionesShader ubicaciones;

    Vector2 centroAnterior;
    Vector2 direccionPantalla;

    bool cargado;
    bool teniaEfecto;
};

#endif
