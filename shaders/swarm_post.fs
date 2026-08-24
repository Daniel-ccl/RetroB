#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;

uniform vec2 resolucion;
uniform vec2 centroEnjambre;
uniform vec2 direccionEnjambre;

uniform float tiempo;
uniform float fase;
uniform float progreso;
uniform float intensidad;

float Hash21(vec2 posicion) {
    posicion = fract(
        posicion * vec2(123.34, 456.21)
    );

    posicion += dot(
        posicion,
        posicion + 45.32
    );

    return fract(posicion.x * posicion.y);
}

vec2 Hash22(vec2 posicion) {
    float valorA = Hash21(posicion);
    float valorB = Hash21(
        posicion + vec2(17.13, 91.71)
    );

    return vec2(valorA, valorB);
}

/*
 * fragTexCoord está invertido verticalmente porque
 * el RenderTexture de Raylib se dibuja con altura negativa.
 *
 * Esta función convierte vectores en coordenadas de pantalla
 * a desplazamientos válidos para el muestreo de la textura.
 */
vec2 PantallaATextura(vec2 vectorPantalla) {
    return vec2(
        vectorPantalla.x,
        -vectorPantalla.y
    );
}

void main() {
    vec4 colorOriginal = texture(
        texture0,
        fragTexCoord
    );

    /*
     * Camino rápido: una sola lectura cuando el ataque
     * no está activo.
     */
    if (intensidad <= 0.001) {
        finalColor = colorOriginal;
        return;
    }

    vec2 uvPantalla = vec2(
        fragTexCoord.x,
        1.0 - fragTexCoord.y
    );

    float aspecto =
        resolucion.x / resolucion.y;

    vec2 deltaPantalla =
        uvPantalla - centroEnjambre;

    vec2 deltaCorregido = vec2(
        deltaPantalla.x * aspecto,
        deltaPantalla.y
    );

    float distancia = length(deltaCorregido);

    vec2 direccionRadial = vec2(0.0);

    if (length(deltaPantalla) > 0.00001) {
        direccionRadial = normalize(
            deltaPantalla
        );
    }

    vec2 desplazamientoPantalla = vec2(0.0);
    vec2 direccionCromatica = direccionRadial;

    float separacionRGB = 0.0;
    float mezclaPixelada = 0.0;
    float tamanoBloque = 1.0;

    float mascaraScanlines = 0.0;
    float brilloCyan = 0.0;
    float brilloMagenta = 0.0;
    float flashBlanco = 0.0;

    /*
     * FASE 1: DISPERSIÓN
     *
     * Bloques desplazados, pixelación y separación RGB.
     */
    if (fase < 1.5) {
        float radio = mix(
            0.055,
            0.31,
            progreso
        );

        float mascara = 1.0 - smoothstep(
            radio * 0.60,
            radio,
            distancia
        );

        tamanoBloque = mix(
            15.0,
            7.0,
            progreso
        );

        vec2 celda = floor(
            uvPantalla *
            resolucion /
            tamanoBloque
        );

        float tiempoEscalonado =
            floor(tiempo * 26.0);

        vec2 ruidoBloque = Hash22(
            celda + tiempoEscalonado
        );

        float bloqueActivo = step(
            0.48,
            Hash21(celda + tiempoEscalonado)
        );

        vec2 saltoBloque = (
            ruidoBloque - 0.5
        ) * vec2(
            18.0 / resolucion.x,
            12.0 / resolucion.y
        );

        desplazamientoPantalla +=
            saltoBloque *
            mascara *
            bloqueActivo;

        separacionRGB =
            mascara *
            mix(0.007, 0.002, progreso);

        mezclaPixelada =
            mascara * 0.72;

        brilloCyan =
            mascara *
            step(0.72, ruidoBloque.x) *
            0.20;

        brilloMagenta =
            mascara *
            step(0.76, ruidoBloque.y) *
            0.16;
    }

    /*
     * FASE 2: SUSPENSIÓN
     *
     * Las líneas cambian en pasos discretos para aparentar
     * que distintas regiones de la imagen quedan congeladas.
     */
    else if (fase < 2.5) {
        float radio = 0.29;

        float mascara = 1.0 - smoothstep(
            radio * 0.72,
            radio,
            distancia
        );

        float fila = floor(
            uvPantalla.y *
            resolucion.y /
            3.0
        );

        float tiempoCongelado =
            floor(tiempo * 11.0);

        float ruidoFila = Hash21(
            vec2(fila, tiempoCongelado)
        );

        float filaDesplazada = step(
            0.70,
            ruidoFila
        );

        desplazamientoPantalla.x +=
            (ruidoFila - 0.5) *
            (12.0 / resolucion.x) *
            mascara *
            filaDesplazada;

        float patronLinea = fract(
            uvPantalla.y *
            resolucion.y /
            3.0 +
            ruidoFila
        );

        mascaraScanlines =
            step(0.70, patronLinea) *
            mascara;

        separacionRGB =
            mascara *
            filaDesplazada *
            0.0045;

        direccionCromatica = vec2(1.0, 0.0);

        brilloCyan =
            mascara *
            step(0.82, ruidoFila) *
            0.28;

        mezclaPixelada =
            mascara *
            filaDesplazada *
            0.28;

        tamanoBloque = 5.0;
    }

    /*
     * FASE 3: REAGRUPACIÓN
     *
     * La imagen es atraída hacia el centro compartido.
     */
    else if (fase < 3.5) {
        float radio = mix(
            0.32,
            0.20,
            progreso
        );

        float mascara = 1.0 - smoothstep(
            radio * 0.72,
            radio,
            distancia
        );

        float pulso = sin(
            distancia * 95.0 -
            tiempo * 13.0
        );

        /*
         * Muestrear desde una zona más exterior hace que
         * los píxeles parezcan moverse hacia el centro.
         */
        desplazamientoPantalla +=
            direccionRadial *
            mascara *
            (
                0.018 +
                pulso * 0.005
            ) *
            progreso;

        float anillo = 1.0 - smoothstep(
            0.008,
            0.026,
            abs(
                distancia -
                radio * (0.78 - progreso * 0.35)
            )
        );

        separacionRGB =
            mascara * 0.0025 +
            anillo * 0.006;

        brilloCyan =
            mascara * 0.12 +
            anillo * 0.26;

        brilloMagenta =
            anillo * 0.17;
    }

    /*
     * FASE 4: ATAQUE
     *
     * Se crea una rasgadura estrecha detrás del blob.
     */
    else if (fase < 4.5) {
        vec2 direccionMovimiento =
            direccionEnjambre;

        if (length(direccionMovimiento) < 0.001) {
            direccionMovimiento = vec2(1.0, 0.0);
        } else {
            direccionMovimiento = normalize(
                direccionMovimiento
            );
        }

        vec2 direccionCorregida = normalize(
            vec2(
                direccionMovimiento.x * aspecto,
                direccionMovimiento.y
            )
        );

        float longitudinal = dot(
            deltaCorregido,
            direccionCorregida
        );

        float transversal = abs(
            deltaCorregido.x *
                direccionCorregida.y -
            deltaCorregido.y *
                direccionCorregida.x
        );

        float ancho = 0.012 +
            sin(tiempo * 17.0) * 0.0025;

        float mascaraAncho = exp(
            -pow(transversal / ancho, 2.0)
        );

        /*
         * La rasgadura se extiende principalmente
         * detrás del movimiento.
         */
        float mascaraLongitud =
            smoothstep(
                -0.34,
                -0.25,
                longitudinal
            ) *
            (
                1.0 -
                smoothstep(
                    0.025,
                    0.075,
                    longitudinal
                )
            );

        float mascaraRasgadura =
            mascaraAncho *
            mascaraLongitud;

        vec2 perpendicular = vec2(
            -direccionMovimiento.y,
            direccionMovimiento.x
        );

        float segmento = floor(
            longitudinal *
            resolucion.y /
            8.0
        );

        float ruidoSegmento = Hash21(
            vec2(
                segmento,
                floor(tiempo * 22.0)
            )
        );

        desplazamientoPantalla +=
            perpendicular *
            (ruidoSegmento - 0.5) *
            0.025 *
            mascaraRasgadura;

        float mascaraCentro = 1.0 - smoothstep(
            0.025,
            0.11,
            distancia
        );

        desplazamientoPantalla +=
            direccionRadial *
            sin(
                distancia * 130.0 -
                tiempo * 20.0
            ) *
            0.006 *
            mascaraCentro;

        separacionRGB =
            mascaraRasgadura * 0.010 +
            mascaraCentro * 0.004;

        direccionCromatica = perpendicular;

        mezclaPixelada =
            mascaraRasgadura *
            step(0.53, ruidoSegmento) *
            0.52;

        tamanoBloque = 6.0;

        brilloCyan =
            mascaraRasgadura * 0.22;

        brilloMagenta =
            mascaraRasgadura *
            step(0.50, ruidoSegmento) *
            0.20;
    }

    /*
     * FASE 5: IMPACTO
     *
     * Anillo radial, fragmentos lanzados hacia afuera
     * y un flash de aproximadamente uno o dos frames.
     */
    else {
        float radioAnillo = mix(
            0.015,
            0.34,
            progreso
        );

        float mascaraAnillo = 1.0 - smoothstep(
            0.010,
            0.032,
            abs(distancia - radioAnillo)
        );

        float mascaraInterior = 1.0 - smoothstep(
            radioAnillo * 0.25,
            radioAnillo + 0.035,
            distancia
        );

        vec2 celda = floor(
            uvPantalla *
            resolucion /
            9.0
        );

        vec2 ruidoBloque = Hash22(
            celda +
            floor(tiempo * 30.0)
        );

        float fragmentoActivo = step(
            0.50,
            ruidoBloque.x
        );

        /*
         * El muestreo se mueve hacia el centro para que
         * la imagen resultante parezca salir disparada.
         */
        desplazamientoPantalla -=
            direccionRadial *
            mascaraAnillo *
            (
                0.018 +
                ruidoBloque.y * 0.030
            ) *
            (1.0 - progreso);

        desplazamientoPantalla +=
            (ruidoBloque - 0.5) *
            0.018 *
            mascaraInterior *
            fragmentoActivo *
            (1.0 - progreso);

        separacionRGB =
            mascaraAnillo *
            mix(0.014, 0.002, progreso);

        mezclaPixelada =
            mascaraInterior *
            fragmentoActivo *
            (1.0 - progreso) *
            0.80;

        tamanoBloque = mix(
            13.0,
            5.0,
            progreso
        );

        brilloCyan =
            mascaraAnillo * 0.35;

        brilloMagenta =
            mascaraAnillo * 0.30;

        /*
         * progress < 0.09 dura aproximadamente 22 ms
         * dentro del flash de 250 ms.
         */
        flashBlanco =
            (
                1.0 -
                smoothstep(
                    0.018,
                    0.090,
                    progreso
                )
            ) *
            mascaraInterior;
    }

    desplazamientoPantalla *= intensidad;
    separacionRGB *= intensidad;
    mezclaPixelada *= intensidad;

    vec2 uvMuestreo = fragTexCoord +
        PantallaATextura(
            desplazamientoPantalla
        );

    uvMuestreo = clamp(
        uvMuestreo,
        vec2(0.001),
        vec2(0.999)
    );

    /*
     * Pixelación localizada.
     */
    if (mezclaPixelada > 0.001) {
        vec2 uvBloque = (
            floor(
                uvMuestreo *
                resolucion /
                tamanoBloque
            ) *
            tamanoBloque +
            tamanoBloque * 0.5
        ) / resolucion;

        uvMuestreo = mix(
            uvMuestreo,
            uvBloque,
            mezclaPixelada
        );
    }

    vec2 offsetRGB = PantallaATextura(
        direccionCromatica *
        separacionRGB
    );

    /*
     * Tres lecturas solamente durante el ataque:
     * rojo, base verde y azul desplazado.
     */
    float canalRojo = texture(
        texture0,
        clamp(
            uvMuestreo + offsetRGB,
            vec2(0.001),
            vec2(0.999)
        )
    ).r;

    vec4 muestraCentral = texture(
        texture0,
        uvMuestreo
    );

    float canalAzul = texture(
        texture0,
        clamp(
            uvMuestreo - offsetRGB,
            vec2(0.001),
            vec2(0.999)
        )
    ).b;

    vec3 color = vec3(
        canalRojo,
        muestraCentral.g,
        canalAzul
    );

    color *= 1.0 -
        mascaraScanlines *
        intensidad *
        0.62;

    color += vec3(
        0.0,
        0.45,
        0.70
    ) * brilloCyan * intensidad;

    color += vec3(
        0.70,
        0.0,
        0.55
    ) * brilloMagenta * intensidad;

    color = mix(
        color,
        vec3(1.0),
        flashBlanco * intensidad
    );

    finalColor = vec4(
        color,
        colorOriginal.a
    );
}
