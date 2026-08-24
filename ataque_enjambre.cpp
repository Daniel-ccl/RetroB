#include "ataque_enjambre.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

unsigned int AtaqueEnjambre::contadorSemillas = 40000;

namespace {

float Aleatorio(float minimo, float maximo) {
    const float t =
        static_cast<float>(rand()) /
        static_cast<float>(RAND_MAX);

    return minimo + (maximo - minimo) * t;
}

float Saturar(float valor) {
    return std::max(0.0f, std::min(valor, 1.0f));
}

Vector3 VectorAleatorioUnitario() {
    Vector3 direccion;

    do {
        direccion = {
            Aleatorio(-1.0f, 1.0f),
            Aleatorio(-1.0f, 1.0f),
            Aleatorio(-1.0f, 1.0f)
        };
    } while (Vector3Length(direccion) < 0.001f);

    return Vector3Normalize(direccion);
}

Vector3 LimitarMagnitud(Vector3 vector, float magnitudMaxima) {
    const float magnitud = Vector3Length(vector);

    if (magnitud <= magnitudMaxima || magnitud < 0.001f) {
        return vector;
    }

    return Vector3Scale(
        vector,
        magnitudMaxima / magnitud
    );
}

Color AplicarAlpha(Color color, float factor) {
    color.a = static_cast<unsigned char>(
        static_cast<float>(color.a) * Saturar(factor)
    );

    return color;
}

Vector3 SumarDesfase(Vector3 posicion, Vector3 desfase) {
    return Vector3Add(posicion, desfase);
}

} // namespace

AtaqueEnjambre::AtaqueEnjambre() {
    fase = FaseEnjambre::INACTIVO;
    objetivoActual = nullptr;

    origenDisparo = {0.0f, 0.0f, 0.0f};
    centroEnjambre = {0.0f, 0.0f, 0.0f};
    velocidadCentro = {0.0f, 0.0f, 0.0f};
    puntoImpacto = {0.0f, 0.0f, 0.0f};

    cooldownRestante = 0.0f;
    tiempoFase = 0.0f;
    tiempoTotal = 0.0f;
    acumuladorHipertrazas = 0.0f;
    tiempoFlashImpacto = 0.0f;

    semillaEnjambre = contadorSemillas++;
}

int AtaqueEnjambre::GetFaseShader() const {
    /*
     * El flash de impacto tiene prioridad incluso cuando
     * las partículas ya se convirtieron en residuos.
     */
    if (tiempoFlashImpacto > 0.0f) {
        return 5;
    }

    switch (fase) {
        case FaseEnjambre::DISPERSION:
            return 1;

        case FaseEnjambre::SUSPENSION:
            return 2;

        case FaseEnjambre::REAGRUPACION:
            return 3;

        case FaseEnjambre::ATAQUE:
        case FaseEnjambre::COLAPSO:
            return 4;

        case FaseEnjambre::INACTIVO:
            return 0;
    }

    return 0;
}

float AtaqueEnjambre::GetProgresoShader() const {
    if (tiempoFlashImpacto > 0.0f) {
        return Saturar(
            1.0f -
            tiempoFlashImpacto / 0.25f
        );
    }

    switch (fase) {
        case FaseEnjambre::DISPERSION:
            return Saturar(
                tiempoFase /
                DURACION_DISPERSION
            );

        case FaseEnjambre::SUSPENSION:
            return Saturar(
                tiempoFase /
                DURACION_SUSPENSION
            );

        case FaseEnjambre::REAGRUPACION:
            return Saturar(
                tiempoFase /
                DURACION_REAGRUPACION
            );

        case FaseEnjambre::COLAPSO:
            return Saturar(
                tiempoFase /
                DURACION_COLAPSO
            );

        case FaseEnjambre::ATAQUE:
            return fmodf(
                tiempoFase * 0.65f,
                1.0f
            );

        case FaseEnjambre::INACTIVO:
            return 0.0f;
    }

    return 0.0f;
}

float AtaqueEnjambre::GetIntensidadShader() const {
    if (tiempoFlashImpacto > 0.0f) {
        return 1.0f;
    }

    switch (fase) {
        case FaseEnjambre::DISPERSION:
            return 0.90f;

        case FaseEnjambre::SUSPENSION:
            return 0.82f;

        case FaseEnjambre::REAGRUPACION:
            return 0.88f;

        case FaseEnjambre::ATAQUE:
            return 0.78f;

        case FaseEnjambre::COLAPSO:
            return 1.0f;

        case FaseEnjambre::INACTIVO:
            return 0.0f;
    }

    return 0.0f;
}

Vector3 AtaqueEnjambre::GetCentroVisual() const {
    if (tiempoFlashImpacto > 0.0f) {
        return puntoImpacto;
    }

    return centroEnjambre;
}

bool AtaqueEnjambre::Disparar(
    Vector3 origen,
    Enemy* objetivoInicial
) {
    if (!ListoParaDisparar()) {
        return false;
    }

    if (!objetivoInicial || !objetivoInicial->EstaVivo()) {
        return false;
    }

    if (
        Vector3Distance(origen, objetivoInicial->GetPosicion()) <
        DISTANCIA_MINIMA_ENJAMBRE
    ) {
        return false;
    }

    particulas.clear();
    hipertrazas.clear();

    origenDisparo = origen;
    centroEnjambre = origen;
    velocidadCentro = {0.0f, 0.0f, 0.0f};

    objetivoActual = objetivoInicial;

    tiempoTotal = 0.0f;
    tiempoFlashImpacto = 0.0f;
    acumuladorHipertrazas = 0.0f;

    semillaEnjambre = contadorSemillas++;

    particulas.reserve(PARTICULAS_INICIALES);
    hipertrazas.reserve(MAX_HIPERTRAZAS);

    for (int i = 0; i < PARTICULAS_INICIALES; i++) {
        Particula particula;

        const Vector3 direccionDispersion =
            VectorAleatorioUnitario();

        const float velocidadDispersion = Aleatorio(
            VELOCIDAD_DISPERSION_MIN,
            VELOCIDAD_DISPERSION_MAX
        );

        const Vector3 offsetInicial = Vector3Scale(
            VectorAleatorioUnitario(),
            Aleatorio(0.05f, 0.45f)
        );

        particula.posicion = Vector3Add(
            origen,
            offsetInicial
        );

        particula.ecoCercano = particula.posicion;
        particula.ecoLejano = particula.posicion;

        particula.velocidad = Vector3Scale(
            direccionDispersion,
            velocidadDispersion
        );

        /*
         * Este offset define la posición que ocupará la partícula
         * dentro del blob después de la reagrupación.
         */
        particula.offsetBlob = Vector3Scale(
            VectorAleatorioUnitario(),
            Aleatorio(0.8f, 2.2f)
        );

        particula.semilla = contadorSemillas++;

        particula.vida = 0.0f;
        particula.vidaMaxima = 0.0f;

        particula.estetica = false;
        particula.activa = true;

        particulas.push_back(particula);
    }

    cooldownRestante = COOLDOWN_ENJAMBRE;
    CambiarFase(FaseEnjambre::DISPERSION);

    return true;
}

void AtaqueEnjambre::CambiarFase(
    FaseEnjambre nuevaFase
) {
    fase = nuevaFase;
    tiempoFase = 0.0f;
}

void AtaqueEnjambre::Actualizar(
    float dt,
    std::vector<Enemy*>& enemigosVivos
) {
    if (dt <= 0.0f) {
        return;
    }

    /*
     * Evita que un frame extremadamente lento desestabilice
     * los resortes del blob.
     */
    dt = std::min(dt, 0.05f);

    cooldownRestante = std::max(
        0.0f,
        cooldownRestante - dt
    );

    tiempoFlashImpacto = std::max(
        0.0f,
        tiempoFlashImpacto - dt
    );

    tiempoTotal += dt;
    tiempoFase += dt;

    switch (fase) {
        case FaseEnjambre::INACTIVO:
            break;

        case FaseEnjambre::DISPERSION:
            ActualizarDispersion(dt);
            break;

        case FaseEnjambre::SUSPENSION:
            ActualizarSuspension(dt);
            break;

        case FaseEnjambre::REAGRUPACION:
            ActualizarReagrupacion(
                dt,
                enemigosVivos
            );
            break;

        case FaseEnjambre::ATAQUE:
            ActualizarAtaque(
                dt,
                enemigosVivos
            );
            break;

        case FaseEnjambre::COLAPSO:
            ActualizarColapso(dt);
            break;
    }

    ActualizarResiduos(dt);
    ActualizarHipertrazas(dt);

    particulas.erase(
        std::remove_if(
            particulas.begin(),
            particulas.end(),
            [](const Particula& particula) {
                return !particula.activa;
            }
        ),
        particulas.end()
    );
}

void AtaqueEnjambre::ActualizarEcos(
    Particula& particula,
    float dt
) {
    particula.ecoLejano = Vector3Lerp(
        particula.ecoLejano,
        particula.ecoCercano,
        Saturar(dt * 8.0f)
    );

    particula.ecoCercano = Vector3Lerp(
        particula.ecoCercano,
        particula.posicion,
        Saturar(dt * 18.0f)
    );
}

void AtaqueEnjambre::ActualizarDispersion(float dt) {
    for (Particula& particula : particulas) {
        if (!particula.activa || particula.estetica) {
            continue;
        }

        ActualizarEcos(particula, dt);

        particula.posicion = Vector3Add(
            particula.posicion,
            Vector3Scale(particula.velocidad, dt)
        );

        /*
         * La dispersión comienza violentamente y pierde fuerza.
         * powf permite que la amortiguación sea independiente del FPS.
         */
        particula.velocidad = Vector3Scale(
            particula.velocidad,
            powf(0.055f, dt)
        );
    }

    centroEnjambre = CalcularCentroParticulas();

    if (tiempoFase >= DURACION_DISPERSION) {
        CambiarFase(FaseEnjambre::SUSPENSION);
    }
}

void AtaqueEnjambre::ActualizarSuspension(float dt) {
    for (Particula& particula : particulas) {
        if (!particula.activa || particula.estetica) {
            continue;
        }

        ActualizarEcos(particula, dt);

        particula.velocidad = Vector3Scale(
            particula.velocidad,
            powf(0.0001f, dt)
        );

        const float faseLocal =
            tiempoTotal * 20.0f +
            static_cast<float>(particula.semilla) * 0.013f;

        const Vector3 interferencia = {
            sinf(faseLocal) * 0.55f,
            cosf(faseLocal * 1.31f) * 0.35f,
            sinf(faseLocal * 0.83f) * 0.55f
        };

        particula.velocidad = Vector3Add(
            particula.velocidad,
            Vector3Scale(interferencia, dt)
        );

        particula.posicion = Vector3Add(
            particula.posicion,
            Vector3Scale(particula.velocidad, dt)
        );
    }

    centroEnjambre = CalcularCentroParticulas();

    if (tiempoFase >= DURACION_SUSPENSION) {
        CambiarFase(FaseEnjambre::REAGRUPACION);
    }
}

void AtaqueEnjambre::ActualizarReagrupacion(
    float dt,
    std::vector<Enemy*>& enemigosVivos
) {
    objetivoActual = BuscarObjetivo(enemigosVivos);

    if (!objetivoActual) {
        ConvertirEnResiduos(
            centroEnjambre,
            false
        );
        return;
    }

    const Vector3 posicionObjetivo =
        objetivoActual->GetPosicion();

    Vector3 haciaObjetivo = Vector3Subtract(
        posicionObjetivo,
        centroEnjambre
    );

    const float distancia = Vector3Length(haciaObjetivo);

    if (distancia > 0.001f) {
        haciaObjetivo = Vector3Scale(
            haciaObjetivo,
            1.0f / distancia
        );

        const float desplazamiento = std::min(
            distancia,
            VELOCIDAD_REAGRUPACION * dt
        );

        centroEnjambre = Vector3Add(
            centroEnjambre,
            Vector3Scale(
                haciaObjetivo,
                desplazamiento
            )
        );
    }

    const float progreso = Saturar(
        tiempoFase / DURACION_REAGRUPACION
    );

    /*
     * Comienza disperso y se comprime gradualmente.
     */
    const float escalaBlob =
        1.65f - progreso * 0.65f;

    ActualizarParticulasBlob(
        dt,
        escalaBlob
    );

    if (tiempoFase >= DURACION_REAGRUPACION) {
        velocidadCentro = Vector3Scale(
            haciaObjetivo,
            VELOCIDAD_CENTRO * 0.25f
        );

        CambiarFase(FaseEnjambre::ATAQUE);
    }
}

void AtaqueEnjambre::ActualizarAtaque(
    float dt,
    std::vector<Enemy*>& enemigosVivos
) {
    objetivoActual = BuscarObjetivo(enemigosVivos);

    if (!objetivoActual) {
        ConvertirEnResiduos(
            centroEnjambre,
            false
        );
        return;
    }

    const Vector3 posicionObjetivo =
        objetivoActual->GetPosicion();

    Vector3 direccionObjetivo = Vector3Subtract(
        posicionObjetivo,
        centroEnjambre
    );

    const float distanciaObjetivo =
        Vector3Length(direccionObjetivo);

    if (distanciaObjetivo <= RADIO_IMPACTO) {
        puntoImpacto = posicionObjetivo;
        centroEnjambre = puntoImpacto;
        velocidadCentro = {0.0f, 0.0f, 0.0f};

        CambiarFase(FaseEnjambre::COLAPSO);
        return;
    }

    direccionObjetivo = Vector3Normalize(
        direccionObjetivo
    );

    /*
     * Dos ejes perpendiculares crean el movimiento curvo
     * del centro compartido.
     */
    Vector3 ejeLateral = Vector3CrossProduct(
        direccionObjetivo,
        {0.0f, 1.0f, 0.0f}
    );

    if (Vector3Length(ejeLateral) < 0.001f) {
        ejeLateral = {1.0f, 0.0f, 0.0f};
    } else {
        ejeLateral = Vector3Normalize(ejeLateral);
    }

    Vector3 ejeVertical = Vector3CrossProduct(
        ejeLateral,
        direccionObjetivo
    );

    if (Vector3Length(ejeVertical) > 0.001f) {
        ejeVertical = Vector3Normalize(ejeVertical);
    }

    const float fuerzaCurva = Saturar(
        distanciaObjetivo / 45.0f
    );

    Vector3 direccionCurvada = direccionObjetivo;

    direccionCurvada = Vector3Add(
        direccionCurvada,
        Vector3Scale(
            ejeLateral,
            sinf(tiempoTotal * 8.5f) *
                0.20f *
                fuerzaCurva
        )
    );

    direccionCurvada = Vector3Add(
        direccionCurvada,
        Vector3Scale(
            ejeVertical,
            cosf(tiempoTotal * 6.3f) *
                0.10f *
                fuerzaCurva
        )
    );

    direccionCurvada = Vector3Normalize(
        direccionCurvada
    );

    const Vector3 velocidadDeseada = Vector3Scale(
        direccionCurvada,
        VELOCIDAD_CENTRO
    );

    velocidadCentro = Vector3Lerp(
        velocidadCentro,
        velocidadDeseada,
        Saturar(dt * 5.0f)
    );

    centroEnjambre = Vector3Add(
        centroEnjambre,
        Vector3Scale(velocidadCentro, dt)
    );

    /*
     * Todas las partículas siguen el mismo centro,
     * por eso ahora se percibe como una sola entidad.
     */
    ActualizarParticulasBlob(
        dt,
        0.90f
    );
}

void AtaqueEnjambre::ActualizarColapso(float dt) {
    const float progreso = Saturar(
        tiempoFase / DURACION_COLAPSO
    );

    centroEnjambre = puntoImpacto;

    /*
     * El radio del blob colapsa rápidamente hacia cero.
     */
    ActualizarParticulasBlob(
        dt,
        1.0f - progreso
    );

    if (tiempoFase < DURACION_COLAPSO) {
        return;
    }

    if (objetivoActual && objetivoActual->EstaVivo()) {
        objetivoActual->RecibirDano(DANO_TOTAL);
    }

    ConvertirEnResiduos(
        puntoImpacto,
        true
    );
}

void AtaqueEnjambre::ActualizarParticulasBlob(
    float dt,
    float escalaBlob
) {
    for (Particula& particula : particulas) {
        if (!particula.activa || particula.estetica) {
            continue;
        }

        ActualizarEcos(particula, dt);

        const Vector3 offsetAnimado =
            CalcularOffsetAnimado(
                particula,
                escalaBlob
            );

        const Vector3 posicionDeseada = Vector3Add(
            centroEnjambre,
            offsetAnimado
        );

        const Vector3 error = Vector3Subtract(
            posicionDeseada,
            particula.posicion
        );

        Vector3 aceleracion = Vector3Scale(
            error,
            FUERZA_BLOB
        );

        aceleracion = Vector3Subtract(
            aceleracion,
            Vector3Scale(
                particula.velocidad,
                AMORTIGUACION_BLOB
            )
        );

        particula.velocidad = Vector3Add(
            particula.velocidad,
            Vector3Scale(aceleracion, dt)
        );

        particula.velocidad = LimitarMagnitud(
            particula.velocidad,
            VELOCIDAD_MAXIMA_PARTICULA
        );

        particula.posicion = Vector3Add(
            particula.posicion,
            Vector3Scale(
                particula.velocidad,
                dt
            )
        );
    }
}

Vector3 AtaqueEnjambre::CalcularOffsetAnimado(
    const Particula& particula,
    float escalaBlob
) const {
    const float angulo =
        tiempoTotal * 3.1f +
        static_cast<float>(particula.semilla) * 0.017f;

    const float coseno = cosf(angulo);
    const float seno = sinf(angulo);

    Vector3 offset = particula.offsetBlob;

    /*
     * Rotación alrededor del eje Y.
     */
    const float xRotado =
        offset.x * coseno -
        offset.z * seno;

    const float zRotado =
        offset.x * seno +
        offset.z * coseno;

    offset.x = xRotado;
    offset.z = zRotado;

    /*
     * Vibración vertical asincrónica para que el blob
     * no parezca una esfera rígida.
     */
    offset.y += sinf(
        tiempoTotal * 7.0f +
        static_cast<float>(particula.semilla) * 0.031f
    ) * 0.45f;

    return Vector3Scale(
        offset,
        escalaBlob
    );
}

Vector3 AtaqueEnjambre::CalcularCentroParticulas() const {
    Vector3 promedio = {0.0f, 0.0f, 0.0f};
    int cantidad = 0;

    for (const Particula& particula : particulas) {
        if (
            !particula.activa ||
            particula.estetica
        ) {
            continue;
        }

        promedio = Vector3Add(
            promedio,
            particula.posicion
        );

        cantidad++;
    }

    if (cantidad == 0) {
        return centroEnjambre;
    }

    return Vector3Scale(
        promedio,
        1.0f / static_cast<float>(cantidad)
    );
}

Enemy* AtaqueEnjambre::BuscarObjetivo(
    const std::vector<Enemy*>& enemigosVivos
) const {
    if (
        objetivoActual &&
        objetivoActual->EstaVivo()
    ) {
        return objetivoActual;
    }

    Enemy* masCercano = nullptr;
    float distanciaMinima =
        RADIO_BUSQUEDA_SECUNDARIA;

    for (Enemy* enemigo : enemigosVivos) {
        if (!enemigo || !enemigo->EstaVivo()) {
            continue;
        }

        const float distancia = Vector3Distance(
            centroEnjambre,
            enemigo->GetPosicion()
        );

        if (distancia < distanciaMinima) {
            distanciaMinima = distancia;
            masCercano = enemigo;
        }
    }

    return masCercano;
}

void AtaqueEnjambre::ConvertirEnResiduos(
    Vector3 centroExplosion,
    bool mostrarFlash
) {
    for (Particula& particula : particulas) {
        if (!particula.activa) {
            continue;
        }

        ActualizarEcos(particula, 1.0f / 60.0f);

        Vector3 direccionExplosion = Vector3Subtract(
            particula.posicion,
            centroExplosion
        );

        direccionExplosion = Vector3Add(
            direccionExplosion,
            Vector3Scale(
                VectorAleatorioUnitario(),
                0.8f
            )
        );

        if (Vector3Length(direccionExplosion) < 0.001f) {
            direccionExplosion =
                VectorAleatorioUnitario();
        } else {
            direccionExplosion = Vector3Normalize(
                direccionExplosion
            );
        }

        if (mostrarFlash) {
            /*
             * Acerca las partículas al punto de colapso
             * antes de expulsarlas.
             */
            particula.posicion = Vector3Lerp(
                particula.posicion,
                centroExplosion,
                0.78f
            );
        }

        particula.velocidad = Vector3Scale(
            direccionExplosion,
            Aleatorio(11.0f, 28.0f)
        );

        particula.vida = Aleatorio(
            VIDA_RESIDUO_MIN,
            VIDA_RESIDUO_MAX
        );

        particula.vidaMaxima = particula.vida;
        particula.estetica = true;
    }

    if (mostrarFlash) {
        puntoImpacto = centroExplosion;
        tiempoFlashImpacto = 0.25f;
    }

    objetivoActual = nullptr;
    velocidadCentro = {0.0f, 0.0f, 0.0f};

    CambiarFase(FaseEnjambre::INACTIVO);
}

void AtaqueEnjambre::ActualizarResiduos(float dt) {
    for (Particula& particula : particulas) {
        if (
            !particula.activa ||
            !particula.estetica
        ) {
            continue;
        }

        ActualizarEcos(particula, dt);

        particula.posicion = Vector3Add(
            particula.posicion,
            Vector3Scale(
                particula.velocidad,
                dt
            )
        );

        particula.velocidad = Vector3Scale(
            particula.velocidad,
            powf(0.30f, dt)
        );

        particula.vida -= dt;

        if (particula.vida <= 0.0f) {
            particula.activa = false;
        }
    }
}

void AtaqueEnjambre::ActualizarHipertrazas(float dt) {
    for (Hipertraza& traza : hipertrazas) {
        traza.vida -= dt;

        traza.pulso = fmodf(
            traza.pulso +
                traza.velocidadPulso * dt,
            1.0f
        );
    }

    hipertrazas.erase(
        std::remove_if(
            hipertrazas.begin(),
            hipertrazas.end(),
            [](const Hipertraza& traza) {
                return traza.vida <= 0.0f;
            }
        ),
        hipertrazas.end()
    );

    if (fase == FaseEnjambre::INACTIVO) {
        return;
    }

    float intervalo = 0.040f;

    switch (fase) {
        case FaseEnjambre::DISPERSION:
            intervalo = 0.032f;
            break;

        case FaseEnjambre::SUSPENSION:
            intervalo = 0.015f;
            break;

        case FaseEnjambre::REAGRUPACION:
            intervalo = 0.027f;
            break;

        case FaseEnjambre::ATAQUE:
            intervalo = 0.042f;
            break;

        case FaseEnjambre::COLAPSO:
            intervalo = 0.010f;
            break;

        case FaseEnjambre::INACTIVO:
            return;
    }

    acumuladorHipertrazas += dt;

    while (
        acumuladorHipertrazas >= intervalo &&
        hipertrazas.size() <
            static_cast<size_t>(MAX_HIPERTRAZAS)
    ) {
        acumuladorHipertrazas -= intervalo;
        CrearHipertraza();
    }
}

void AtaqueEnjambre::CrearHipertraza() {
    std::vector<int> candidatas;
    candidatas.reserve(particulas.size());

    for (
        int i = 0;
        i < static_cast<int>(particulas.size());
        i++
    ) {
        if (
            particulas[i].activa &&
            !particulas[i].estetica
        ) {
            candidatas.push_back(i);
        }
    }

    if (candidatas.size() < 2) {
        return;
    }

    const int indiceA = candidatas[
        rand() % candidatas.size()
    ];

    int indiceB = indiceA;
    float distanciaMayor = -1.0f;

    /*
     * Busca una partícula razonablemente distante.
     * No crea conexiones permanentes ni una malla completa.
     */
    for (int intento = 0; intento < 4; intento++) {
        const int candidata = candidatas[
            rand() % candidatas.size()
        ];

        if (candidata == indiceA) {
            continue;
        }

        const float distancia = Vector3Distance(
            particulas[indiceA].posicion,
            particulas[candidata].posicion
        );

        if (distancia > distanciaMayor) {
            distanciaMayor = distancia;
            indiceB = candidata;
        }
    }

    if (indiceB == indiceA) {
        return;
    }

    const Vector3 posicionA =
        particulas[indiceA].posicion;

    const Vector3 posicionB =
        particulas[indiceB].posicion;

    const Vector3 eje = Vector3Subtract(
        posicionB,
        posicionA
    );

    Hipertraza traza;

    /*
     * La línea se extiende más allá de ambas partículas.
     * Esto evita que parezca una conexión física.
     */
    traza.inicio = Vector3Subtract(
        posicionA,
        Vector3Scale(
            eje,
            Aleatorio(0.12f, 0.42f)
        )
    );

    traza.fin = Vector3Add(
        posicionB,
        Vector3Scale(
            eje,
            Aleatorio(0.12f, 0.42f)
        )
    );

    traza.desfaseCentral = Vector3Scale(
        VectorAleatorioUnitario(),
        Aleatorio(0.05f, 0.24f)
    );

    if (rand() % 3 == 0) {
        traza.color = {
            255,
            70,
            255,
            190
        };
    } else {
        traza.color = {
            0,
            240,
            255,
            190
        };
    }

    traza.vidaMaxima = Aleatorio(
        0.035f,
        0.105f
    );

    traza.vida = traza.vidaMaxima;
    traza.pulso = Aleatorio(0.0f, 1.0f);
    traza.velocidadPulso = Aleatorio(7.0f, 15.0f);

    traza.corteA = Aleatorio(0.20f, 0.40f);
    traza.corteB = Aleatorio(0.60f, 0.80f);

    traza.semilla = contadorSemillas++;

    hipertrazas.push_back(traza);
}

void AtaqueEnjambre::DibujarHipertrazas() const {
    for (const Hipertraza& traza : hipertrazas) {
        const float ruidoVisual = sinf(
            tiempoTotal * 95.0f +
            static_cast<float>(traza.semilla) * 0.021f
        );

        /*
         * Algunos frames no dibujan la traza.
         * Es una intersección momentánea, no una cuerda.
         */
        if (ruidoVisual < -0.42f) {
            continue;
        }

        const float desvanecimiento =
            traza.vida / traza.vidaMaxima;

        const float parpadeo =
            0.55f + 0.45f * Saturar(ruidoVisual);

        const Color colorTraza = AplicarAlpha(
            traza.color,
            desvanecimiento * parpadeo
        );

        const Vector3 segmentoA1 = traza.inicio;
        const Vector3 segmentoA2 = Vector3Lerp(
            traza.inicio,
            traza.fin,
            traza.corteA
        );

        const Vector3 segmentoB1 = Vector3Lerp(
            traza.inicio,
            traza.fin,
            traza.corteB
        );

        const Vector3 segmentoB2 = traza.fin;

        DrawLine3D(
            segmentoA1,
            segmentoA2,
            colorTraza
        );

        DrawLine3D(
            segmentoB1,
            segmentoB2,
            colorTraza
        );

        /*
         * Fragmento central desplazado:
         * parece pertenecer a otra proyección espacial.
         */
        const float centroInicio =
            traza.corteA + 0.07f;

        const float centroFinal =
            traza.corteB - 0.07f;

        if (centroFinal > centroInicio) {
            const Vector3 fragmentoInicio =
                SumarDesfase(
                    Vector3Lerp(
                        traza.inicio,
                        traza.fin,
                        centroInicio
                    ),
                    traza.desfaseCentral
                );

            const Vector3 fragmentoFinal =
                SumarDesfase(
                    Vector3Lerp(
                        traza.inicio,
                        traza.fin,
                        centroFinal
                    ),
                    traza.desfaseCentral
                );

            DrawLine3D(
                fragmentoInicio,
                fragmentoFinal,
                AplicarAlpha(
                    colorTraza,
                    0.65f
                )
            );
        }

        /*
         * Pulso blanco que viaja sobre la traza.
         */
        const float pulsoInicio =
            traza.pulso;

        const float pulsoFinal = std::min(
            1.0f,
            pulsoInicio + 0.075f
        );

        const Vector3 posicionPulsoInicio =
            Vector3Lerp(
                traza.inicio,
                traza.fin,
                pulsoInicio
            );

        const Vector3 posicionPulsoFinal =
            Vector3Lerp(
                traza.inicio,
                traza.fin,
                pulsoFinal
            );

        const Color blancoPulso = AplicarAlpha(
            WHITE,
            desvanecimiento
        );

        DrawLine3D(
            posicionPulsoInicio,
            posicionPulsoFinal,
            blancoPulso
        );

        DrawSphere(
            posicionPulsoFinal,
            0.065f,
            blancoPulso
        );
    }
}

void AtaqueEnjambre::Dibujar() const {
    if (
        particulas.empty() &&
        hipertrazas.empty() &&
        tiempoFlashImpacto <= 0.0f
    ) {
        return;
    }

    BeginBlendMode(BLEND_ADDITIVE);

    /*
     * Shockwave digital del disparo.
     */
    if (
        fase == FaseEnjambre::DISPERSION &&
        tiempoFase <= 0.24f
    ) {
        const float progreso = Saturar(
            tiempoFase / 0.24f
        );

        const float radio =
            0.8f + progreso * 10.0f;

        const float alpha =
            1.0f - progreso;

        DrawCircle3D(
            origenDisparo,
            radio,
            {1.0f, 0.0f, 0.0f},
            tiempoTotal * 420.0f,
            AplicarAlpha(
                {0, 240, 255, 220},
                alpha
            )
        );

        DrawCircle3D(
            origenDisparo,
            radio * 0.72f,
            {0.0f, 1.0f, 0.0f},
            -tiempoTotal * 510.0f,
            AplicarAlpha(
                {255, 50, 255, 190},
                alpha
            )
        );

        DrawSphere(
            origenDisparo,
            0.35f + (1.0f - progreso) * 0.65f,
            AplicarAlpha(
                WHITE,
                alpha
            )
        );
    }

    DibujarHipertrazas();

    for (const Particula& particula : particulas) {
        if (!particula.activa) {
            continue;
        }

        float factorVida = 1.0f;

        if (
            particula.estetica &&
            particula.vidaMaxima > 0.0f
        ) {
            factorVida = Saturar(
                particula.vida /
                particula.vidaMaxima
            );
        }

        Color colorPrincipal = {
            0,
            245,
            255,
            255
        };

        Color colorEco = {
            255,
            55,
            255,
            150
        };

        float escala = 0.30f;

        if (particula.estetica) {
            colorPrincipal = {
                255,
                70,
                255,
                220
            };

            colorEco = {
                0,
                220,
                255,
                110
            };

            escala = 0.22f;
        } else {
            switch (fase) {
                case FaseEnjambre::DISPERSION:
                    if (particula.semilla % 3 == 0) {
                        colorPrincipal = {
                            255,
                            60,
                            255,
                            255
                        };
                    }

                    escala = 0.29f;
                    break;

                case FaseEnjambre::SUSPENSION:
                    colorPrincipal = {
                        180,
                        255,
                        255,
                        255
                    };

                    escala = 0.36f;
                    break;

                case FaseEnjambre::REAGRUPACION:
                    escala = 0.33f;
                    break;

                case FaseEnjambre::ATAQUE:
                    escala = 0.28f;
                    break;

                case FaseEnjambre::COLAPSO:
                    colorPrincipal = WHITE;
                    escala = 0.38f;
                    break;

                case FaseEnjambre::INACTIVO:
                    break;
            }
        }

        /*
         * Dos ecos temporales: magenta y cyan.
         */
        DrawLine3D(
            particula.ecoLejano,
            particula.ecoCercano,
            AplicarAlpha(
                colorEco,
                factorVida * 0.55f
            )
        );

        DrawLine3D(
            particula.ecoCercano,
            particula.posicion,
            AplicarAlpha(
                colorPrincipal,
                factorVida * 0.75f
            )
        );

        /*
         * Halo y núcleo de cada partícula.
         */
        DrawSphere(
            particula.posicion,
            escala * 1.85f,
            AplicarAlpha(
                colorPrincipal,
                factorVida * 0.20f
            )
        );

        DrawSphere(
            particula.posicion,
            escala,
            AplicarAlpha(
                colorPrincipal,
                factorVida
            )
        );

        DrawSphere(
            particula.posicion,
            escala * 0.30f,
            AplicarAlpha(
                WHITE,
                factorVida
            )
        );
    }

    /*
     * Núcleo común visible durante la formación y persecución.
     */
    if (
        fase == FaseEnjambre::REAGRUPACION ||
        fase == FaseEnjambre::ATAQUE ||
        fase == FaseEnjambre::COLAPSO
    ) {
        float radioNucleo = 0.55f;

        if (fase == FaseEnjambre::COLAPSO) {
            const float progreso = Saturar(
                tiempoFase / DURACION_COLAPSO
            );

            radioNucleo =
                0.70f - progreso * 0.58f;
        }

        DrawSphere(
            centroEnjambre,
            radioNucleo * 2.2f,
            {0, 220, 255, 45}
        );

        DrawSphere(
            centroEnjambre,
            radioNucleo,
            {0, 245, 255, 220}
        );

        DrawSphere(
            centroEnjambre,
            radioNucleo * 0.32f,
            WHITE
        );

        /*
         * Los anillos solamente aparecen en algunos frames.
         * No forman una jaula persistente.
         */
        const float intermitencia =
            sinf(tiempoTotal * 38.0f);

        if (intermitencia > 0.20f) {
            DrawCircle3D(
                centroEnjambre,
                radioNucleo * 2.8f,
                {1.0f, 0.0f, 0.0f},
                tiempoTotal * 300.0f,
                {0, 240, 255, 105}
            );

            DrawCircle3D(
                centroEnjambre,
                radioNucleo * 2.2f,
                {0.0f, 1.0f, 0.0f},
                -tiempoTotal * 420.0f,
                {255, 60, 255, 85}
            );
        }
    }

    /*
     * Flash posterior al impacto.
     */
    if (tiempoFlashImpacto > 0.0f) {
        const float progreso = Saturar(
            1.0f -
            tiempoFlashImpacto / 0.25f
        );

        const float desvanecimiento =
            1.0f - progreso;

        const float radio =
            0.4f + progreso * 7.0f;

        DrawCircle3D(
            puntoImpacto,
            radio,
            {1.0f, 0.0f, 0.0f},
            tiempoTotal * 600.0f,
            AplicarAlpha(
                {0, 255, 255, 240},
                desvanecimiento
            )
        );

        DrawCircle3D(
            puntoImpacto,
            radio * 0.72f,
            {0.0f, 1.0f, 0.0f},
            -tiempoTotal * 720.0f,
            AplicarAlpha(
                {255, 50, 255, 220},
                desvanecimiento
            )
        );

        DrawSphere(
            puntoImpacto,
            0.25f + desvanecimiento * 0.85f,
            AplicarAlpha(
                WHITE,
                desvanecimiento
            )
        );
    }

    EndBlendMode();

    /*
     * Segunda pasada: núcleos sólidos.
     *
     * Los halos, trazas y ecos anteriores usan mezcla aditiva.
     * Esta pasada devuelve volumen y legibilidad a las partículas.
     */
    BeginBlendMode(BLEND_ALPHA);

    for (const Particula& particula : particulas) {
        if (!particula.activa) {
            continue;
        }

        float factorVida = 1.0f;

        if (
            particula.estetica &&
            particula.vidaMaxima > 0.0f
        ) {
            factorVida = Saturar(
                particula.vida /
                particula.vidaMaxima
            );
        }

        Color colorSolido;
        float radioSolido;

        if (particula.estetica) {
            colorSolido = {
                255,
                70,
                255,
                static_cast<unsigned char>(
                    230.0f * factorVida
                )
            };

            radioSolido = 0.18f;
        } else {
            const bool particulaMagenta =
                particula.semilla % 4 == 0;

            colorSolido = particulaMagenta
                ? Color{255, 45, 255, 255}
                : Color{0, 235, 255, 255};

            switch (fase) {
                case FaseEnjambre::DISPERSION:
                    radioSolido = 0.34f;
                    break;

                case FaseEnjambre::SUSPENSION:
                    radioSolido = 0.43f;
                    break;

                case FaseEnjambre::REAGRUPACION:
                    radioSolido = 0.39f;
                    break;

                case FaseEnjambre::ATAQUE:
                    radioSolido = 0.34f;
                    break;

                case FaseEnjambre::COLAPSO:
                    radioSolido = 0.46f;
                    colorSolido = WHITE;
                    break;

                case FaseEnjambre::INACTIVO:
                    radioSolido = 0.25f;
                    break;
            }
        }

        DrawSphere(
            particula.posicion,
            radioSolido,
            colorSolido
        );

        /*
         * Punto blanco central para mejorar la lectura
         * incluso sobre escenarios cyan o magenta.
         */
        DrawSphere(
            particula.posicion,
            radioSolido * 0.32f,
            AplicarAlpha(
                WHITE,
                factorVida
            )
        );
    }

    EndBlendMode();
}
