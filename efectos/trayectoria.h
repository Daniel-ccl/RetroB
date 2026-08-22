#ifndef TRAYECTORIA_H
#define TRAYECTORIA_H

#include "raylib.h"

// ruido determinista 0..1 a partir de una semilla entera (mismo patron que Ambiente::cellRand)
float RuidoHash(unsigned int seed);

// punto en el trayecto de origen a destino en el tiempo t (0..1),
// perturbado lateralmente por ruido que decae al acercarse al destino
Vector3 TrayectoriaAleatoria(Vector3 origen, Vector3 destino, float t, float variacion, unsigned int seed);

#endif
