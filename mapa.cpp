#include "mapa.h"
#include "raymath.h"
#include "culling.h"
#include <algorithm> 
#include <cmath>

Mapa::Mapa(int tamaño, int celdas) {
    this->tamaño = tamaño;
    this->celdas = celdas;
    this->paso = (float)tamaño / (float)celdas;
    alturas.resize(celdas, std::vector<int>(celdas, 0));
}

bool Mapa::PosicionAIndice(Vector3 pos, int& gridX, int& gridZ) {
    float offsetX = pos.x + (tamaño / 2.0f);
    float offsetZ = pos.z + (tamaño / 2.0f);
    gridX = (int)(offsetX / paso);
    gridZ = (int)(offsetZ / paso);
    return (gridX >= 0 && gridX < celdas && gridZ >= 0 && gridZ < celdas);
}

float Mapa::AlturaSubCubo(int x, int z, int sx, int sz) const {
    auto getMapH = [&](int mx, int mz) -> float {
        if (mx < 0 || mx >= celdas || mz < 0 || mz >= celdas) return 0.0f;
        int h = alturas[mx][mz];
        return (float)h * 10.0f;
    };

    float fx = x + (sx + 0.5f) / 4.0f;
    float fz = z + (sz + 0.5f) / 4.0f;
    float cx = fx - 0.5f;
    float cz = fz - 0.5f;

    int x0 = (int)std::floor(cx);
    int z0 = (int)std::floor(cz);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float tx = cx - x0;
    float tz = cz - z0;

    float h00 = getMapH(x0, z0);
    float h10 = getMapH(x1, z0);
    float h01 = getMapH(x0, z1);
    float h11 = getMapH(x1, z1);

    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;
    float baseH = h0 * (1.0f - tz) + h1 * tz;

    if (baseH <= 0.5f) return 0.0f;

    float ruido = ((x * 13 + z * 27 + sx * 5 + sz * 7) % 15) / 5.0f;
    return baseH + ruido;
}

float Mapa::AlturaSuperficie(float worldX, float worldZ) const {
    float offset = tamaño / 2.0f;

    float gridXf = (worldX + offset) / paso;
    float gridZf = (worldZ + offset) / paso;

    int x = (int)std::floor(gridXf);
    int z = (int)std::floor(gridZf);

    float fracX = gridXf - x;
    float fracZ = gridZf - z;
    int sx = std::min(3, std::max(0, (int)(fracX * 4.0f)));
    int sz = std::min(3, std::max(0, (int)(fracZ * 4.0f)));

    return AlturaSubCubo(x, z, sx, sz);
}

void Mapa::Dibujar3D(Vector3 camPos, Vector3 camTarget, float fovyDeg, float aspect, float distanciaCull) {
    Color colorNeon = (Color){ 0, 255, 255, 200 };
    float offset = tamaño / 2.0f;
    float cullSq = distanciaCull * distanciaCull; 

    float mitadFovyRad = (fovyDeg * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * aspect);

    Camera3D camTmp = {0};
    camTmp.position = camPos;
    camTmp.target   = camTarget;

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            Vector3 pos = { (x * paso) - offset + (paso / 2.0f), 0.0f, (z * paso) - offset + (paso / 2.0f) };

            if (distanciaCull > 0.0f) {
                float ddx = pos.x - camPos.x;
                float ddz = pos.z - camPos.z;
                if ((ddx*ddx + ddz*ddz) > cullSq &&
                    !EsVisibleEnFrustum(pos, camTmp, mitadFovxRad)) continue;
            }

            if (alturas[x][z] == 0) {
                DrawCubeWires(pos, paso, 0.0f, paso, colorNeon); 
            }

            bool hasMountain = false;
            for(int dx = -1; dx <= 1; dx++) {
                for(int dz = -1; dz <= 1; dz++) {
                    int nx = x + dx, nz = z + dz;
                    if(nx >= 0 && nx < celdas && nz >= 0 && nz < celdas && alturas[nx][nz] > 0) {
                        hasMountain = true;
                    }
                }
            }

            if (hasMountain) {
                float subPaso = paso / 4.0f; 
                
                for(int sx = 0; sx < 4; sx++) {
                    for(int sz = 0; sz < 4; sz++) {

                        float finalH = AlturaSubCubo(x, z, sx, sz);

                        if (finalH > 0.0f) {
                            float ruido = ((x * 13 + z * 27 + sx * 5 + sz * 7) % 15) / 5.0f;

                            Vector3 subPos = {
                                (x * paso) - offset + (sx * subPaso) + (subPaso / 2.0f),
                                finalH / 2.0f,
                                (z * paso) - offset + (sz * subPaso) + (subPaso / 2.0f)
                            };

                            Color cubeColor = BLUE;
                            if (finalH > 15.0f) cubeColor = GREEN;
                            if (finalH > 25.0f) cubeColor = YELLOW;
                            if (finalH > 35.0f) cubeColor = ORANGE;
                            if (finalH > 45.0f) cubeColor = RED;
                            
                            if (ruido > 1.5f) {
                                cubeColor.r = std::max(0, cubeColor.r - 30);
                                cubeColor.g = std::max(0, cubeColor.g - 30);
                                cubeColor.b = std::max(0, cubeColor.b - 30);
                            }
                            
                            DrawCube(subPos, subPaso, finalH, subPaso, cubeColor);
                            DrawCubeWires(subPos, subPaso, finalH, subPaso, colorNeon);
                        }
                    }
                }
            }
        }
    }
}




