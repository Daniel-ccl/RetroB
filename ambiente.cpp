#include "ambiente.h"
#include "culling.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

struct GeometriaTemporal {
    std::vector<float> vertices;
    std::vector<float> texcoords;
    std::vector<unsigned char> colores;
};

static float cellRand(int x, int z, int salt = 0) {
	std::uint32_t h =
		static_cast<std::uint32_t>(x) * 1619u +
		static_cast<std::uint32_t>(z) * 31337u +
		static_cast<std::uint32_t>(salt) * 3119u;
	h = (h ^ (h >> 16)) * 0x45d9f3b;
	h = (h ^ (h >> 16)) * 0x45d9f3b;
	h ^= (h >> 16);
	return (float)(h & 0xFFFF) / 65535.0f;
}

static void AgregarVertice(GeometriaTemporal& geometria, Vector3 posicion, Vector2 uv, Color color) {
    geometria.vertices.push_back(posicion.x);
    geometria.vertices.push_back(posicion.y);
    geometria.vertices.push_back(posicion.z);
    geometria.texcoords.push_back(uv.x);
    geometria.texcoords.push_back(uv.y);
    geometria.colores.push_back(color.r);
    geometria.colores.push_back(color.g);
    geometria.colores.push_back(color.b);
    geometria.colores.push_back(color.a);
}

static void AgregarTriangulo(GeometriaTemporal& geometria,
                             Vector3 a, Vector2 uvA,
                             Vector3 b, Vector2 uvB,
                             Vector3 c, Vector2 uvC,
                             Color colorA, Color colorB, Color colorC) {
    AgregarVertice(geometria, a, uvA, colorA);
    AgregarVertice(geometria, b, uvB, colorB);
    AgregarVertice(geometria, c, uvC, colorC);
}

static Model CrearModelo(const GeometriaTemporal& geometria, Shader shader) {
    Mesh mesh = {0};
    mesh.vertexCount = (int)(geometria.vertices.size() / 3);
    mesh.triangleCount = mesh.vertexCount / 3;
    mesh.vertices = (float*)MemAlloc(geometria.vertices.size() * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(geometria.texcoords.size() * sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc(geometria.colores.size() * sizeof(unsigned char));
    memcpy(mesh.vertices, geometria.vertices.data(), geometria.vertices.size() * sizeof(float));
    memcpy(mesh.texcoords, geometria.texcoords.data(), geometria.texcoords.size() * sizeof(float));
    memcpy(mesh.colors, geometria.colores.data(), geometria.colores.size() * sizeof(unsigned char));
    UploadMesh(&mesh, false);
    Model modelo = LoadModelFromMesh(mesh);
    modelo.materials[0].shader = shader;
    return modelo;
}

Ambiente::Ambiente(const Mapa& mapa) : mapa(mapa) {
    RecalcularAgua();
    GenerarNubes();
    GenerarSuelo();
}

void Ambiente::ToggleMode() {
    modoNatural = !modoNatural;
}

void Ambiente::RecalcularAgua() {
    celdasAgua.clear();
    int celdas = mapa.GetCeldas();
    const int dx[] = {1, -1, 0, 0};
    const int dz[] = {0, 0, 1, -1};

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            if (mapa.GetAltura(x, z) != 0) continue;
            int vecinos = 0;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int nz = z + dz[d];
                if (nx < 0 || nx >= celdas || nz < 0 || nz >= celdas) vecinos++;
                else if (mapa.GetAltura(nx, nz) > 0) vecinos++;
            }
            if (vecinos >= 3) celdasAgua.push_back({x, z});
        }
    }
}

void Ambiente::GenerarNubes() {
    nubes.clear();
    int celdas = mapa.GetCeldas();
    float paso = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            int alturaCelda = mapa.GetAltura(x, z);
            if (alturaCelda != 4 && alturaCelda != 5) continue;
            if (cellRand(x, z, 99) > 0.4f) continue;

            Nube n;
            float wx = (x * paso) - offset + paso * 0.5f;
            float wz = (z * paso) - offset + paso * 0.5f;

            float superficie = mapa.AlturaSuperficie(wx, wz);
            float alturaNube = superficie + paso * (2.2f + cellRand(x, z, 3) * 0.8f);
            n.posicion = {wx, alturaNube, wz};
            n.velocidad = 0.0f;
            n.radio = paso * (0.6f + cellRand(x, z, 5) * 0.8f);
            n.nParticulas = PUFFS_POR_NUBE;
            n.esNieve = alturaCelda == 5;
            n.tiempoHastaProximoRelampago = 0.5f + cellRand(x, z, 15) * 2.5f;
            n.brilloRelampago = 0.0f;
            n.relampagoSubiendo = false;
            n.patronRelampago = (unsigned int)(cellRand(x, z, 19) * 1000000.0f);

            n.offsets.reserve(n.nParticulas);
            for (int i = 0; i < n.nParticulas; i++) {
                Vector3 offsetNube = {
                    (cellRand(x*100+i, z, 7) - 0.5f) * n.radio * 2.0f,
                    (cellRand(x*100+i, z, 8) - 0.5f) * n.radio * 0.4f,
                    (cellRand(x*100+i, z, 9) - 0.5f) * n.radio * 1.5f
                };
                n.offsets.push_back(offsetNube);
            }

            int nGotas = 30;
            n.gotas.resize(nGotas);
            float radioPrecipitacion = paso * 0.42f;
            for (int i = 0; i < nGotas; i++) {
                int clave = x*200+i;
                float rx = (cellRand(clave, z, 10) - 0.5f) * radioPrecipitacion * 2.0f;
                float rz = (cellRand(clave, z, 11) - 0.5f) * radioPrecipitacion * 2.0f;
                float suelo = mapa.AlturaSuperficie(n.posicion.x + rx, n.posicion.z + rz);
                float startY = suelo + cellRand(clave, z, 12) * std::max(1.0f, n.posicion.y - suelo);
                Gota& gota = n.gotas[i];
                gota.posicion = {n.posicion.x + rx, startY, n.posicion.z + rz};
                gota.velocidad = n.esNieve
                    ? 4.0f + cellRand(clave, z, 13) * 3.0f
                    : 18.0f + cellRand(clave, z, 13) * 8.0f;
                gota.driftX = n.esNieve
                    ? (cellRand(clave, z, 14) - 0.5f) * 1.4f
                    : 0.25f + cellRand(clave, z, 14) * 0.45f;
                gota.driftZ = n.esNieve
                    ? (cellRand(clave, z, 16) - 0.5f) * 1.2f
                    : (cellRand(clave, z, 16) - 0.5f) * 0.25f;
                gota.fase = cellRand(clave, z, 17) * 6.2831853f;
                gota.escala = 0.75f + cellRand(clave, z, 18) * 0.5f;
            }

            nubes.push_back(std::move(n));
        }
    }
}

void Ambiente::GenerarSuelo() {
    coloresSuelo.clear();
    coloresSuelo.reserve(PARCHES_SUELO * PARCHES_SUELO);
    for (int i = 0; i < PARCHES_SUELO * PARCHES_SUELO; i++) {
        unsigned char r = 90 + rand() % 30;
        unsigned char g = 65 + rand() % 20;
        unsigned char b = 45 + rand() % 15;
        coloresSuelo.push_back((Color){r, g, b, 255});
    }
}

void Ambiente::DescargarParches(std::vector<ParcheNatural>& parches) {
    for (auto& parche : parches) UnloadModel(parche.modelo);
    parches.clear();
}

void Ambiente::ConstruirParchesGrass() {
    int celdas = mapa.GetCeldas();
    float paso = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;

    for (int inicioX = 0; inicioX < celdas; inicioX += CELDAS_POR_PARCHE) {
        for (int inicioZ = 0; inicioZ < celdas; inicioZ += CELDAS_POR_PARCHE) {
            int finX = std::min(celdas, inicioX + CELDAS_POR_PARCHE);
            int finZ = std::min(celdas, inicioZ + CELDAS_POR_PARCHE);
            GeometriaTemporal geometria;

            for (int x = inicioX; x < finX; x++) {
                for (int z = inicioZ; z < finZ; z++) {
                    int nivel = mapa.GetAltura(x, z);
                    if (nivel == 0 || nivel > 2) continue;
                    float worldX = (x * paso) - offset + paso * 0.5f;
                    float worldZ = (z * paso) - offset + paso * 0.5f;

                    for (int i = 0; i < 8; i++) {
                        float ox = (cellRand(x, z, i*10+0) - 0.5f) * paso * 0.85f;
                        float oz = (cellRand(x, z, i*10+1) - 0.5f) * paso * 0.85f;
                        float altura = 0.8f + cellRand(x, z, i*10+2) * 1.4f;
                        float ancho = 0.08f + cellRand(x, z, i*10+4) * 0.08f;
                        float angulo = cellRand(x, z, i*10+5) * 6.2831853f;
                        float fase = cellRand(x, z, i*10+6);
                        float px = worldX + ox;
                        float pz = worldZ + oz;
                        float baseY = mapa.AlturaSuperficie(px, pz) + 0.03f;
                        Vector3 lateral = {std::cos(angulo) * ancho, 0.0f, std::sin(angulo) * ancho};
                        Vector3 izquierda = {px - lateral.x, baseY, pz - lateral.z};
                        Vector3 derecha = {px + lateral.x, baseY, pz + lateral.z};
                        Vector3 punta = {px, baseY + altura, pz};
                        unsigned char verde = 180 + (unsigned char)(cellRand(x, z, i*10+3) * 75);
                        Color base = {0, (unsigned char)(verde * 0.72f), 30, 220};
                        Color cima = {0, verde, 40, 220};
                        Vector2 uvBase = {fase, 0.0f};
                        Vector2 uvCima = {fase, 1.0f};
                        AgregarTriangulo(geometria, izquierda, uvBase, derecha, uvBase, punta, uvCima, base, base, cima);
                        AgregarTriangulo(geometria, derecha, uvBase, izquierda, uvBase, punta, uvCima, base, base, cima);
                    }
                }
            }

            if (geometria.vertices.empty()) continue;
            float x0 = inicioX * paso - offset;
            float x1 = finX * paso - offset;
            float z0 = inicioZ * paso - offset;
            float z1 = finZ * paso - offset;
            float centroX = (x0 + x1) * 0.5f;
            float centroZ = (z0 + z1) * 0.5f;
            float centroY = mapa.AlturaSuperficie(centroX, centroZ);
            float radio = std::sqrt((x1-x0)*(x1-x0) + (z1-z0)*(z1-z0)) * 0.5f + 3.0f;
            parchesGrass.push_back({CrearModelo(geometria, shaderGrass), {centroX, centroY, centroZ}, radio});
        }
    }
}

void Ambiente::ConstruirParchesMist() {
    int celdas = mapa.GetCeldas();
    float paso = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;
    const int dx[] = {1, -1, 0, 0};
    const int dz[] = {0, 0, 1, -1};

    for (int inicioX = 0; inicioX < celdas; inicioX += CELDAS_POR_PARCHE) {
        for (int inicioZ = 0; inicioZ < celdas; inicioZ += CELDAS_POR_PARCHE) {
            int finX = std::min(celdas, inicioX + CELDAS_POR_PARCHE);
            int finZ = std::min(celdas, inicioZ + CELDAS_POR_PARCHE);
            GeometriaTemporal geometria;

            for (int x = inicioX; x < finX; x++) {
                for (int z = inicioZ; z < finZ; z++) {
                    int nivel = mapa.GetAltura(x, z);
                    if (nivel == 0) continue;
                    bool borde = false;
                    for (int d = 0; d < 4; d++) {
                        int nx = x + dx[d];
                        int nz = z + dz[d];
                        if (nx < 0 || nx >= celdas || nz < 0 || nz >= celdas) continue;
                        if (mapa.GetAltura(nx, nz) != nivel) {
                            borde = true;
                            break;
                        }
                    }
                    if (!borde) continue;

                    float worldX = (x * paso) - offset + paso * 0.5f;
                    float worldZ = (z * paso) - offset + paso * 0.5f;
                    float altura = mapa.AlturaSuperficie(worldX, worldZ);
                    if (altura < alturaFogMin || altura > alturaFogMax) continue;

                    Color color = {200, 220, 255, 55};
                    float inicioParcheX = worldX - paso * 0.45f;
                    float inicioParcheZ = worldZ - paso * 0.45f;
                    float subPaso = paso * 0.45f;

                    for (int sx = 0; sx < 2; sx++) {
                        for (int sz = 0; sz < 2; sz++) {
                            float x0 = inicioParcheX + sx * subPaso;
                            float x1 = x0 + subPaso;
                            float z0 = inicioParcheZ + sz * subPaso;
                            float z1 = z0 + subPaso;
                            float u0 = sx * 0.5f;
                            float u1 = (sx + 1) * 0.5f;
                            float v0 = sz * 0.5f;
                            float v1 = (sz + 1) * 0.5f;
                            Vector3 a = {x0, mapa.AlturaSuperficie(x0, z0) + 0.18f, z0};
                            Vector3 b = {x1, mapa.AlturaSuperficie(x1, z0) + 0.18f, z0};
                            Vector3 c = {x0, mapa.AlturaSuperficie(x0, z1) + 0.18f, z1};
                            Vector3 d = {x1, mapa.AlturaSuperficie(x1, z1) + 0.18f, z1};
                            AgregarTriangulo(geometria, a, {u0,v0}, c, {u0,v1}, b, {u1,v0}, color, color, color);
                            AgregarTriangulo(geometria, b, {u1,v0}, c, {u0,v1}, d, {u1,v1}, color, color, color);
                        }
                    }
                }
            }

            if (geometria.vertices.empty()) continue;
            float x0 = inicioX * paso - offset;
            float x1 = finX * paso - offset;
            float z0 = inicioZ * paso - offset;
            float z1 = finZ * paso - offset;
            float centroX = (x0 + x1) * 0.5f;
            float centroZ = (z0 + z1) * 0.5f;
            float centroY = mapa.AlturaSuperficie(centroX, centroZ);
            float radio = std::sqrt((x1-x0)*(x1-x0) + (z1-z0)*(z1-z0)) * 0.5f;
            parchesMist.push_back({CrearModelo(geometria, shaderMist), {centroX, centroY, centroZ}, radio});
        }
    }
}

void Ambiente::ReconstruirNaturaleza() {
    DescargarParches(parchesGrass);
    DescargarParches(parchesMist);
    if (!shadersNaturalezaCargados) return;
    ConstruirParchesGrass();
    ConstruirParchesMist();
}

bool Ambiente::ParcheVisible(const ParcheNatural& parche, const Camera3D& cam, float mitadFovxRad) const {
    float dx = parche.centro.x - cam.position.x;
    float dz = parche.centro.z - cam.position.z;
    float distanciaSq = dx*dx + dz*dz;
    float alcance = distanciaNaturaleza + parche.radio;
    if (distanciaSq > alcance * alcance) return false;
    float distancia = std::sqrt(std::max(1.0f, distanciaSq));
    float margen = 0.20f + std::atan2(parche.radio, distancia);
    return EsVisibleEnFrustum(parche.centro, cam, mitadFovxRad, margen, parche.radio + 20.0f);
}

void Ambiente::DibujarSueloBase() const {
    float lado = mapa.GetTamaño() / (float)PARCHES_SUELO;
    float offset = mapa.GetTamaño() / 2.0f;
    float y = -0.05f;

    for (int x = 0; x < PARCHES_SUELO; x++) {
        for (int z = 0; z < PARCHES_SUELO; z++) {
            float x0 = x * lado - offset;
            float z0 = z * lado - offset;
            Vector3 a = {x0, y, z0};
            Vector3 b = {x0 + lado, y, z0};
            Vector3 c = {x0, y, z0 + lado};
            Vector3 d = {x0 + lado, y, z0 + lado};
            Color color = coloresSuelo[x * PARCHES_SUELO + z];
            DrawTriangle3D(a, c, b, color);
            DrawTriangle3D(b, c, d, color);
        }
    }
}

void Ambiente::DibujarCielo(int screenWidth, int screenHeight) const {
    if (!modoNatural) return;
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, colorCieloArriba, colorCieloHorizonte);
}

void Ambiente::ActualizarPrecipitacion(float dt) {
    float radioPrecipitacion = mapa.GetPaso() * 0.42f;

    for (auto& n : nubes) {
        for (auto& gota : n.gotas) {
            float oscilacionX = n.esNieve ? std::sin(tiempoShader * 1.6f + gota.fase) * 0.65f : 0.0f;
            float oscilacionZ = n.esNieve ? std::cos(tiempoShader * 1.25f + gota.fase) * 0.5f : 0.0f;
            gota.posicion.y -= gota.velocidad * dt;
            gota.posicion.x += (gota.driftX + oscilacionX) * dt;
            gota.posicion.z += (gota.driftZ + oscilacionZ) * dt;

            float suelo = mapa.AlturaSuperficie(gota.posicion.x, gota.posicion.z);
            bool fueraDeZona = std::fabs(gota.posicion.x - n.posicion.x) > radioPrecipitacion ||
                               std::fabs(gota.posicion.z - n.posicion.z) > radioPrecipitacion;
            if (gota.posicion.y <= suelo || fueraDeZona) {
                float rx = (((float)rand() / RAND_MAX) - 0.5f) * radioPrecipitacion * 2.0f;
                float rz = (((float)rand() / RAND_MAX) - 0.5f) * radioPrecipitacion * 2.0f;
                gota.posicion = {n.posicion.x + rx, n.posicion.y, n.posicion.z + rz};
            }
        }
    }
}

void Ambiente::ActualizarRelampagos(float dt, const Camera3D& cam) {
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    for (auto& n : nubes) {
        bool enProgreso = n.brilloRelampago > 0.0f;

        if (!enProgreso) {
            n.tiempoHastaProximoRelampago -= dt;
            if (n.tiempoHastaProximoRelampago <= 0.0f) {
                float azar = (float)rand() / (float)RAND_MAX;
                n.tiempoHastaProximoRelampago = RELAMPAGO_INTERVALO_MIN +
                    azar * (RELAMPAGO_INTERVALO_MAX - RELAMPAGO_INTERVALO_MIN);
                if (EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) {
                    n.relampagoSubiendo = true;
                    n.brilloRelampago = 0.001f;
                    n.patronRelampago = (unsigned int)rand();
                }
            }
        } else if (n.relampagoSubiendo) {
            n.brilloRelampago += dt / RELAMPAGO_DURACION_SUBIDA;
            if (n.brilloRelampago >= 1.0f) {
                n.brilloRelampago = 1.0f;
                n.relampagoSubiendo = false;
            }
        } else {
            n.brilloRelampago -= dt / RELAMPAGO_DURACION_BAJADA;
            if (n.brilloRelampago <= 0.0f) n.brilloRelampago = 0.0f;
        }
    }
}

void Ambiente::Actualizar(float dt, const Camera3D& cam) {
    std::uint64_t revisionActual = mapa.GetRevision();
    if (revisionActual != revisionGeometria) {
        RecalcularAgua();
        GenerarNubes();
        ReconstruirNaturaleza();
        revisionGeometria = revisionActual;
    }

    tiempoAgua += dt;
    tiempoShader += dt;

    ActualizarPrecipitacion(dt);
    ActualizarRelampagos(dt, cam);
}

void Ambiente::DibujarParchesGrass(const Camera3D& cam, float mitadFovxRad) const {
    if (!shadersNaturalezaCargados || parchesGrass.empty()) return;
    SetShaderValue(shaderGrass, locTiempoGrass, &tiempoShader, SHADER_UNIFORM_FLOAT);
    for (const auto& parche : parchesGrass) {
        if (ParcheVisible(parche, cam, mitadFovxRad)) DrawModel(parche.modelo, {0,0,0}, 1.0f, WHITE);
    }
}

void Ambiente::DibujarParchesMist(const Camera3D& cam, float mitadFovxRad) const {
    if (!shadersNaturalezaCargados || parchesMist.empty()) return;
    SetShaderValue(shaderMist, locTiempoMist, &tiempoShader, SHADER_UNIFORM_FLOAT);
    rlDisableDepthMask();
    for (const auto& parche : parchesMist) {
        if (ParcheVisible(parche, cam, mitadFovxRad)) DrawModel(parche.modelo, {0,0,0}, 1.0f, WHITE);
    }
    rlEnableDepthMask();
}

void Ambiente::DibujarPrecipitacion(const Camera3D& cam) const {
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    rlBegin(RL_LINES);
    for (const auto& n : nubes) {
        if (n.esNieve) continue;
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;
        for (const auto& gota : n.gotas) {
            float largo = 1.0f + gota.escala * 1.1f;
            Vector3 arriba = {
                gota.posicion.x - gota.driftX * 0.04f,
                gota.posicion.y + largo,
                gota.posicion.z - gota.driftZ * 0.04f
            };
            rlColor4ub(130, 180, 255, 170);
            rlVertex3f(arriba.x, arriba.y, arriba.z);
            rlColor4ub(130, 180, 255, 90);
            rlVertex3f(gota.posicion.x, gota.posicion.y, gota.posicion.z);
        }
    }
    rlEnd();

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    rlBegin(RL_TRIANGLES);
    for (const auto& n : nubes) {
        if (!n.esNieve) continue;
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;
        for (const auto& gota : n.gotas) {
            float tamaño = 0.12f + gota.escala * 0.10f;
            Vector3 r = {right.x*tamaño, right.y*tamaño, right.z*tamaño};
            Vector3 u = {up.x*tamaño, up.y*tamaño, up.z*tamaño};
            Vector3 a = {gota.posicion.x-r.x-u.x, gota.posicion.y-r.y-u.y, gota.posicion.z-r.z-u.z};
            Vector3 b = {gota.posicion.x+r.x-u.x, gota.posicion.y+r.y-u.y, gota.posicion.z+r.z-u.z};
            Vector3 c = {gota.posicion.x+r.x+u.x, gota.posicion.y+r.y+u.y, gota.posicion.z+r.z+u.z};
            Vector3 d = {gota.posicion.x-r.x+u.x, gota.posicion.y-r.y+u.y, gota.posicion.z-r.z+u.z};
            rlColor4ub(220, 235, 255, 200);
            rlVertex3f(a.x, a.y, a.z);
            rlVertex3f(b.x, b.y, b.z);
            rlVertex3f(c.x, c.y, c.z);
            rlVertex3f(a.x, a.y, a.z);
            rlVertex3f(c.x, c.y, c.z);
            rlVertex3f(d.x, d.y, d.z);
        }
    }
    rlEnd();
}

Vector3 Ambiente::CeldaAMundo(int x, int z) const {
    float paso = mapa.GetPaso();
    float offset = mapa.GetTamaño() / 2.0f;
    return {(x * paso) - offset + paso * 0.5f, 0.0f, (z * paso) - offset + paso * 0.5f};
}

void Ambiente::DibujarNieve(int x, int z, float worldX, float worldZ) const {
    float paso = mapa.GetPaso();
    for (int i = 0; i < 12; i++) {
        float ox = (cellRand(x, z, i*7+0) - 0.5f) * paso * 0.9f;
        float oz = (cellRand(x, z, i*7+1) - 0.5f) * paso * 0.9f;
        float tamaño = 0.15f + cellRand(x, z, i*7+2) * 0.25f;
        float px = worldX + ox;
        float pz = worldZ + oz;
        float baseY = mapa.AlturaSuperficie(px, pz);
        DrawCube({px, baseY + tamaño * 0.25f, pz}, tamaño, tamaño * 0.5f, tamaño, WHITE);
    }
}

void Ambiente::DibujarAgua(float worldX, float worldZ, float shimmer) const {
    float paso = mapa.GetPaso();
    float y = 0.3f + sinf(shimmer) * 0.15f;
    float h = paso * 0.48f;
    Vector3 tl = {worldX - h, y, worldZ - h};
    Vector3 tr = {worldX + h, y, worldZ - h};
    Vector3 bl = {worldX - h, y, worldZ + h};
    Vector3 br = {worldX + h, y, worldZ + h};
    unsigned char blue = (unsigned char)(180 + sinf(shimmer * 1.3f) * 40);
    Color agua = {0, (unsigned char)(200 + sinf(shimmer*0.7f)*30), blue, 180};
    DrawTriangle3D(tl, bl, tr, agua);
    DrawTriangle3D(tr, bl, br, agua);
    Color borde = {0, 255, 255, 120};
    DrawLine3D(tl, tr, borde);
    DrawLine3D(tr, br, borde);
    DrawLine3D(br, bl, borde);
    DrawLine3D(bl, tl, borde);
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
    if (!shaderNubeCargado) {
        DibujarNubes();
        return;
    }

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up = Vector3CrossProduct(right, forward);
    SetShaderValue(shaderNube, locCamRight, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderNube, locCamUp, &up, SHADER_UNIFORM_VEC3);
    SetShaderValue(shaderNube, locTiempo, &tiempoShader, SHADER_UNIFORM_FLOAT);

    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    for (const auto& n : nubes) {
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;
        Vector3 colorBase = {0.34f, 0.37f, 0.44f};
        Vector3 colorFlash = {1.45f, 1.52f, 1.72f};
        Vector3 colorFinal = Vector3Lerp(colorBase, colorFlash, n.brilloRelampago);
        SetShaderValue(shaderNube, locColorNube, &colorFinal, SHADER_UNIFORM_VEC3);
        int nPuffs = std::min(PUFFS_POR_NUBE, n.nParticulas);

        for (int i = 0; i < nPuffs; i++) {
            Vector3 posPuff = Vector3Add(n.posicion, n.offsets[i]);
            float uvOff[2] = {posPuff.x * 0.002f, posPuff.z * 0.002f};
            SetShaderValue(shaderNube, locUVOffset, uvOff, SHADER_UNIFORM_VEC2);
            float fase = (float)i * 1.7f + n.posicion.x * 0.01f;
            SetShaderValue(shaderNube, locFasePuff, &fase, SHADER_UNIFORM_FLOAT);
            float brilloBase = i == 0 ? 1.0f : 0.75f;
            float brillo = brilloBase + n.brilloRelampago * (1.0f - brilloBase) * 0.8f;
            SetShaderValue(shaderNube, locBrilloPuff, &brillo, SHADER_UNIFORM_FLOAT);
            float escala = n.radio * (1.6f + i * 0.3f);
            DrawModel(modeloNube, posPuff, escala, WHITE);
        }
    }
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void Ambiente::DibujarRelampagos(const Camera3D& cam) const {
    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));

    rlBegin(RL_LINES);
    for (const auto& n : nubes) {
        if (n.brilloRelampago < 0.12f) continue;
        if (!EsVisibleEnFrustum(n.posicion, cam, mitadFovxRad, 0.25f, radioCullAmbiente)) continue;

        float suelo = mapa.AlturaSuperficie(n.posicion.x, n.posicion.z) + 0.3f;
        Vector3 anterior = {n.posicion.x, n.posicion.y - n.radio * 0.15f, n.posicion.z};
        int clave = (int)(n.patronRelampago & 0x7fffffffu);

        for (int i = 1; i <= 8; i++) {
            float t = i / 8.0f;
            float amplitud = n.radio * 0.38f * (1.0f - t);
            float desvioX = (cellRand(clave, i, 31) - 0.5f) * amplitud * 2.0f;
            float desvioZ = (cellRand(clave, i, 32) - 0.5f) * amplitud * 2.0f;
            Vector3 siguiente = {
                n.posicion.x + desvioX,
                n.posicion.y + (suelo - n.posicion.y) * t,
                n.posicion.z + desvioZ
            };

            unsigned char alphaExterior = (unsigned char)(80.0f + n.brilloRelampago * 120.0f);
            unsigned char alphaCentro = (unsigned char)(150.0f + n.brilloRelampago * 105.0f);
            rlColor4ub(105, 145, 255, alphaExterior);
            rlVertex3f(anterior.x - 0.08f, anterior.y, anterior.z);
            rlVertex3f(siguiente.x - 0.08f, siguiente.y, siguiente.z);
            rlColor4ub(230, 240, 255, alphaCentro);
            rlVertex3f(anterior.x, anterior.y, anterior.z);
            rlVertex3f(siguiente.x, siguiente.y, siguiente.z);
            rlColor4ub(105, 145, 255, alphaExterior);
            rlVertex3f(anterior.x + 0.08f, anterior.y, anterior.z);
            rlVertex3f(siguiente.x + 0.08f, siguiente.y, siguiente.z);
            anterior = siguiente;
        }
    }
    rlEnd();
}

void Ambiente::CargarShader() {
    shaderNube = LoadShader("shaders/cloud.vs", "shaders/cloud.fs");
    locTiempo = GetShaderLocation(shaderNube, "tiempo");
    locUVOffset = GetShaderLocation(shaderNube, "uvOffset");
    locColorNube = GetShaderLocation(shaderNube, "colorNube");
    locCamRight = GetShaderLocation(shaderNube, "camRight");
    locCamUp = GetShaderLocation(shaderNube, "camUp");
    locFasePuff = GetShaderLocation(shaderNube, "fasePuff");
    locBrilloPuff = GetShaderLocation(shaderNube, "brilloPuff");
    Mesh quadNube = GenMeshPlane(1.0f, 1.0f, 1, 1);
    modeloNube = LoadModelFromMesh(quadNube);
    modeloNube.materials[0].shader = shaderNube;
    shaderNubeCargado = true;

    shaderGrass = LoadShader("shaders/grass.vs", "shaders/grass.fs");
    shaderMist = LoadShader("shaders/mist.vs", "shaders/mist.fs");
    locTiempoGrass = GetShaderLocation(shaderGrass, "tiempo");
    locTiempoMist = GetShaderLocation(shaderMist, "tiempo");
    shadersNaturalezaCargados = true;
    ReconstruirNaturaleza();
    revisionGeometria = mapa.GetRevision();
}

void Ambiente::DescargarShader() {
    DescargarParches(parchesGrass);
    DescargarParches(parchesMist);

    if (shadersNaturalezaCargados) {
        UnloadShader(shaderGrass);
        UnloadShader(shaderMist);
        shadersNaturalezaCargados = false;
    }

    if (shaderNubeCargado) {
        UnloadModel(modeloNube);
        UnloadShader(shaderNube);
        shaderNubeCargado = false;
    }

    revisionGeometria = ~std::uint64_t{0};
}

void Ambiente::Dibujar(const Camera3D& cam) const {
    if (!modoNatural) return;
    DibujarSueloBase();

    float mitadFovyRad = (cam.fovy * 0.5f) * DEG2RAD;
    float mitadFovxRad = atanf(tanf(mitadFovyRad) * (16.0f / 9.0f));
    DibujarParchesGrass(cam, mitadFovxRad);

    int celdas = mapa.GetCeldas();
    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            if (mapa.GetAltura(x, z) != 5) continue;
            Vector3 wp = CeldaAMundo(x, z);
            wp.y = mapa.AlturaSuperficie(wp.x, wp.z);
            if (!EsVisibleEnFrustum(wp, cam, mitadFovxRad, 0.20f, radioCullAmbiente)) continue;
            DibujarNieve(x, z, wp.x, wp.z);
        }
    }

    for (const auto& [x, z] : celdasAgua) {
        Vector3 wp = CeldaAMundo(x, z);
        if (!EsVisibleEnFrustum(wp, cam, mitadFovxRad, 0.20f, radioCullAmbiente)) continue;
        DibujarAgua(wp.x, wp.z, tiempoAgua + cellRand(x, z, 0) * 6.28f);
    }

    DibujarParchesMist(cam, mitadFovxRad);
    DibujarNubesShader(cam);
    DibujarRelampagos(cam);
    DibujarPrecipitacion(cam);
}
