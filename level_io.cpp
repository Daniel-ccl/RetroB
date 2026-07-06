#include "level_io.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

static std::unordered_map<std::string, std::string> ParseTokens(const std::string& linea) {
    std::unordered_map<std::string, std::string> tokens;
    std::istringstream ss(linea);
    std::string token;
    while (ss >> token) {
        auto eq = token.find('=');
        if (eq == std::string::npos) continue;
        std::string key = token.substr(0, eq);
        std::string val = token.substr(eq + 1);

        if (key == "desc") {
            if (!val.empty() && val.front() == '"') {
                val = val.substr(1);
                std::string resto;
                std::getline(ss, resto);
                auto cierre = resto.rfind('"');
                if (cierre != std::string::npos) resto = resto.substr(0, cierre);
                val += resto;
            }
        }
        tokens[key] = val;
    }
    return tokens;
}

static std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

static float GetF(const std::unordered_map<std::string,std::string>& t, const std::string& k, float def = 0.0f) {
    auto it = t.find(k);
    return it != t.end() ? std::stof(it->second) : def;
}

static int GetI(const std::unordered_map<std::string,std::string>& t, const std::string& k, int def = 0) {
    auto it = t.find(k);
    return it != t.end() ? std::stoi(it->second) : def;
}

static std::string GetS(const std::unordered_map<std::string,std::string>& t, const std::string& k, const std::string& def = "") {
    auto it = t.find(k);
    return it != t.end() ? it->second : def;
}

namespace LevelIO {

bool Cargar(const std::string& ruta, LevelData& out) {
    std::ifstream f(ruta);
    if (!f.is_open()) return false;

    out = LevelData{};
    std::string linea;
    bool enTerrain = false;
    int  filaTerreno = 0;

    while (std::getline(f, linea)) {
        if (linea.empty() || linea[0] == '#') continue;

        std::string cmd;
        std::istringstream ls(linea);
        ls >> cmd;

        if (cmd == "TERRAIN_START") { enTerrain = true; filaTerreno = 0; continue; }
        if (cmd == "TERRAIN_END")   { enTerrain = false; continue; }

        if (enTerrain) {
            std::vector<int> fila;
            int v;
            std::istringstream rs(linea);
            while (rs >> v) fila.push_back(v);
            if ((int)out.alturas.size() <= filaTerreno) out.alturas.push_back(fila);
            filaTerreno++;
            continue;
        }

        auto t = ParseTokens(linea);

        if (cmd == "LEVEL") {
            out.nombre = GetS(t, "nombre");
            out.tamano = GetI(t, "tamano", 400);
            out.celdas = GetI(t, "celdas", 40);
        }
        else if (cmd == "PLAYER_SPAWN") {
            out.jugadorSpawn = { GetF(t,"x"), GetF(t,"y"), GetF(t,"z") };
            out.jugadorRotacion = GetF(t, "rot");
        }
        else if (cmd == "ENEMY") {
            EnemySpawn e;
            e.id       = GetS(t, "id");
            e.tipo     = GetS(t, "tipo", "sam");
            e.posicion = { GetF(t,"x"), GetF(t,"y"), GetF(t,"z") };
            e.rotacion = GetF(t, "rot");
            if (t.count("hp"))            e.config.hp            = GetF(t,"hp");
            if (t.count("missileCount"))  e.config.missileCount  = GetI(t,"missileCount");
            if (t.count("missileDamage")) e.config.missileDamage = GetF(t,"missileDamage");
            if (t.count("missileSpeed"))  e.config.missileSpeed  = GetF(t,"missileSpeed");
            if (t.count("coneRange"))     e.config.coneRange     = GetF(t,"coneRange");
            if (t.count("coneAngle"))     e.config.coneAngle     = GetF(t,"coneAngle");
            out.enemigos.push_back(e);
        }
        else if (cmd == "OBJECTIVE") {
            Objective o;
            o.tipo        = GetS(t, "tipo");
            o.descripcion = GetS(t, "desc");
            o.location    = { GetF(t,"x"), GetF(t,"y"), GetF(t,"z") };
            o.radius      = GetF(t, "radius");
            o.duration    = GetF(t, "duration");
            std::string tgts = GetS(t, "targets");
            if (!tgts.empty()) o.targetIds = Split(tgts, ',');
            out.objetivos.push_back(o);
        }
    }
    return true;
}

bool Guardar(const std::string& ruta, const LevelData& lvl) {
    std::ofstream f(ruta);
    if (!f.is_open()) return false;

    f << "LEVEL nombre=" << lvl.nombre
      << " tamano=" << lvl.tamano
      << " celdas=" << lvl.celdas << "\n";

    f << "PLAYER_SPAWN x=" << lvl.jugadorSpawn.x
      << " y=" << lvl.jugadorSpawn.y
      << " z=" << lvl.jugadorSpawn.z
      << " rot=" << lvl.jugadorRotacion << "\n";

    f << "\nTERRAIN_START\n";
    for (const auto& fila : lvl.alturas) {
        for (int i = 0; i < (int)fila.size(); i++) {
            if (i > 0) f << ' ';
            f << fila[i];
        }
        f << '\n';
    }
    f << "TERRAIN_END\n";

    for (const auto& e : lvl.enemigos) {
        f << "\nENEMY id=" << e.id << " tipo=" << e.tipo
          << " x=" << e.posicion.x << " y=" << e.posicion.y << " z=" << e.posicion.z
          << " rot=" << e.rotacion;
        if (e.config.hp            >= 0) f << " hp="            << e.config.hp;
        if (e.config.missileCount  >= 0) f << " missileCount="  << e.config.missileCount;
        if (e.config.missileDamage >= 0) f << " missileDamage=" << e.config.missileDamage;
        if (e.config.missileSpeed  >= 0) f << " missileSpeed="  << e.config.missileSpeed;
        if (e.config.coneRange     >= 0) f << " coneRange="     << e.config.coneRange;
        if (e.config.coneAngle     >= 0) f << " coneAngle="     << e.config.coneAngle;
        f << '\n';
    }

    for (const auto& o : lvl.objetivos) {
        f << "\nOBJECTIVE tipo=" << o.tipo;
        if (!o.targetIds.empty()) {
            f << " targets=";
            for (int i = 0; i < (int)o.targetIds.size(); i++) {
                if (i > 0) f << ',';
                f << o.targetIds[i];
            }
        }
        if (o.tipo == "alcanzar")
            f << " x=" << o.location.x << " y=" << o.location.y << " z=" << o.location.z
              << " radius=" << o.radius;
        if (o.tipo == "sobrevivir")
            f << " duration=" << o.duration;
        f << " desc=\"" << o.descripcion << "\"\n";
    }

    return true;
}

} // namespace LevelIO
