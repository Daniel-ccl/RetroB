
#ifndef AVION_H
#define AVION_H

#include "raylib.h"
#include "raymath.h"
#include "bomba.h"
#include "proyectil.h"
#include "salud.h"
#include <vector>

class Saturno; 

class Avion {
private:
    Vector3 posicion;
    float rotacionY;          
    float rotacionVelocidad;  
    float velocidad;
    float velocidadBase;
    float velocidadBoost;

    float anguloBanco;
    float anguloPitch;

    float velocidadVertical;  // momentum vertical en modo vuelo libre (no orbital)

    bool boosteando;          

    Salud salud{900.0f};      

    static const int MAX_TRAIL = 50;
    Vector3 trail[MAX_TRAIL];
    Vector2 trailJitter[MAX_TRAIL];   
    int trailIndex;

    std::vector<Bomba> bombas;

    Saturno* planetaActual;
    float escala;

    Vector2 miraPos;          
    float   miraTiempoSoft;   
    float   miraTiempoHard;   
    bool    miraTrabada;      
    bool    miraSoftLock;     
    bool    miraTieneObjetivo; 
    Vector3 miraObjetivoPos3D; 

    static constexpr float MIRA_VELOCIDAD_SEGUIMIENTO = 900.0f; 
    static constexpr float MIRA_TIEMPO_SOFT_NECESARIO = 0.5f;  
    static constexpr float MIRA_TIEMPO_HARD_NECESARIO = 1.0f;  
    static constexpr float MIRA_RADIO_REGION           = 220.0f; 

    std::vector<Proyectil> misiles;
    float cooldownDisparo = 0.0f; 

    static constexpr float MISIL_COOLDOWN     = 3.0f;   
    static constexpr float MISIL_VELOCIDAD    = 25.0f;  
    static constexpr float MISIL_ALCANCE_MAX  = 500.0f;
    static constexpr float MISIL_TAMANO        = 0.35f; 
    static constexpr float MISIL_TIEMPO_ERRANTE = 1.0f; 
    static constexpr float MISIL_TIEMPO_RECTO = 0.3f; 
    static constexpr float MISIL_SEPARACION_LANZAMIENTO = 0.8f; 

    static constexpr float MISIL_RADIO_IMPACTO_INTERNO = 0.05f;

    void ActualizarVueloLibre(float dt);
    void ActualizarOrbital(float dt);
    void ActualizarBoost(float dt);
    void DibujarLineasDeVelocidad() const;

public:
    Avion();
    void Actualizar(float dt, Vector3 objetivoPos, bool objetivoExiste);
    void Dibujar();
    Vector3 GetPosicion() const;
    float GetRotacionY() const;
    bool GetBoosteando() const { return boosteando; }

    void ActualizarMira(float dt, const Camera3D& cam, Vector3 objetivoPos, bool objetivoExiste,
                        int screenWidth, int screenHeight);
    void DibujarMira() const; 
    bool GetMiraTrabada() const { return miraTrabada; }
    bool GetMiraSoftLock() const { return miraSoftLock; }
    float GetFraccionHardLock() const {
        if (MIRA_TIEMPO_HARD_NECESARIO <= 0.0f) return miraSoftLock ? 1.0f : 0.0f;
        return Clamp(miraTiempoHard / MIRA_TIEMPO_HARD_NECESARIO, 0.0f, 1.0f);
    }

    void DibujarConoLock() const;

    bool RevisarImpacto(Vector3 objetivoPos, float radioObjetivo);

    void RecibirDano(float cantidad) { salud.RecibirDano(cantidad); }
    bool EstaVivo() const { return salud.EstaVivo(); }
    float GetPorcentajeSalud() const { return salud.GetPorcentaje(); }

    float GetFraccionBoost() const {
        if (velocidadBoost <= 0.0f) return 0.0f;
        return Clamp((velocidad - velocidadBase) / velocidadBoost, 0.0f, 1.0f);
    }
    float GetFraccionCooldownListo() const {
        if (MISIL_COOLDOWN <= 0.0f) return 1.0f;
        return Clamp(1.0f - (cooldownDisparo / MISIL_COOLDOWN), 0.0f, 1.0f);
    }
    bool GetMisilListo() const { return cooldownDisparo <= 0.0f; }
    int GetNumMisilesActivos() const { return (int)misiles.size(); }

    void SetPosicion(const Vector3& pos);
    void SetPlanetaActual(Saturno* planeta);
    void SetEscala(float esc);
};

#endif
