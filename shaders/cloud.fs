// shaders/cloud.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float tiempo;      // time, sent every frame
uniform vec2  uvOffset;    // cloud world position mapped to UV drift
uniform vec3  colorNube;   // white-ish, tweakable per cloud
uniform float fasePuff;    // per-puff time offset, asi no laten todos en sincronia
uniform float brilloPuff;  // 0..1 — brillo base de este puff especifico (los traseros/chicos un poco mas tenues)

// --- simple hash noise, no textures needed ---
float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    // smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(hash(i + vec2(0,0)), hash(i + vec2(1,0)), u.x),
        mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), u.x),
        u.y
    );
}

float cloudShape(vec2 uv) {
    // churn lento del muestreo de ruido a lo largo del tiempo — cada puff con su
    // propia fase, para que la nube se sienta viva sin que todos los puffs se
    // muevan exactamente igual (eso se veria sincronizado/artificial).
    vec2 deriva = vec2(tiempo * 0.015, tiempo * 0.01) + fasePuff;

    // 3 octaves of noise for detail
    float n  = noise(uv * 3.0  + deriva) * 0.5;
          n += noise(uv * 6.0  + deriva * 1.4) * 0.3;
          n += noise(uv * 12.0 + deriva * 0.6) * 0.2;

    // soft circular mask so edges fade naturally
    float dist = length(uv - 0.5) * 2.0;
    float mask = 1.0 - smoothstep(0.4, 1.0, dist);

    return n * mask;
}

void main() {
    // uvOffset drives translation — shape stays, position moves
    vec2 uv = fragTexCoord + uvOffset;

    float density = cloudShape(uv);

    // threshold — below this it's transparent
    float alpha = smoothstep(0.35, 0.65, density);

    if (alpha < 0.01) discard;  // skip fully transparent pixels

    // luz "de arriba" falsa y barata: la mitad superior del quad (fragTexCoord.y alto)
    // se ve un poco mas brillante, como si el sol le pegara encima — sin luces reales.
    float luzArriba = 0.85 + fragTexCoord.y * 0.25;

    // slight brightness variation for fluffiness illusion
    float bright = (0.85 + density * 0.15) * luzArriba * brilloPuff;
    finalColor = vec4(colorNube * bright, alpha * 0.88 * brilloPuff);
}
