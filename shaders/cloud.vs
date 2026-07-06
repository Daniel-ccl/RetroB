// shaders/cloud.vs
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform vec3 camRight;    // camera right vector, sent from C++
uniform vec3 camUp;       // camera up vector

out vec2 fragTexCoord;

void main() {
    // billboard: rebuild position using camera axes
    // vertexPosition.xy are the quad corners (-0.5 to 0.5)
    vec3 worldPos = vec3(matModel[3]);  // extract translation only
    worldPos += camRight * vertexPosition.x;
    worldPos += camUp    * vertexPosition.y;

    fragTexCoord = vertexTexCoord;
    gl_Position  = mvp * vec4(worldPos, 1.0);
}
