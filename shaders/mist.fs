#version 330

in vec2 fragTexCoord;
in vec2 fragWorldXZ;
in vec4 fragColor;

uniform float tiempo;

out vec4 finalColor;

float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 34.5);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0)), u.x),
        u.y
    );
}

void main() {
    vec2 deriva = vec2(tiempo * 0.025, -tiempo * 0.018);
    float detalle = noise(fragWorldXZ * 0.055 + deriva);
    detalle = detalle * 0.68 + noise(fragWorldXZ * 0.11 - deriva * 0.7) * 0.32;
    vec2 bordeUv = min(fragTexCoord, 1.0 - fragTexCoord);
    float borde = smoothstep(0.0, 0.18, min(bordeUv.x, bordeUv.y));
    float densidad = 0.42 + smoothstep(0.22, 0.82, detalle) * 0.58;
    float pulso = 0.88 + sin(tiempo * 0.35 + fragWorldXZ.x * 0.03) * 0.12;
    float alpha = fragColor.a * borde * densidad * pulso;
    if (alpha < 0.01) discard;
    finalColor = vec4(fragColor.rgb * (0.96 + detalle * 0.04), alpha);
}
