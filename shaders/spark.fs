#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec3 colorBase;
uniform float progreso;
uniform float pixelSize;
uniform float modo;

void main() {
    vec2 uvPix = floor(fragTexCoord * pixelSize) / pixelSize;
    vec2 centro = uvPix - 0.5;
    float dist = length(centro) * 2.0;

    float radio = modo > 0.5 ? mix(0.5, 1.3, progreso) : mix(1.0, 0.15, progreso);
    float mascara = 1.0 - smoothstep(radio * 0.6, radio, dist);
    if (mascara <= 0.01) discard;

    float escalones = 5.0;
    float intensidad = ceil(mascara * escalones) / escalones;

    float brillo = modo > 0.5 ? mix(2.2, 0.4, progreso) : mix(2.0, 0.5, progreso);
    vec3 color = colorBase * intensidad * brillo;
    float alpha = intensidad * (1.0 - progreso);

    finalColor = vec4(color, alpha);
}
