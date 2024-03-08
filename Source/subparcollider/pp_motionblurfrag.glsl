#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D velocityTexture;
uniform int mode;

void main() {
    if(mode == 0){
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    int iterations = 16;
    vec2 velocity = texture(velocityTexture, TexCoords).xy;
    velocity /= (iterations * 3);

    // suppress motion blur for slow-moving pixels
    float magnitude = velocity.length;
    if(magnitude < 2){
        velocity *= (0.5 * magnitude);
    }

    vec4 color = vec4(0);
    float sum_weight = 0.0;

    if(mode == 1) { // weighted mode makes the blur weaker further from the object, sharpening the image
        for(int i = -iterations; i < iterations; i++){
            float weight = 1.0 / (3 + abs(i));
            color += weight * texture(screenTexture, TexCoords + i * velocity);
            sum_weight += weight;
        }
    }
    else if(mode == 2) { // linear mode gives a stronger, smoother blur
        for(int i = -iterations; i < iterations; i++){
            float weight = 1.0;
            color += weight * texture(screenTexture, TexCoords + i * velocity);
            sum_weight += weight;
        }
    }
    FragColor = color / sum_weight;
}

