#ifndef AMBIENTE_H
#define AMBIENTE_H

#include "raylib.h"
#include "mapa.h"
#include <vector>
#include "rlgl.h"

struct Gota {
    Vector3 posicion;
    float   velocidad;   
    float   driftX;      
};

struct Nube {
    Vector3 posicion;
    float   velocidad;
    float   radio;
    int     nParticulas;
    std::vector<Vector3> offsets;

    bool          esNieve;       
    std::vector<Gota> gotas;

    float tiempoHastaProximoRelampago;
    float brilloRelampago; 
    bool  relampagoSubiendo; 
};

class Ambiente {
public:
    Ambiente(const Mapa& mapa);

    void Actualizar(float dt, const Camera3D& cam);
    void Dibujar(const Camera3D& cam) const;

    void CargarShader();
    void DescargarShader();

    void RecalcularAgua();
    void ToggleMode();
    bool GetModoNatural() const { return modoNatural; }

private:
    const Mapa& mapa;
    bool modoNatural = false;

    Shader  shaderNube;
    Mesh    quadNube;
    Model   modeloNube;
    int     locTiempo, locUVOffset, locColorNube;
    int     locCamRight, locCamUp;
    int     locFasePuff, locBrilloPuff;
    bool    shaderNubeCargado = false;
    float   tiempoShader = 0.0f;

    std::vector<std::pair<int,int>> celdasAgua;
    float tiempoAgua = 0.0f;

    std::vector<Nube> nubes;
    float alturaCloud   = 120.0f;
    float umbralNieve   = 130.0f;  

    float alturaFogMin = 15.0f;
    float alturaFogMax = 30.0f;

    float radioCullAmbiente = 60.0f; 

    static constexpr int PUFFS_POR_NUBE = 4; 
    static constexpr float RELAMPAGO_INTERVALO = 9.0f;  
    static constexpr float RELAMPAGO_DURACION_SUBIDA = 0.35f; 
    static constexpr float RELAMPAGO_DURACION_BAJADA = 0.5f;  

    void GenerarNubes();
    void ActualizarPrecipitacion(float dt);
    void ActualizarRelampagos(float dt, const Camera3D& cam);
    void DibujarPrecipitacion(const Camera3D& cam) const;
    void DibujarGrass(int x, int z, float worldX, float worldZ, float altTop) const;
    void DibujarNieve(int x, int z, float worldX, float worldZ, float altTop) const;
    void DibujarAgua(float worldX, float worldZ, float shimmer) const;
    void DibujarFog(const Camera3D& cam) const;
    void DibujarNubes() const;
    void DibujarNubesShader(const Camera3D& cam) const;

    Vector3 CeldaAMundo(int x, int z) const;
};

#endif
