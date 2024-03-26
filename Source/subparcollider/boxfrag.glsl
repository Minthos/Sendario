#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 velocityOut;

void main() {
    //fragColor = vec4(1.0);
    fragColor = texture(tex, texCoord);
    velocityOut = velocity;
    gl_FragDepth = log2(z + 2.0) * 0.03;
}

