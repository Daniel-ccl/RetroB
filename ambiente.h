#ifndef AMBIENTE_H
#define AMBIENTE_H

#include "mapa.h"
#include "raylib.h"
#include <cstdint>
#include <utility>
#include <vector>

struct Gota {
    Vector3 posicion;
    float velocidad;
    float driftX;
    float driftZ;
    float fase;
    float escala;
};

struct Nube {
    Vector3 posicion;
    float velocidad;
    float radio;
    int nParticulas;
    std::vector<Vector3> offsets;
    bool esNieve;
    std::vector<Gota> gotas;
    float tiempoHastaProximoRelampago;
    float brilloRelampago;
    bool relampagoSubiendo;
    unsigned int patronRelampago;
};

struct ParcheNatural {
    Model modelo{};
    Vector3 centro{};
    float radio = 0.0f;
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
    void DibujarCielo(int screenWidth, int screenHeight) const;

private:
    const Mapa& mapa;
    bool modoNatural = false;

    Shader shaderNube{};
    Model modeloNube{};
    int locTiempo = -1;
    int locUVOffset = -1;
    int locColorNube = -1;
    int locCamRight = -1;
    int locCamUp = -1;
    int locFasePuff = -1;
    int locBrilloPuff = -1;
    bool shaderNubeCargado = false;

    Shader shaderGrass{};
    Shader shaderMist{};
    int locTiempoGrass = -1;
    int locTiempoMist = -1;
    bool shadersNaturalezaCargados = false;

    std::vector<ParcheNatural> parchesGrass;
    std::vector<ParcheNatural> parchesMist;
    std::uint64_t revisionGeometria = ~std::uint64_t{0};

    std::vector<std::pair<int,int>> celdasAgua;
    std::vector<Color> coloresSuelo;
    std::vector<Nube> nubes;

    float tiempoAgua = 0.0f;
    float tiempoShader = 0.0f;
    float alturaFogMin = 15.0f;
    float alturaFogMax = 30.0f;
    float radioCullAmbiente = 60.0f;
    float distanciaNaturaleza = 160.0f;

    static constexpr int PARCHES_SUELO = 6;
    static constexpr int CELDAS_POR_PARCHE = 8;
    static constexpr int PUFFS_POR_NUBE = 3;
    static constexpr float RELAMPAGO_INTERVALO_MIN = 2.0f;
    static constexpr float RELAMPAGO_INTERVALO_MAX = 6.0f;
    static constexpr float RELAMPAGO_DURACION_SUBIDA = 0.04f;
    static constexpr float RELAMPAGO_DURACION_BAJADA = 0.16f;

    Color colorCieloArriba = (Color){40, 55, 90, 255};
    Color colorCieloHorizonte = (Color){170, 150, 140, 255};

    void GenerarNubes();
    void GenerarSuelo();
    void ReconstruirNaturaleza();
    void ConstruirParchesGrass();
    void ConstruirParchesMist();
    void DescargarParches(std::vector<ParcheNatural>& parches);
    bool ParcheVisible(const ParcheNatural& parche, const Camera3D& cam, float mitadFovxRad) const;

    void DibujarSueloBase() const;
    void DibujarParchesGrass(const Camera3D& cam, float mitadFovxRad) const;
    void DibujarParchesMist(const Camera3D& cam, float mitadFovxRad) const;
    void ActualizarPrecipitacion(float dt);
    void ActualizarRelampagos(float dt, const Camera3D& cam);
    void DibujarPrecipitacion(const Camera3D& cam) const;
    void DibujarNieve(int x, int z, float worldX, float worldZ) const;
    void DibujarAgua(float worldX, float worldZ, float shimmer) const;
    void DibujarNubes() const;
    void DibujarNubesShader(const Camera3D& cam) const;
    void DibujarRelampagos(const Camera3D& cam) const;
    Vector3 CeldaAMundo(int x, int z) const;
};

#endif
