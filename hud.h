#ifndef HUD_H
#define HUD_H

#include "raylib.h"

namespace Hud {
    void DibujarPanelPrincipal(int screenWidth, int screenHeight, float pctSalud, float fraccionBoost,
                                bool misilListo, bool municionInfinita, int numMisilesActivos);
}

#endif
