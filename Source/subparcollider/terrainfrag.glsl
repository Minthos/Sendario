#version 430

in vec2 texCoord;
in vec2 velocity;
in float z;

uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 velocityOut;


float remap( float minval, float maxval, float curval )
{
    return ( curval - minval ) / ( maxval - minval );
}

void main() {
    const float pi = 3.1415926535897932384626433832795;


// new shit

    float inclination = texCoord.x / pi;
    float insolation = texCoord.y * 0.5 + 0.5;

    vec3 color = mix(vec3(0.15, 0.4, 0.15), vec3(0.7, 0.5, 0.3), max(0.0, min(1.0, -0.5 + 5 * inclination)));
//    vec3 color = vec3(1.0);
    fragColor = vec4(color * insolation, 1.0);
  


  // old shit

    //fragColor = vec4(1.0);
    //fragColor = texture(tex, texCoord);

//    fragColor = vec4(texCoord * 0.5 + 0.5, 0.3, 1.0); // nice red/green uv colors

    //fragColor = vec4(texCoord * 0.5 + 0.5, 1.0, 1.0); // uv colors
    //fragColor = vec4(0.75, texCoord * 0.5 + 0.5, 1.0); // pink/orange variant
    //fragColor = vec4(texCoord.x * 0.25 + 0.25, 0.5, texCoord.y * 0.25 + 0.25, 1.0);




    velocityOut = velocity;
    gl_FragDepth = log2(z + 2.0) * 0.04;
}

