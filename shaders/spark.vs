#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform vec3 camRight;
uniform vec3 camUp;

out vec2 fragTexCoord;

void main() {
    vec3 worldPos = vec3(matModel[3]);
    worldPos += camRight * vertexPosition.x;
    worldPos += camUp    * vertexPosition.y;

    fragTexCoord = vertexTexCoord;
    gl_Position  = mvp * vec4(worldPos, 1.0);
}
