
#include "portal.h"
#include <cmath>
#include <cstdlib>

Portal::Portal(Vector3 pos, float r, Color color) {
    posicion = pos;
    radio = r;
    colorAnillo = color;
    cooldown = 0.0f;
    GenerarParticulas(50);
}

void Portal::GenerarParticulas(int cantidad) {
    particulas.clear();
    particulas.reserve(cantidad);

    for (int i = 0; i < cantidad; i++) {
        float theta = (rand() % 1000 / 1000.0f) * 2 * M_PI;
        float phi   = acosf(2.0f * (rand() % 1000 / 1000.0f) - 1.0f);
        float r     = (rand() % 1000 / 1000.0f) * radio;

        Particula p;
        p.posicion = {
            posicion.x + r * sinf(phi) * cosf(theta),
            posicion.y + r * sinf(phi) * sinf(theta),
            posicion.z + r * cosf(phi)
        };

        Vector3 dir = {
            p.posicion.x - posicion.x,
            p.posicion.y - posicion.y,
            p.posicion.z - posicion.z
        };
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (len > 0.001f) { dir.x /= len; dir.y /= len; dir.z /= len; }

        float speed = 0.5f * (0.5f + rand() % 100 / 100.0f);
        p.velocidad = { dir.x * speed, dir.y * speed, dir.z * speed };

        p.vida = 2.0f + (rand() % 100 / 100.0f);
        p.colorFuego = { (unsigned char)(200 + rand() % 55), (unsigned char)(50 + rand() % 50), 0, 255 };
        p.tamano = 0.05f + (rand() % 10 / 100.0f);
        p.flickerTimer = (rand() % 100) / 100.0f * 0.1f; 
        particulas.push_back(p);
    }
}

bool Portal::DetectarEntrada(Vector3 posAvion) {
    if (cooldown > 0.0f) return false;

    float dx = posAvion.x - posicion.x;
    float dy = posAvion.y - posicion.y;
    float dz = posAvion.z - posicion.z;
    float dist2 = dx*dx + dy*dy + dz*dz;

    if (dist2 <= (radio * radio)) {
        cooldown = 10.0f; 
        return true;
    }
    return false;
}

void Portal::Actualizar(float dt) {
    if (cooldown > 0.0f) cooldown -= dt;

    for (auto &p : particulas) {
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;
        p.vida -= dt;

        p.flickerTimer -= dt;
        if (p.flickerTimer <= 0.0f) {
            p.colorFuego = { (unsigned char)(200 + rand() % 55), (unsigned char)(50 + rand() % 50), 0, 255 };
            p.tamano = 0.05f + (rand() % 10 / 100.0f);
            p.flickerTimer = 0.05f + (rand() % 100) / 100.0f * 0.1f; 
        }

        if (p.vida <= 0.0f) {
            float theta = (rand() % 1000 / 1000.0f) * 2 * M_PI;
            float phi   = acosf(2.0f * (rand() % 1000 / 1000.0f) - 1.0f);
            float r     = (rand() % 1000 / 1000.0f) * radio;

            p.posicion = {
                posicion.x + r * sinf(phi) * cosf(theta),
                posicion.y + r * sinf(phi) * sinf(theta),
                posicion.z + r * cosf(phi)
            };

            Vector3 dir = {
                p.posicion.x - posicion.x,
                p.posicion.y - posicion.y,
                p.posicion.z - posicion.z
            };
            float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
            if (len > 0.001f) { dir.x /= len; dir.y /= len; dir.z /= len; }

            float speed = 0.5f * (0.5f + rand() % 100 / 100.0f);
            p.velocidad = { dir.x * speed, dir.y * speed, dir.z * speed };
            p.vida = 2.0f + (rand() % 100 / 100.0f);
        }
    }
}

void Portal::Dibujar() const {
    DrawCircle3D(posicion, radio, (Vector3){0,1,0}, 0.0f, colorAnillo);

    for (const auto &p : particulas) {
        DrawSphere(p.posicion, p.tamano, p.colorFuego);
    }
}
