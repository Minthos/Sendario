#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;
in float elevation;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 velocityOut;

void main() {
    const float pi = 3.1415926535897932384626433832795;
    float inclination = texCoord.x / pi;
    float insolation = texCoord.y + 0.5;
    vec3 grass = mix(vec3(0.15, 0.4, 0.15), vec3(1.0), min(1.0, max(0.0, (elevation - 1000.0) / 1000.0)));
    vec3 rock = mix(vec3(0.7, 0.5, 0.3), vec3(0.3), min(1.0, max(0.0, (elevation + 2000.0) / 2000.0)));
    vec3 color = mix(grass, rock, max(0.0, min(1.0, -0.5 + 5 * inclination)));
    fragColor = vec4(color * insolation, 1.0);
    velocityOut = velocity;
    gl_FragDepth = log2(z + 2.0) * 0.03;
}

