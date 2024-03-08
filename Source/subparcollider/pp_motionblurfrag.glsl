#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D velocityTexture;
//uniform int iterations;
uniform int mode;

void main() {
    int iterations = 16;
    if(mode == 0){
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    vec2 velocity = texture(velocityTexture, TexCoords).xy;
    velocity /= (iterations * 4);

    float magnitude = velocity.length;
    if(magnitude < 5){
        velocity *= (0.2 * magnitude);
    }


    vec4 color = vec4(0);
    float sum_weight = 0.0;

    if(mode == 1) {
        for(int i = -iterations; i < iterations; i++){
            float weight = 1.0 / (3 + abs(i));
            color += weight * texture(screenTexture, TexCoords + i * velocity);
            sum_weight += weight;
        }
    }
    else if(mode == 2){
        for(int i = -iterations; i < iterations; i++){
            float weight = 1.0;
            color += weight * texture(screenTexture, TexCoords + i * velocity);
            sum_weight += weight;
        }
    }
    FragColor = color / sum_weight;
}

