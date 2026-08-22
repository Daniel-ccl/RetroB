#include "trayectoria.h"
#include "raymath.h"
#include <cmath>

float RuidoHash(unsigned int seed) {
    unsigned int h = seed;
    h = (h ^ (h >> 16)) * 0x45d9f3b;
    h = (h ^ (h >> 16)) * 0x45d9f3b;
    h ^= (h >> 16);
    return (float)(h & 0xFFFF) / 65535.0f;
}

Vector3 TrayectoriaAleatoria(Vector3 origen, Vector3 destino, float t, float variacion, unsigned int seed) {
    t = Clamp(t, 0.0f, 1.0f);
    Vector3 base = Vector3Lerp(origen, destino, t);

    Vector3 dir = Vector3Subtract(destino, origen);
    float len = Vector3Length(dir);

    Vector3 lateral1, lateral2;
    if (len > 0.001f) {
        dir = Vector3Scale(dir, 1.0f / len);
        Vector3 arriba = (fabsf(dir.y) > 0.95f) ? (Vector3){1.0f,0.0f,0.0f} : (Vector3){0.0f,1.0f,0.0f};
        lateral1 = Vector3Normalize(Vector3CrossProduct(dir, arriba));
        lateral2 = Vector3CrossProduct(dir, lateral1);
    } else {
        lateral1 = {1.0f, 0.0f, 0.0f};
        lateral2 = {0.0f, 0.0f, 1.0f};
    }

    float r1 = RuidoHash(seed * 7919u  + (unsigned int)(t * 1000.0f))       - 0.5f;
    float r2 = RuidoHash(seed * 104729u + (unsigned int)(t * 1000.0f) + 17u) - 0.5f;

    float decaimiento = 1.0f - t; // se estabiliza sobre el destino conforme t -> 1
    Vector3 perturbacion = Vector3Add(
        Vector3Scale(lateral1, r1 * variacion * decaimiento),
        Vector3Scale(lateral2, r2 * variacion * decaimiento)
    );

    return Vector3Add(base, perturbacion);
}
