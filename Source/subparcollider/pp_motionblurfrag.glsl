#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D velocityTexture;
uniform int iterations;

void main() {
    vec2 velocity = texture(velocityTexture, TexCoords).xy;
    velocity /= 24;

    vec4 color = vec4(0);
    float sum_weight = 0.0;
    for(int i = 0; i < iterations; i++){
        float weight = iterations - i;
        color += weight * texture(screenTexture, TexCoords - i * velocity);
        sum_weight += weight;
    }
    for(int i = 1; i < iterations; i++){
        float weight = iterations - i;
        color += weight * texture(screenTexture, TexCoords + i * velocity);
        sum_weight += weight;
    }
    FragColor = color / sum_weight;
}

