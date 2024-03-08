#version 430

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

uniform mat4 current;
uniform mat4 previous;

// TODO: switch to using a uniform buffer to reduce the number of opengl calls
// Define the UBO and its binding point
//layout(std140, binding = 0) uniform Transformations {
//    mat4 current;
//    mat4 previous;
//} transform;

out vec2 texCoord;
out vec2 velocity;

void main() {
    vec4 curPos = current * vec4(vertexPosition, 1.0);
    vec4 prevPos = previous * vec4(vertexPosition, 1.0);

    velocity = curPos.xy / curPos.w - prevPos.xy / prevPos.w;
    gl_Position = curPos;
    texCoord = vertexTexCoord;
}

