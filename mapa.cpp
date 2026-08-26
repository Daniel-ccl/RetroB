#include "mapa.h"
#include "rlgl.h"
#include <cmath>
#include <cstring>
#include <algorithm>

static const Color COLOR_NEON = { 0, 255, 255, 200 };

static Color ColorPorAltura(float h) {
    Color c = BLUE;
    if (h > 15.0f) c = GREEN;
    if (h > 25.0f) c = YELLOW;
    if (h > 35.0f) c = ORANGE;
    if (h > 45.0f) c = RED;
    return c;
}

static Color ColorNaturalPorAltura(float h) {
    Color c = (Color){101, 74, 46, 255};
    if (h > 25.0f) c = (Color){130, 130, 135, 255};
    if (h > 45.0f) c = (Color){235, 240, 245, 255};
    return c;
}

static void AgregarCajaSolida(std::vector<float>& verts, std::vector<unsigned char>& cols,
                               std::vector<unsigned short>& idx, Vector3 pos,
                               float w, float h, float d, Color color) {
    unsigned short base = (unsigned short)(verts.size() / 3);
    float hx = w / 2.0f, hy = h / 2.0f, hz = d / 2.0f;
    Vector3 e[8] = {
        {pos.x-hx,pos.y-hy,pos.z-hz}, {pos.x+hx,pos.y-hy,pos.z-hz},
        {pos.x+hx,pos.y+hy,pos.z-hz}, {pos.x-hx,pos.y+hy,pos.z-hz},
        {pos.x-hx,pos.y-hy,pos.z+hz}, {pos.x+hx,pos.y-hy,pos.z+hz},
        {pos.x+hx,pos.y+hy,pos.z+hz}, {pos.x-hx,pos.y+hy,pos.z+hz}
    };
    for (int i = 0; i < 8; i++) {
        verts.push_back(e[i].x); verts.push_back(e[i].y); verts.push_back(e[i].z);
        cols.push_back(color.r); cols.push_back(color.g); cols.push_back(color.b); cols.push_back(color.a);
    }
    static const unsigned short caras[36] = {
        0,1,2, 0,2,3,   1,5,6, 1,6,2,   5,4,7, 5,7,6,
        4,0,3, 4,3,7,   3,2,6, 3,6,7,   4,5,1, 4,1,0
    };
    for (int i = 0; i < 36; i++) idx.push_back(base + caras[i]);
}

static void AgregarCajaWire(std::vector<Vector3>& out, Vector3 pos, float w, float h, float d) {
    float hx = w / 2.0f, hy = h / 2.0f, hz = d / 2.0f;
    Vector3 e[8] = {
        {pos.x-hx,pos.y-hy,pos.z-hz}, {pos.x+hx,pos.y-hy,pos.z-hz},
        {pos.x+hx,pos.y+hy,pos.z-hz}, {pos.x-hx,pos.y+hy,pos.z-hz},
        {pos.x-hx,pos.y-hy,pos.z+hz}, {pos.x+hx,pos.y-hy,pos.z+hz},
        {pos.x+hx,pos.y+hy,pos.z+hz}, {pos.x-hx,pos.y+hy,pos.z+hz}
    };
    static const int aristas[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; i++) { out.push_back(e[aristas[i][0]]); out.push_back(e[aristas[i][1]]); }
}

static void AgregarCuadradoWire(std::vector<Vector3>& out, Vector3 centro, float paso) {
    float h = paso / 2.0f;
    Vector3 a = {centro.x-h,0.0f,centro.z-h}, b = {centro.x+h,0.0f,centro.z-h};
    Vector3 c = {centro.x+h,0.0f,centro.z+h}, d = {centro.x-h,0.0f,centro.z+h};
    out.push_back(a); out.push_back(b);
    out.push_back(b); out.push_back(c);
    out.push_back(c); out.push_back(d);
    out.push_back(d); out.push_back(a);
}

Mapa::Mapa(int tamaño, int celdas) {
    this->tamaño = tamaño;
    this->celdas = celdas;
    this->paso = (float)tamaño / (float)celdas;
    alturas.resize(celdas, std::vector<int>(celdas, 0));
}

Mapa::Mapa(const Mapa& otro)
    : tamaño(otro.tamaño), celdas(otro.celdas), paso(otro.paso), alturas(otro.alturas) {
    sucio = true;
}

Mapa& Mapa::operator=(const Mapa& otro) {
    if (this == &otro) return *this;
    DescargarModelos();
    tamaño  = otro.tamaño;
    celdas  = otro.celdas;
    paso    = otro.paso;
    alturas = otro.alturas;
    alturaFina.clear();
    segmentosWireRetro.clear();
    sucio = true;
    return *this;
}

Mapa::~Mapa() {
    DescargarModelos();
}

void Mapa::SetAltura(int x, int z, int v) {
    alturas[x][z] = v;
    sucio = true;
}

bool Mapa::PosicionAIndice(Vector3 pos, int& gridX, int& gridZ) {
    float offsetX = pos.x + (tamaño / 2.0f);
    float offsetZ = pos.z + (tamaño / 2.0f);
    gridX = (int)(offsetX / paso);
    gridZ = (int)(offsetZ / paso);
    return (gridX >= 0 && gridX < celdas && gridZ >= 0 && gridZ < celdas);
}

float Mapa::AlturaBilinealCoarse(float fx, float fz) const {
    auto getMapH = [&](int mx, int mz) -> float {
        if (mx < 0 || mx >= celdas || mz < 0 || mz >= celdas) return 0.0f;
        return (float)alturas[mx][mz] * 10.0f;
    };
    float cx = fx - 0.5f;
    float cz = fz - 0.5f;
    int x0 = (int)std::floor(cx), z0 = (int)std::floor(cz);
    int x1 = x0 + 1, z1 = z0 + 1;
    float tx = cx - x0, tz = cz - z0;

    float h00 = getMapH(x0,z0), h10 = getMapH(x1,z0);
    float h01 = getMapH(x0,z1), h11 = getMapH(x1,z1);
    float h0 = h00 * (1.0f-tx) + h10 * tx;
    float h1 = h01 * (1.0f-tx) + h11 * tx;
    return h0 * (1.0f-tz) + h1 * tz;
}

float Mapa::AlturaSubCubo(int x, int z, int sx, int sz) const {
    float fx = x + (sx + 0.5f) / 4.0f;
    float fz = z + (sz + 0.5f) / 4.0f;
    float baseH = AlturaBilinealCoarse(fx, fz);
    if (baseH <= 0.5f) return 0.0f;
    float ruido = ((x*13 + z*27 + sx*5 + sz*7) % 15) / 5.0f;
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

float Mapa::AlturaFinaEn(int x, int sx, int z, int sz) const {
    int finoLado = celdas * 4;
    return alturaFina[(size_t)(x*4+sx) * finoLado + (z*4+sz)];
}

void Mapa::RecalcularAlturaFina() {
    int finoLado = celdas * 4;
    alturaFina.assign((size_t)finoLado * finoLado, 0.0f);
    for (int x = 0; x < celdas; x++)
        for (int z = 0; z < celdas; z++)
            for (int sx = 0; sx < 4; sx++)
                for (int sz = 0; sz < 4; sz++)
                    alturaFina[(size_t)(x*4+sx)*finoLado + (z*4+sz)] = AlturaSubCubo(x, z, sx, sz);
}

void Mapa::DescargarModelos() {
    if (modelosCargados) {
        UnloadModel(modeloSolidoRetro);
        UnloadModel(modeloNatural);
        modelosCargados = false;
    }
}

void Mapa::ConstruirModeloRetro() {
    segmentosWireRetro.clear();

    std::vector<float> verts;
    std::vector<unsigned char> cols;
    std::vector<unsigned short> idx;

    float offset = tamaño / 2.0f;

    for (int x = 0; x < celdas; x++) {
        for (int z = 0; z < celdas; z++) {
            Vector3 centroCelda = { (x*paso)-offset+(paso/2.0f), 0.0f, (z*paso)-offset+(paso/2.0f) };
            AgregarCuadradoWire(segmentosWireRetro, centroCelda, paso);

            bool hasMountain = false;
            for (int dx = -1; dx <= 1; dx++)
                for (int dz = -1; dz <= 1; dz++) {
                    int nx = x+dx, nz = z+dz;
                    if (nx >= 0 && nx < celdas && nz >= 0 && nz < celdas && alturas[nx][nz] > 0)
                        hasMountain = true;
                }
            if (!hasMountain) continue;

            float subPaso = paso / 4.0f;
            for (int sx = 0; sx < 4; sx++) {
                for (int sz = 0; sz < 4; sz++) {
                    float finalH = AlturaFinaEn(x, sx, z, sz);
                    if (finalH <= 0.0f) continue;
                    if (verts.size()/3 + 8 > 65500) continue;

                    float ruido = ((x*13+z*27+sx*5+sz*7) % 15) / 5.0f;
                    Color color = ColorPorAltura(finalH);
                    if (ruido > 1.5f) {
                        color.r = (unsigned char)std::max(0, color.r-30);
                        color.g = (unsigned char)std::max(0, color.g-30);
                        color.b = (unsigned char)std::max(0, color.b-30);
                    }

                    Vector3 subPos = {
                        (x*paso)-offset+(sx*subPaso)+(subPaso/2.0f),
                        finalH/2.0f,
                        (z*paso)-offset+(sz*subPaso)+(subPaso/2.0f)
                    };

                    AgregarCajaSolida(verts, cols, idx, subPos, subPaso, finalH, subPaso, color);
                    AgregarCajaWire(segmentosWireRetro, subPos, subPaso, finalH, subPaso);
                }
            }
        }
    }

    Mesh mesh = {0};
    mesh.vertexCount = (int)(verts.size()/3);
    mesh.triangleCount = (int)(idx.size()/3);
    mesh.vertices = (float*)MemAlloc(verts.size()*sizeof(float));
    memcpy(mesh.vertices, verts.data(), verts.size()*sizeof(float));
    mesh.normals = (float*)MemAlloc(verts.size()*sizeof(float));
    for (size_t i = 0; i < verts.size()/3; i++) { mesh.normals[i*3]=0.0f; mesh.normals[i*3+1]=1.0f; mesh.normals[i*3+2]=0.0f; }
    mesh.colors = (unsigned char*)MemAlloc(cols.size()*sizeof(unsigned char));
    memcpy(mesh.colors, cols.data(), cols.size()*sizeof(unsigned char));
    mesh.indices = (unsigned short*)MemAlloc(idx.size()*sizeof(unsigned short));
    memcpy(mesh.indices, idx.data(), idx.size()*sizeof(unsigned short));

    UploadMesh(&mesh, false);
    modeloSolidoRetro = LoadModelFromMesh(mesh);
}

void Mapa::ConstruirModeloNatural() {
    int fino = celdas * 4;
    int lado = fino + 1;
    float offset = tamaño / 2.0f;

    std::vector<float> verts((size_t)lado*lado*3);
    std::vector<float> norms((size_t)lado*lado*3, 0.0f);
    std::vector<unsigned char> cols((size_t)lado*lado*4);

    for (int i = 0; i < lado; i++) {
        for (int j = 0; j < lado; j++) {
            float h = AlturaBilinealCoarse((float)i/4.0f, (float)j/4.0f);
            if (h <= 0.5f) h = 0.0f;

            size_t v = (size_t)(i*lado+j);
            verts[v*3+0] = ((float)i/4.0f)*paso - offset;
            verts[v*3+1] = h;
            verts[v*3+2] = ((float)j/4.0f)*paso - offset;
            norms[v*3+1] = 1.0f;

            Color col = ColorNaturalPorAltura(h);
            cols[v*4+0]=col.r; cols[v*4+1]=col.g; cols[v*4+2]=col.b; cols[v*4+3]=255;
        }
    }

    std::vector<unsigned short> idx;
    idx.reserve((size_t)fino*fino*6);
    for (int i = 0; i < fino; i++) {
        for (int j = 0; j < fino; j++) {
            unsigned short a = (unsigned short)(i*lado+j);
            unsigned short b = (unsigned short)((i+1)*lado+j);
            unsigned short c = (unsigned short)(i*lado+j+1);
            unsigned short d = (unsigned short)((i+1)*lado+j+1);
            idx.push_back(a); idx.push_back(b); idx.push_back(c);
            idx.push_back(b); idx.push_back(d); idx.push_back(c);
        }
    }

    Mesh mesh = {0};
    mesh.vertexCount = lado*lado;
    mesh.triangleCount = (int)(idx.size()/3);
    mesh.vertices = (float*)MemAlloc(verts.size()*sizeof(float));
    memcpy(mesh.vertices, verts.data(), verts.size()*sizeof(float));
    mesh.normals = (float*)MemAlloc(norms.size()*sizeof(float));
    memcpy(mesh.normals, norms.data(), norms.size()*sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc(cols.size()*sizeof(unsigned char));
    memcpy(mesh.colors, cols.data(), cols.size()*sizeof(unsigned char));
    mesh.indices = (unsigned short*)MemAlloc(idx.size()*sizeof(unsigned short));
    memcpy(mesh.indices, idx.data(), idx.size()*sizeof(unsigned short));

    UploadMesh(&mesh, false);
    modeloNatural = LoadModelFromMesh(mesh);
}

void Mapa::ConstruirModelos() {
    DescargarModelos();
    ConstruirModeloRetro();
    ConstruirModeloNatural();
    modelosCargados = true;
}

void Mapa::Dibujar3D(bool modoNatural) {
    if (sucio) {
        RecalcularAlturaFina();
        ConstruirModelos();
        sucio = false;
    }

    if (modoNatural) {
        DrawModel(modeloNatural, {0,0,0}, 1.0f, WHITE);
        return;
    }

    DrawModel(modeloSolidoRetro, {0,0,0}, 1.0f, WHITE);

    rlBegin(RL_LINES);
    for (size_t i = 0; i + 1 < segmentosWireRetro.size(); i += 2) {
        rlColor4ub(COLOR_NEON.r, COLOR_NEON.g, COLOR_NEON.b, COLOR_NEON.a);
        rlVertex3f(segmentosWireRetro[i].x,   segmentosWireRetro[i].y,   segmentosWireRetro[i].z);
        rlVertex3f(segmentosWireRetro[i+1].x, segmentosWireRetro[i+1].y, segmentosWireRetro[i+1].z);
    }
    rlEnd();
}
