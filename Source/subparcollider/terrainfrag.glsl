#version 430

in vec3 texCoord;
in vec2 velocity;
in float z;
flat in int typeid;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out ivec4 velocityOut;

void main() {
    const float pi = 3.1415926535897932384626433832795;
    float inclination = texCoord.x / pi;
    float insolation = texCoord.y + 0.5;
    float elevation = texCoord.z;
    switch(typeid){
    case 1: // terrain
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
        break;
    case 2: // treetrunk
        fragColor = mix(vec4(0.4, 0.3, 0.2, 1.0), vec4(0, 0, 0, 1), 1.0 - texCoord.z);
        break;
    case 3: // leaf
        fragColor = mix(vec4(vec3(0.15, 0.4, 0.15), 1.0), vec4(0, 0, 0, 1), 1.0 - texCoord.z);
        break;
    }
    float depth = log2(z + 2.0) * 0.03;
    velocityOut = ivec4(velocity * 4096.0, z, 1);
    gl_FragDepth = depth;
}

