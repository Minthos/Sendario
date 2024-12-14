#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;
flat in int typeid;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out ivec4 velocityOut;

void main() {
    if(typeid == 0) {
        fragColor = texture(tex, texCoord);
    } else {
        fragColor = vec4(1.0);
    }
    float depth = log2(z + 2.0) * 0.03;
    velocityOut = ivec4(velocity * 4096.0, z, 2);
    gl_FragDepth = depth;
}

