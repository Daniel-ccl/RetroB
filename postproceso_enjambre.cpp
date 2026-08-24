#include "postproceso_enjambre.h"

#include "raymath.h"

PostProcesoEnjambre::PostProcesoEnjambre() {
    shader = {};
    centroAnterior = {0.5f, 0.5f};
    direccionPantalla = {1.0f, 0.0f};

    cargado = false;
    teniaEfecto = false;
}

bool PostProcesoEnjambre::Cargar(
    int ancho,
    int alto
) {
    shader = LoadShader(
        nullptr,
        "shaders/swarm_post.fs"
    );

    if (shader.id == 0) {
        cargado = false;
        return false;
    }

    ubicaciones.resolucion = GetShaderLocation(
        shader,
        "resolucion"
    );

    ubicaciones.centroEnjambre = GetShaderLocation(
        shader,
        "centroEnjambre"
    );

    ubicaciones.direccionEnjambre = GetShaderLocation(
        shader,
        "direccionEnjambre"
    );

    ubicaciones.tiempo = GetShaderLocation(
        shader,
        "tiempo"
    );

    ubicaciones.fase = GetShaderLocation(
        shader,
        "fase"
    );

    ubicaciones.progreso = GetShaderLocation(
        shader,
        "progreso"
    );

    ubicaciones.intensidad = GetShaderLocation(
        shader,
        "intensidad"
    );

    const Vector2 resolucion = {
        static_cast<float>(ancho),
        static_cast<float>(alto)
    };

    SetShaderValue(
        shader,
        ubicaciones.resolucion,
        &resolucion,
        SHADER_UNIFORM_VEC2
    );

    cargado = true;
    return true;
}

void PostProcesoEnjambre::Descargar() {
    if (!cargado) {
        return;
    }

    UnloadShader(shader);
    shader = {};
    cargado = false;
}

void PostProcesoEnjambre::Actualizar(
    const AtaqueEnjambre& ataque,
    const Camera3D& camera,
    int ancho,
    int alto,
    bool habilitado
) {
    if (!cargado) {
        return;
    }

    const Vector3 centroMundo =
        ataque.GetCentroVisual();

    const Vector2 centroPixeles =
        GetWorldToScreen(
            centroMundo,
            camera
        );

    const bool centroVisible =
        centroPixeles.x >= -100.0f &&
        centroPixeles.x <=
            static_cast<float>(ancho) + 100.0f &&
        centroPixeles.y >= -100.0f &&
        centroPixeles.y <=
            static_cast<float>(alto) + 100.0f;

    const Vector2 centroNormalizado = {
        centroPixeles.x /
            static_cast<float>(ancho),

        centroPixeles.y /
            static_cast<float>(alto)
    };

    float intensidad =
        ataque.GetIntensidadShader();

    if (!habilitado || !centroVisible) {
        intensidad = 0.0f;
    }

    if (intensidad > 0.001f) {
        if (!teniaEfecto) {
            centroAnterior = centroNormalizado;
            direccionPantalla = {1.0f, 0.0f};
        } else {
            const Vector2 desplazamiento =
                Vector2Subtract(
                    centroNormalizado,
                    centroAnterior
                );

            if (
                Vector2Length(desplazamiento) >
                0.00001f
            ) {
                const Vector2 direccionNueva =
                    Vector2Normalize(
                        desplazamiento
                    );

                direccionPantalla = Vector2Lerp(
                    direccionPantalla,
                    direccionNueva,
                    0.30f
                );

                if (
                    Vector2Length(direccionPantalla) >
                    0.00001f
                ) {
                    direccionPantalla =
                        Vector2Normalize(
                            direccionPantalla
                        );
                }
            }
        }

        centroAnterior = centroNormalizado;
        teniaEfecto = true;
    } else {
        teniaEfecto = false;
    }

    const float tiempo =
        static_cast<float>(GetTime());

    const float fase =
        static_cast<float>(
            ataque.GetFaseShader()
        );

    const float progreso =
        ataque.GetProgresoShader();

    SetShaderValue(
        shader,
        ubicaciones.centroEnjambre,
        &centroNormalizado,
        SHADER_UNIFORM_VEC2
    );

    SetShaderValue(
        shader,
        ubicaciones.direccionEnjambre,
        &direccionPantalla,
        SHADER_UNIFORM_VEC2
    );

    SetShaderValue(
        shader,
        ubicaciones.tiempo,
        &tiempo,
        SHADER_UNIFORM_FLOAT
    );

    SetShaderValue(
        shader,
        ubicaciones.fase,
        &fase,
        SHADER_UNIFORM_FLOAT
    );

    SetShaderValue(
        shader,
        ubicaciones.progreso,
        &progreso,
        SHADER_UNIFORM_FLOAT
    );

    SetShaderValue(
        shader,
        ubicaciones.intensidad,
        &intensidad,
        SHADER_UNIFORM_FLOAT
    );
}

void PostProcesoEnjambre::Dibujar(
    const RenderTexture2D& escena
) const {
    const Rectangle origen = {
        0.0f,
        0.0f,
        static_cast<float>(escena.texture.width),
        -static_cast<float>(escena.texture.height)
    };

    if (!cargado) {
        DrawTextureRec(
            escena.texture,
            origen,
            {0.0f, 0.0f},
            WHITE
        );

        return;
    }

    BeginShaderMode(shader);

    DrawTextureRec(
        escena.texture,
        origen,
        {0.0f, 0.0f},
        WHITE
    );

    EndShaderMode();
}
