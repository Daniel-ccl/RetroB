#ifndef MAPA_H
#define MAPA_H

#include "raylib.h"
#include <cstdint>
#include <vector>

class Mapa {
private:
    int tamaño;
    int celdas;
    float paso;

    std::vector<std::vector<int>> alturas;
    std::vector<float> alturaFina;
    std::vector<float> alturaNatural;
    std::vector<Vector3> segmentosWireRetro;

    bool sucio = true;
    bool modelosCargados = false;
    std::uint64_t revision = 0;

    Model modeloSolidoRetro{};
    Model modeloNatural{};
    Model modeloCierreNatural{};

    float AlturaBilinealCoarse(float fx, float fz) const;
    float AlturaSubCubo(int x, int z, int sx, int sz) const;
    float AlturaFinaEn(int x, int sx, int z, int sz) const;

    void RecalcularAlturaFina();
    void ConstruirModelos();
    void ConstruirModeloRetro();
    void ConstruirModeloNatural();
    void ConstruirCierreNatural();
    void DescargarModelos();

public:
    Mapa(int tamaño = 400, int celdas = 20);
    Mapa(const Mapa& otro);
    Mapa& operator=(const Mapa& otro);
    ~Mapa();

    void Dibujar3D(bool modoNatural);

    bool PosicionAIndice(Vector3 pos, int& gridX, int& gridZ);
    float AlturaSuperficie(float worldX, float worldZ) const;

    int   GetCeldas()  const { return celdas; }
    int   GetTamaño()  const { return tamaño; }
    float GetPaso()    const { return paso; }
    int   GetAltura(int x, int z) const { return alturas[x][z]; }
    std::uint64_t GetRevision() const { return revision; }
    void  SetAltura(int x, int z, int v);
};

#endif
