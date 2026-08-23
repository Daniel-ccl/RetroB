#ifndef ATAQUE_ENJAMBRE_H
#define ATAQUE_ENJAMBRE_H

#include "raylib.h"
#include "enemy.h"
#include <vector>

class AtaqueEnjambre {
public:
    AtaqueEnjambre();

    bool Disparar(Vector3 origen, Enemy* objetivoInicial);
    void Actualizar(float dt, std::vector<Enemy*>& enemigosVivos);
    void Dibujar() const;

    bool  ListoParaDisparar()      const { return cooldownRestante <= 0.0f; }
    float GetCooldownRestante()    const { return cooldownRestante; }
    bool  TieneParticulasActivas() const { return !particulas.empty(); }

    static constexpr float COOLDOWN_ENJAMBRE          = 60.0f;
    static constexpr float DISTANCIA_MINIMA_ENJAMBRE  = 60.0f;
    static constexpr float TIEMPO_BUSQUEDA_SECUNDARIA = 2.0f;
    static constexpr float RADIO_BUSQUEDA_SECUNDARIA  = 100.0f;
    static constexpr float RADIO_DESCONEXION_BLOB     = 1.0f;

private:
    enum EstadoParticula { ATACANDO, BUSCANDO, ESTETICA };

    struct Particula {
        Vector3 posicion;
        Vector3 velocidad;
        EstadoParticula estado;
        Enemy* objetivo;
        Vector3 origenReferencia;
        float distanciaReferencia;
        unsigned int semilla;
        float tiempoBusqueda;
        float vida;
        bool activa;
    };

    std::vector<Particula> particulas;
    float cooldownRestante;

    static unsigned int contadorSemillas;

    static constexpr int   PARTICULAS_INICIALES         = 10;
    static constexpr int   PARTICULAS_POR_FRAGMENTACION = 3;
    static constexpr float VELOCIDAD_PARTICULA          = 22.0f;
    static constexpr float RADIO_IMPACTO                = 1.2f;
    static constexpr float DANO_POR_PARTICULA           = 12.0f;
    static constexpr float VIDA_ESTETICA                = 1.5f;
    static constexpr float DIST_MAX_CONEXION            = 9.0f;
    static constexpr float VARIACION_ENJAMBRE           = 5.0f;
    static constexpr float RADIO_SPAWN_ENJAMBRE         = 0.8f;

    void ActualizarParticula(Particula& p, float dt, std::vector<Enemy*>& enemigosVivos,
                              std::vector<Particula>& nuevas);
    void Fragmentar(const Particula& original, std::vector<Particula>& nuevas) const;
    bool ParticulaConectable(const Particula& p) const;
    static bool EsOfensiva(EstadoParticula e) { return e == ATACANDO || e == BUSCANDO; }
};

#endif
