const char* vertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
out vec3 normal;
out vec3 eye_direction;
out vec3 light_direction;
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightDir;
void main() {
    gl_Position =  MVP * vec4(vertexPosition,1);
    normal = mat3(V) * vertexNormal;
    eye_direction = vec3(5, 5, 5) - (V * vec4(-vertexPosition, 1)).xyz;
    light_direction = (mat3(V) * lightDir) + eye_direction;
}
)glsl";