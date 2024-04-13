#version 430

layout(location = 0) in vec4 vertexPosition;
//layout(location = 1) in int typeId;
layout(location = 1) in vec3 vertexTexCoord;

uniform mat4 current;
uniform mat4 previous;

out vec3 texCoord;
out vec2 velocity;
out float z;
flat out int typeid;

void main() {
    vec4 curPos = current * vec4(vertexPosition.xyz, 1.0);
    vec4 prevPos = previous * vec4(vertexPosition.xyz, 1.0);
    typeid = floatBitsToInt(vertexPosition.w);

    //velocity = curPos.xy / curPos.w - prevPos.xy / prevPos.w;
    velocity = (curPos.xy - prevPos.xy) / max(1.0, max(curPos.w, prevPos.w));
    gl_Position = curPos;
    texCoord = vertexTexCoord;
    z = gl_Position.z;
//    typeid = typeId;
}

