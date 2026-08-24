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

    int GetFaseShader() const;
    float GetProgresoShader() const;
    float GetIntensidadShader() const;
    Vector3 GetCentroVisual() const;


    bool ListoParaDisparar() const {
        return cooldownRestante <= 0.0f;
    }

    float GetCooldownRestante() const {
        return cooldownRestante;
    }

    bool TieneParticulasActivas() const {
        return !particulas.empty();
    }

    static constexpr float COOLDOWN_ENJAMBRE = 60.0f;
    static constexpr float DISTANCIA_MINIMA_ENJAMBRE = 60.0f;
    static constexpr float RADIO_BUSQUEDA_SECUNDARIA = 100.0f;

private:
    enum class FaseEnjambre {
        INACTIVO,
        DISPERSION,
        SUSPENSION,
        REAGRUPACION,
        ATAQUE,
        COLAPSO
    };

    struct Particula {
        Vector3 posicion;
        Vector3 ecoCercano;
        Vector3 ecoLejano;
        Vector3 velocidad;
        Vector3 offsetBlob;

        unsigned int semilla;

        float vida;
        float vidaMaxima;

        bool estetica;
        bool activa;
    };

    struct Hipertraza {
        Vector3 inicio;
        Vector3 fin;
        Vector3 desfaseCentral;

        Color color;

        float vida;
        float vidaMaxima;
        float pulso;
        float velocidadPulso;
        float corteA;
        float corteB;

        unsigned int semilla;
    };

    std::vector<Particula> particulas;
    std::vector<Hipertraza> hipertrazas;

    FaseEnjambre fase;
    Enemy* objetivoActual;

    Vector3 origenDisparo;
    Vector3 centroEnjambre;
    Vector3 velocidadCentro;
    Vector3 puntoImpacto;

    float cooldownRestante;
    float tiempoFase;
    float tiempoTotal;
    float acumuladorHipertrazas;
    float tiempoFlashImpacto;

    unsigned int semillaEnjambre;

    static unsigned int contadorSemillas;

    static constexpr int PARTICULAS_INICIALES = 16;
    static constexpr int MAX_HIPERTRAZAS = 24;

    static constexpr float DURACION_DISPERSION = 0.62f;
    static constexpr float DURACION_SUSPENSION = 0.18f;
    static constexpr float DURACION_REAGRUPACION = 0.78f;
    static constexpr float DURACION_COLAPSO = 0.16f;

    static constexpr float VELOCIDAD_DISPERSION_MIN = 18.0f;
    static constexpr float VELOCIDAD_DISPERSION_MAX = 36.0f;
    static constexpr float VELOCIDAD_CENTRO = 35.0f;
    static constexpr float VELOCIDAD_REAGRUPACION = 4.0f;

    static constexpr float FUERZA_BLOB = 42.0f;
    static constexpr float AMORTIGUACION_BLOB = 10.0f;
    static constexpr float VELOCIDAD_MAXIMA_PARTICULA = 58.0f;

    static constexpr float RADIO_IMPACTO = 2.6f;
    static constexpr float DANO_TOTAL = 120.0f;

    static constexpr float VIDA_RESIDUO_MIN = 0.70f;
    static constexpr float VIDA_RESIDUO_MAX = 1.30f;

    void CambiarFase(FaseEnjambre nuevaFase);

    void ActualizarDispersion(float dt);
    void ActualizarSuspension(float dt);
    void ActualizarReagrupacion(
        float dt,
        std::vector<Enemy*>& enemigosVivos
    );
    void ActualizarAtaque(
        float dt,
        std::vector<Enemy*>& enemigosVivos
    );
    void ActualizarColapso(float dt);
    void ActualizarResiduos(float dt);

    void ActualizarEcos(Particula& particula, float dt);
    void ActualizarParticulasBlob(float dt, float escalaBlob);

    Vector3 CalcularOffsetAnimado(
        const Particula& particula,
        float escalaBlob
    ) const;

    Vector3 CalcularCentroParticulas() const;

    Enemy* BuscarObjetivo(
        const std::vector<Enemy*>& enemigosVivos
    ) const;

    void ConvertirEnResiduos(
        Vector3 centroExplosion,
        bool mostrarFlash
    );

    void ActualizarHipertrazas(float dt);
    void CrearHipertraza();
    void DibujarHipertrazas() const;
};

#endif
