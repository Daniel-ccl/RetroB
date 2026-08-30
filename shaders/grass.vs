#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;
uniform float tiempo;

out vec4 fragColor;

void main() {
    vec3 posicion = vertexPosition;
    float anclaje = vertexTexCoord.y;
    float fase = vertexTexCoord.x * 6.2831853;
    float onda = sin(tiempo * 1.7 + fase + posicion.x * 0.045 + posicion.z * 0.035);
    float rafaga = sin(tiempo * 0.63 + posicion.x * 0.018 - posicion.z * 0.022);
    posicion.x += (onda * 0.16 + rafaga * 0.10) * anclaje;
    posicion.z += cos(tiempo * 1.35 + fase + posicion.z * 0.04) * 0.08 * anclaje;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(posicion, 1.0);
}
