#include "ui.h"

namespace {

Font fuenteInterfaz{};
bool fuentePersonalizadaCargada = false;

constexpr const char* CARACTERES_UI =
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "áéíóúÁÉÍÓÚñÑüÜ¿¡";

}

namespace UI {

bool Inicializar(const char* rutaFuente, int tamanoBase) {
    if (fuentePersonalizadaCargada) {
        Finalizar();
    }

    if (!FileExists(rutaFuente)) {
        return false;
    }

    int cantidadPuntosCodigo = 0;
    int* puntosCodigo = LoadCodepoints(
        CARACTERES_UI,
        &cantidadPuntosCodigo
    );

    Font fuenteNueva = LoadFontEx(
        rutaFuente,
        tamanoBase,
        puntosCodigo,
        cantidadPuntosCodigo
    );

    UnloadCodepoints(puntosCodigo);

    Font fuentePredeterminada = GetFontDefault();

    if (!IsFontValid(fuenteNueva) ||
        fuenteNueva.texture.id == fuentePredeterminada.texture.id) {
        fuenteInterfaz = {};
        fuentePersonalizadaCargada = false;
        return false;
    }

    fuenteInterfaz = fuenteNueva;
    fuentePersonalizadaCargada = true;

    SetTextureFilter(
        fuenteInterfaz.texture,
        TEXTURE_FILTER_BILINEAR
    );

    return true;
}

void Finalizar() {
    if (!fuentePersonalizadaCargada) {
        return;
    }

    UnloadFont(fuenteInterfaz);
    fuenteInterfaz = {};
    fuentePersonalizadaCargada = false;
}

Font ObtenerFuente() {
    if (fuentePersonalizadaCargada) {
        return fuenteInterfaz;
    }

    return GetFontDefault();
}

Vector2 MedirTexto(
    const char* texto,
    float tamano,
    float espaciado
) {
    return MeasureTextEx(
        ObtenerFuente(),
        texto,
        tamano,
        espaciado
    );
}

void DibujarTexto(
    const char* texto,
    Vector2 posicion,
    float tamano,
    Color color,
    float espaciado
) {
    DrawTextEx(
        ObtenerFuente(),
        texto,
        posicion,
        tamano,
        espaciado,
        color
    );
}

void DibujarTextoCentrado(
    const char* texto,
    Rectangle area,
    float tamano,
    Color color,
    float espaciado
) {
    Vector2 tamanoTexto = MedirTexto(
        texto,
        tamano,
        espaciado
    );

    Vector2 posicion{
        area.x + (area.width - tamanoTexto.x) * 0.5f,
        area.y + (area.height - tamanoTexto.y) * 0.5f
    };

    DibujarTexto(
        texto,
        posicion,
        tamano,
        color,
        espaciado
    );
}

}
