#ifndef SALUD_H
#define SALUD_H

class Salud {
public:
    Salud(float hpMax = 100.0f);

    void RecibirDano(float cantidad);
    void Curar(float cantidad); 
    void Reiniciar();           

    bool  EstaVivo()     const { return hpActual > 0.0f; }
    float GetHpActual()  const { return hpActual; }
    float GetHpMax()     const { return hpMax; }
    float GetPorcentaje() const { return hpMax > 0.0f ? (hpActual / hpMax) : 0.0f; } 

private:
    float hpMax;
    float hpActual;
};

#endif
