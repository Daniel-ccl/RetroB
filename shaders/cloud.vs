#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform vec3 camRight;
uniform vec3 camUp;

out vec2 fragTexCoord;

void main() {
    vec2 esquina = vec2(vertexPosition.x, vertexPosition.z);
    vec3 posicionBillboard = camRight * esquina.x + camUp * esquina.y;
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp * vec4(posicionBillboard, 1.0);
}
