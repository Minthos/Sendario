#version 430

in vec3 texCoord;
in vec2 velocity;
in float z;
flat in int typeid;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out ivec4 velocityOut;

void main() {
    fragColor = vec4(texCoord, 1.0);
    float depth = log2(z + 2.0) * 0.03;
    velocityOut = ivec4(velocity * 4096.0, z, 1);
    gl_FragDepth = depth;
}

