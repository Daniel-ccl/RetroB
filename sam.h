#ifndef SAM_H
#define SAM_H

#include "raylib.h"
#include "salud.h"
#include "parche_flores.h"
#include "proyectil.h"
#include "enemy.h"
#include "level_data.h"
#include <vector>
#include <string>

enum EstadoSam {
    SAM_BUSCANDO,   
    SAM_BLOQUEADO,  
    SAM_DISPARADO   
};

class Sam : public Enemy {
public:
    Sam(Vector3 posBase, const std::string& id = "", const EnemyConfig& cfg = EnemyConfig{});

    bool Actualizar(float dt, Vector3 posAvion) override;
    void Dibujar() const override;            
    void DibujarCono() const;        
    void DibujarBarraSalud() const override;  

    EstadoSam GetEstado() const { return estado; } 
    Vector3 GetPosicion() const override { return posicionBase; }
    void SetPosicion(Vector3 posBase) override; 

    void RecibirDano(float cantidad) override { salud.RecibirDano(cantidad); }
    bool EstaVivo() const override { return salud.EstaVivo(); }
    float GetPorcentajeSalud() const override { return salud.GetPorcentaje(); }

    const std::string& GetId() const override { return id; }
    float GetDanoMisil() const { return danoMisil; }

private:
    std::string id;
    Vector3 posicionBase;   
    Vector3 posicionOjo;    

    float anguloActual;     
    float velocidadRotacion;

    float anguloConoRad;    
    float alcance;          

    EstadoSam estado;
    float tiempoLock;       
    float tiempoLockNecesario; 
    float cooldownDisparo;  

    float radioImpacto;     

    Salud salud{120.0f};    

    ParcheFlores floresMuerte; 
    bool floresGeneradas = false; 

    std::vector<Proyectil> misiles;

    float misilVelocidad;
    int   misilesPorVolea;
    float danoMisil; 

    static constexpr float MISIL_ALCANCE_MAX    = 500.0f;
    static constexpr float MISIL_TAMANO         = 0.22f;
    static constexpr float MISIL_TIEMPO_ERRANTE = 1.0f;
    static constexpr float MISIL_TIEMPO_RECTO   = 0.3f;
    static constexpr float MISIL_SPREAD_GRADOS  = 12.0f;

    bool AvionEnCono(Vector3 posAvion) const;
    Vector3 DireccionCono() const; 

    void DispararMisiles(Vector3 posAvion);
    bool ActualizarMisiles(float dt, Vector3 posAvion); 
    void DibujarMisiles() const;
};

#endif
