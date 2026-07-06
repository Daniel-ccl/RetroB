#include "salud.h"
#include <algorithm>

Salud::Salud(float hpMax) {
    this->hpMax = hpMax;
    this->hpActual = hpMax;
}

void Salud::RecibirDano(float cantidad) {
    if (cantidad <= 0.0f) return; 
    hpActual -= cantidad;
    if (hpActual < 0.0f) hpActual = 0.0f;
}

void Salud::Curar(float cantidad) {
    if (cantidad <= 0.0f) return;
    hpActual = std::min(hpMax, hpActual + cantidad);
}

void Salud::Reiniciar() {
    hpActual = hpMax;
}
