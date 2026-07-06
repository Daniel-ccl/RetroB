#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include "raylib.h"
#include <string>
#include <vector>

struct EnemyConfig {
    float hp            = -1.0f;
    int   missileCount  = -1;
    float missileDamage = -1.0f;
    float missileSpeed  = -1.0f;
    float coneRange     = -1.0f;
    float coneAngle     = -1.0f;
};

struct EnemySpawn {
    std::string tipo;       
    std::string id;         
    Vector3     posicion;
    float       rotacion;   
    EnemyConfig config;
};

struct Objective {
    std::string              tipo;        
    std::vector<std::string> targetIds;  
    Vector3                  location;   
    float                    radius;     
    float                    duration;   
    std::string              descripcion;
};

struct LevelData {
    std::string nombre;
    int         tamano;   
    int         celdas;   

    std::vector<std::vector<int>> alturas;

    Vector3 jugadorSpawn;
    float   jugadorRotacion;

    std::vector<EnemySpawn> enemigos;
    std::vector<Objective>  objetivos; 
};

#endif
