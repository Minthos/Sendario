#version 430

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexTexCoord;

uniform mat4 current;
uniform mat4 previous;

out vec2 texCoord;
out vec2 velocity;
out float z;
out float elevation;

void main() {
    vec4 curPos = current * vec4(vertexPosition, 1.0);
    vec4 prevPos = previous * vec4(vertexPosition, 1.0);

    velocity = curPos.xy / curPos.w - prevPos.xy / prevPos.w;
    gl_Position = curPos;
    texCoord = vertexTexCoord.xy;
    z = gl_Position.z;
    elevation = vertexTexCoord.z;
}

