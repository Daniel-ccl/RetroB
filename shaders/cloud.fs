#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float tiempo;
uniform vec2 uvOffset;
uniform vec3 colorNube;
uniform float fasePuff;
uniform float brilloPuff;

float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float ruido(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0)), u.x),
        u.y
    );
}

float formaElipse(vec2 p, vec2 centro, vec2 escala) {
    float distancia = length((p - centro) / escala);
    return 1.0 - smoothstep(0.62, 1.0, distancia);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 deriva = uvOffset + vec2(tiempo * 0.012, tiempo * 0.008) + fasePuff;
    float detalle = ruido(uv * 3.5 + deriva) * 0.68;
    detalle += ruido(uv * 7.0 + deriva * 1.7) * 0.32;

    float forma = formaElipse(uv, vec2(0.50, 0.54), vec2(0.50, 0.34));
    forma = max(forma, formaElipse(uv, vec2(0.30, 0.55), vec2(0.27, 0.25)));
    forma = max(forma, formaElipse(uv, vec2(0.68, 0.57), vec2(0.30, 0.27)));
    float densidad = forma * (0.72 + detalle * 0.42);
    float alpha = smoothstep(0.10, 0.48, densidad) * 0.88 * brilloPuff;

    if (alpha < 0.015) discard;

    float luzSuperior = 0.72 + uv.y * 0.22;
    float volumen = 0.82 + detalle * 0.22;
    finalColor = vec4(colorNube * luzSuperior * volumen, alpha);
}
