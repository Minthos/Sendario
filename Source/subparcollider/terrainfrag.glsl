#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;
in float elevation;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 velocityOut;

void main() {
    const float pi = 3.1415926535897932384626433832795;
    float inclination = texCoord.x / pi;
    float insolation = texCoord.y + 0.5;
    vec3 grass = mix(vec3(0.15, 0.4, 0.15), vec3(1.0), min(1.0, max(0.0, (elevation - 2000.0) / 1000.0)));
    vec3 rock = mix(vec3(0.7, 0.5, 0.3), vec3(0.15), min(1.0, max(0.0, elevation / 2000.0)));
    vec3 color = mix(grass, rock, max(0.0, min(1.0, (-0.5 + 5 * inclination) - max(0.0, (elevation - 3000) / 2000.0) )));
    vec3 sand = mix(vec3(194/255.0, 178/255.0, 128/255.0), color, min(1.0, max(0.0, (elevation + (20.0 * inclination)) / 10.0)));
    if(elevation < 1.0){
        color = vec3(0.1, 0.2, 0.3);
        fragColor = vec4(color * (3.0 + insolation) * 0.25, 1.0);
    } else if(elevation < 100.0) {
        fragColor = vec4(sand * insolation, 1.0);
    } else {
        fragColor = vec4(color * insolation, 1.0);
    }
    velocityOut = vec4(velocity, 1.0, 1.0);
    gl_FragDepth = log2(z + 2.0) * 0.03;
}

