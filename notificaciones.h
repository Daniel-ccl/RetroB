#ifndef NOTIFICACIONES_H
#define NOTIFICACIONES_H

#include "raylib.h"
#include <vector>
#include <string>

enum TipoNotificacion {
    NOTIF_INFO,      
    NOTIF_EXITO,     
    NOTIF_PELIGRO    
};

class Notificaciones {
public:
    void Empujar(const std::string& mensaje, TipoNotificacion tipo = NOTIF_INFO);

    void Actualizar(float dt);
    void Dibujar(int screenWidth, int screenHeight) const; 

private:
    struct Entrada {
        std::string mensaje;
        TipoNotificacion tipo;
        float tiempoVida;       
        float tiempoVidaMax;    
    };

    std::vector<Entrada> entradas;

    static constexpr float DURACION_TOTAL = 3.5f;  
    static constexpr float DURACION_FADE  = 0.6f;   
    static constexpr int   MAX_VISIBLES   = 5;       
};

#endif
