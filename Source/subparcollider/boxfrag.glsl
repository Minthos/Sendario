#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 velocityOut;

void main() {
    //fragColor = vec4(1.0);
    fragColor = texture(tex, texCoord);
    float depth = log2(z + 2.0) * 0.03;
    velocityOut = vec4(velocity, depth, 0.99);
    gl_FragDepth = depth;
}

