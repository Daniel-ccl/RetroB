#include "ambiente.h"
#include "raymath.h"
#include "culling.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static float cellRand(int x, int z, int salt = 0) {
    unsigned int h = (unsigned int)(x * 1619 + z * 31337 + salt * 3119);
    h = (h ^ (h >> 16)) * 0x45d9f3b;
    h = (h ^ (h >> 16)) * 0x45d9f3b;
    h ^= (h >> 16);
    return (float)(h & 0xFFFF) / 65535.0f;
}

Ambiente::Ambiente(const Mapa& mapa) : mapa(mapa) {
    RecalcularAgua();
    GenerarNubes();
}

void Ambiente::ToggleMode() { modoNatural = !modoNatural; }

void Ambiente::RecalcularAgua() {
    celdasAgua.clear();
    int celdas = mapa.GetCeldas();
    const int dx[] = { 1,-1, 0, 0 };
    const int dz[] = { 0, 0, 1,-1 };
    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            if (mapa.GetAltura(x, z) != 0) continue;
            int vecinos = 0;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], nz = z + dz[d];
                if (nx < 0 || nx >= celdas || nz < 0 || nz >= celdas) vecinos++;
                else if (mapa.GetAltura(nx, nz) > 0) vecinos++;
            }
            if (vecinos >= 3) celdasAgua.push_back({x, z});
        }
    }
}

void Ambiente::GenerarNubes() {
    nubes.clear();
    int   celdas = mapa.GetCeldas();
    float paso   = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            if (mapa.GetAltura(x, z) == 0) continue;
            if (cellRand(x, z, 99) > 0.4f) continue;

            Nube n;
            float wx = (x * paso) - offset + paso * 0.5f;
            float wz = (z * paso) - offset + paso * 0.5f;
            wx += (cellRand(x, z, 1) - 0.5f) * paso;
            wz += (cellRand(x, z, 2) - 0.5f) * paso;

            n.posicion    = { wx, alturaCloud + cellRand(x,z,3)*10.0f, wz };
            n.velocidad   = 1.5f + cellRand(x, z, 4) * 2.0f;
            n.radio       = paso * (0.6f + cellRand(x, z, 5) * 0.8f);
            n.nParticulas = 6 + (int)(cellRand(x, z, 6) * 8);
            n.esNieve     = (n.posicion.y >= umbralNieve);

            n.tiempoHastaProximoRelampago = cellRand(x, z, 15) * RELAMPAGO_INTERVALO;
            n.brilloRelampago   = 0.0f;
            n.relampagoSubiendo = false;

            n.offsets.reserve(n.nParticulas);
            for (int i = 0; i < n.nParticulas; i++) {
                Vector3 off = {
                    (cellRand(x*100+i, z, 7) - 0.5f) * n.radio * 2.0f,
                    (cellRand(x*100+i, z, 8) - 0.5f) * n.radio * 0.4f,
                    (cellRand(x*100+i, z, 9) - 0.5f) * n.radio * 1.5f
                };
                n.offsets.push_back(off);
            }

            int nGotas = 30;
            n.gotas.resize(nGotas);
            for (int i = 0; i < nGotas; i++) {
                float rx = (cellRand(x*200+i, z, 10) - 0.5f) * n.radio * 2.0f;
                float rz = (cellRand(x*200+i, z, 11) - 0.5f) * n.radio * 2.0f;
                float startY = n.posicion.y - cellRand(x*200+i, z, 12) * n.posicion.y;
                n.gotas[i].posicion  = { n.posicion.x + rx, startY, n.posicion.z + rz };
                n.gotas[i].velocidad = n.esNieve ? (4.0f  + cellRand(x*200+i, z, 13) * 3.0f)
                                                 : (18.0f + cellRand(x*200+i, z, 13) * 8.0f);
                n.gotas[i].driftX    = n.esNieve ? (cellRand(x*200+i, z, 14) - 0.5f) * 1.5f
                                                 : 0.0f;
            }

            nubes.push_back(n);
        }
    }
}

void Ambiente::ActualizarPrecipitacion(float dt) {
    for (auto& n : nubes) {
        for (auto& g : n.gotas) {
            g.posicion.y -= g.velocidad * dt;
            g.posicion.x += g.driftX   * dt;

            if (g.posicion.y <= 0.0f) {
                float rx = (((float)rand() / RAND_MAX) - 0.5f) * n.radio * 2.0f;
                float rz = (((float)rand() / RAND_MAX) - 0.5f) * n.radio * 2.0f;
                g.posicion  = { n.posicion.x + rx, n.posicion.y, n.posicion.z + rz };
            }
        }
    }
}

void Ambiente::ActualizarRelampagos(float dt, const Camera3D& cam) {
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    for (auto& n : nubes) {
        if (n.esNieve) continue; 

        bool enProgreso = n.brilloRelampago > 0.0f;

        if (!enProgreso) {
            n.tiempoHastaProximoRelampago -= dt;
            if (n.tiempoHastaProximoRelampago <= 0.0f) {
                n.tiempoHastaProximoRelampago = RELAMPAGO_INTERVALO;

                bool visible = EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente);
                if (visible) {
                    n.relampagoSubiendo = true;
                    n.brilloRelampago = 0.001f; 
                }
            }
        } else {
            if (n.relampagoSubiendo) {
                n.brilloRelampago += dt / RELAMPAGO_DURACION_SUBIDA;
                if (n.brilloRelampago >= 1.0f) {
                    n.brilloRelampago = 1.0f;
                    n.relampagoSubiendo = false;
                }
            } else {
                n.brilloRelampago -= dt / RELAMPAGO_DURACION_BAJADA;
                if (n.brilloRelampago <= 0.0f) {
                    n.brilloRelampago = 0.0f; 
                }
            }
        }
    }
}

void Ambiente::Actualizar(float dt, const Camera3D& cam) {
    tiempoAgua   += dt;
    tiempoShader += dt;

    float mapaSize = mapa.GetTamaño();
    float halfMap  = mapaSize / 2.0f;

    for (auto& n : nubes) {
        n.posicion.x += n.velocidad * dt;
        if (n.posicion.x >  halfMap) n.posicion.x -= mapaSize;
        if (n.posicion.x < -halfMap) n.posicion.x += mapaSize;

        for (auto& g : n.gotas) {
            g.posicion.x += n.velocidad * dt;
            if (g.posicion.x >  halfMap) g.posicion.x -= mapaSize;
            if (g.posicion.x < -halfMap) g.posicion.x += mapaSize;
        }
    }

    ActualizarPrecipitacion(dt);
    ActualizarRelampagos(dt, cam);
}

void Ambiente::DibujarPrecipitacion(const Camera3D& cam) const {
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    for (const auto& n : nubes) {
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;

        for (const auto& g : n.gotas) {
            if (n.esNieve) {
                DrawSphere(g.posicion, 0.18f, (Color){220, 235, 255, 200});
            } else {
                Vector3 top = { g.posicion.x, g.posicion.y + 1.2f, g.posicion.z };
                DrawLine3D(top, g.posicion, (Color){130, 180, 255, 180});
            }
        }
    }
}

Vector3 Ambiente::CeldaAMundo(int x, int z) const {
    float paso   = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;
    return { (x * paso) - offset + paso * 0.5f, 0.0f, (z * paso) - offset + paso * 0.5f };
}

void Ambiente::DibujarGrass(int x, int z, float worldX, float worldZ, float altTop) const {
    const int N = 8;
    for (int i = 0; i < N; i++) {
        float ox = (cellRand(x, z, i*10+0) - 0.5f) * mapa.GetPaso() * 0.85f;
        float oz = (cellRand(x, z, i*10+1) - 0.5f) * mapa.GetPaso() * 0.85f;
        float h  = 0.8f + cellRand(x, z, i*10+2) * 1.4f;
        Vector3 base = { worldX + ox, altTop,     worldZ + oz };
        Vector3 tip  = { worldX + ox, altTop + h, worldZ + oz };
        unsigned char g = 180 + (unsigned char)(cellRand(x, z, i*10+3) * 75);
        DrawLine3D(base, tip, (Color){ 0, g, 40, 220 });
    }
}

void Ambiente::DibujarNieve(int x, int z, float worldX, float worldZ, float altTop) const {
    const int N = 12;
    float paso = mapa.GetPaso();
    for (int i = 0; i < N; i++) {
        float ox   = (cellRand(x, z, i*7+0) - 0.5f) * paso * 0.9f;
        float oz   = (cellRand(x, z, i*7+1) - 0.5f) * paso * 0.9f;
        float size = 0.15f + cellRand(x, z, i*7+2) * 0.25f;
        Vector3 pos = { worldX + ox, altTop + size * 0.5f, worldZ + oz };
        DrawCube(pos, size, size * 0.5f, size, WHITE);
    }
}

void Ambiente::DibujarAgua(float worldX, float worldZ, float shimmer) const {
    float paso = mapa.GetPaso();
    float y    = 0.3f + sinf(shimmer) * 0.15f;
    float h    = paso * 0.48f;
    Vector3 tl = { worldX - h, y, worldZ - h };
    Vector3 tr = { worldX + h, y, worldZ - h };
    Vector3 bl = { worldX - h, y, worldZ + h };
    Vector3 br = { worldX + h, y, worldZ + h };
    unsigned char blue = (unsigned char)(180 + sinf(shimmer * 1.3f) * 40);
    Color agua = { 0, (unsigned char)(200 + sinf(shimmer*0.7f)*30), blue, 180 };
    DrawTriangle3D(tl, bl, tr, agua);
    DrawTriangle3D(tr, bl, br, agua);
    Color borde = { 0, 255, 255, 120 };
    DrawLine3D(tl, tr, borde); DrawLine3D(tr, br, borde);
    DrawLine3D(br, bl, borde); DrawLine3D(bl, tl, borde);
}

void Ambiente::DibujarFog(const Camera3D& cam) const {
    int celdas = mapa.GetCeldas();
    float paso = mapa.GetPaso();
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));
    const int ddx[] = {1,-1,0,0};
    const int ddz[] = {0,0,1,-1};
    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            int h = mapa.GetAltura(x, z);
            if (h == 0) continue;
            bool esBorde = false;
            for (int d = 0; d < 4; d++) {
                int nx = x+ddx[d], nz = z+ddz[d];
                if (nx<0||nx>=celdas||nz<0||nz>=celdas) continue;
                if (mapa.GetAltura(nx,nz) != h) { esBorde = true; break; }
            }
            if (!esBorde) continue;
            float altTop = (float)h * 10.0f;
            if (altTop < alturaFogMin || altTop > alturaFogMax) continue;
            Vector3 wp = CeldaAMundo(x, z);
            if (!EsVisibleEnFrustum(wp, cam, mitadFovxRad, 0.20f, radioCullAmbiente)) continue;
            float h2 = paso * 0.45f;
            Vector3 tl = { wp.x-h2, altTop, wp.z-h2 };
            Vector3 tr = { wp.x+h2, altTop, wp.z-h2 };
            Vector3 bl = { wp.x-h2, altTop, wp.z+h2 };
            Vector3 br = { wp.x+h2, altTop, wp.z+h2 };
            Color fog = { 200, 220, 255, 55 };
            DrawTriangle3D(tl, bl, tr, fog);
            DrawTriangle3D(tr, bl, br, fog);
        }
    }
}

void Ambiente::DibujarNubes() const {
    for (const auto& n : nubes) {
        for (int i = 0; i < n.nParticulas; i++) {
            Vector3 p = Vector3Add(n.posicion, n.offsets[i]);
            float r = n.radio * (0.3f + 0.1f * i);
            DrawCube(p, r, r * 0.35f, r, (Color){220, 230, 255, 180});
            DrawCubeWires(p, r, r * 0.35f, r, (Color){180, 200, 255, 80});
        }
    }
}

void Ambiente::DibujarNubesShader(const Camera3D& cam) const {
    if (!shaderNubeCargado) { DibujarNubes(); return; }

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right   = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up      = Vector3CrossProduct(right, forward);

    SetShaderValue(shaderNube, locCamRight,  &right,        SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderNube, locCamUp,     &up,           SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderNube, locTiempo,    &tiempoShader, SHADER_UNIFORM_FLOAT);

    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    for (const auto& n : nubes) {
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;

        Vector3 colorBase = { 0.92f, 0.95f, 1.0f };
        Vector3 colorFlash = { 1.3f, 1.25f, 1.05f }; 
        Vector3 colorFinal = Vector3Lerp(colorBase, colorFlash, n.brilloRelampago);
        SetShaderValue(shaderNube, locColorNube, &colorFinal, SHADER_UNIFORM_VEC3);

        int nPuffs = std::min(PUFFS_POR_NUBE, n.nParticulas);

        for (int i = 0; i < nPuffs; i++) {
            Vector3 posPuff = Vector3Add(n.posicion, n.offsets[i]);

            float uvOff[2] = { posPuff.x * 0.002f, posPuff.z * 0.002f };
            SetShaderValue(shaderNube, locUVOffset, uvOff, SHADER_UNIFORM_VEC2);

            float fase = (float)i * 1.7f + n.posicion.x * 0.01f;
            SetShaderValue(shaderNube, locFasePuff, &fase, SHADER_UNIFORM_FLOAT);

            float brilloBase = (i == 0) ? 1.0f : 0.75f;
            float brillo = brilloBase + n.brilloRelampago * (1.0f - brilloBase) * 0.8f;
            SetShaderValue(shaderNube, locBrilloPuff, &brillo, SHADER_UNIFORM_FLOAT);

            float s = n.radio * (1.6f + i * 0.3f); 
            DrawModel(modeloNube, posPuff, s, WHITE);
        }
    }
}

void Ambiente::CargarShader() {
    shaderNube = LoadShader("shaders/cloud.vs", "shaders/cloud.fs");
    locTiempo     = GetShaderLocation(shaderNube, "tiempo");
    locUVOffset   = GetShaderLocation(shaderNube, "uvOffset");
    locColorNube  = GetShaderLocation(shaderNube, "colorNube");
    locCamRight   = GetShaderLocation(shaderNube, "camRight");
    locCamUp      = GetShaderLocation(shaderNube, "camUp");
    locFasePuff   = GetShaderLocation(shaderNube, "fasePuff");
    locBrilloPuff = GetShaderLocation(shaderNube, "brilloPuff");
    shaderNube.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shaderNube, "matModel");
    quadNube   = GenMeshPlane(1.0f, 1.0f, 1, 1);
    modeloNube = LoadModelFromMesh(quadNube);
    modeloNube.materials[0].shader = shaderNube;
    shaderNubeCargado = true;
}

void Ambiente::DescargarShader() {
    if (shaderNubeCargado) {
        UnloadShader(shaderNube);
        UnloadModel(modeloNube);
    }
}

void Ambiente::Dibujar(const Camera3D& cam) const {
    if (!modoNatural) return;

    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    int celdas = mapa.GetCeldas();
    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            int h = mapa.GetAltura(x, z);
            if (h == 0) continue;
            Vector3 wp = CeldaAMundo(x, z);
            if (!EsVisibleEnFrustum(wp, cam, mitadFovxRad, 0.20f, radioCullAmbiente)) continue;
            float altTop = (float)h * 10.0f;
            if (h <= 2) DibujarGrass(x, z, wp.x, wp.z, altTop);
            if (h >= 4) DibujarNieve(x, z, wp.x, wp.z, altTop);
        }
    }

    for (const auto& [ax, az] : celdasAgua) {
        Vector3 wp = CeldaAMundo(ax, az);
        if (!EsVisibleEnFrustum(wp, cam, mitadFovxRad, 0.20f, radioCullAmbiente)) continue;
        DibujarAgua(wp.x, wp.z, tiempoAgua + cellRand(ax, az, 0) * 6.28f);
    }

    DibujarFog(cam);
    DibujarNubesShader(cam);
    DibujarPrecipitacion(cam);
}
