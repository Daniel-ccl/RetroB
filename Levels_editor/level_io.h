#ifndef LEVEL_IO_H
#define LEVEL_IO_H

#include "level_data.h"
#include <string>

namespace LevelIO {
    bool Cargar(const std::string& ruta, LevelData& out);

    bool Guardar(const std::string& ruta, const LevelData& lvl);
}

#endif
