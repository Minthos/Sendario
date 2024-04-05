#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform isampler2D velocityTexture;
uniform float antialiasing;
uniform float inv_strength;
uniform int mode;

void main() {
    vec2 coords = TexCoords * antialiasing;
   
    if(mode == 0){
        FragColor = texture(screenTexture, coords);
        return;
    }

    int iterations = 16;
    vec2 velocity = texture(velocityTexture, coords).xy / 4096.0;
    ivec2 metadata = texture(velocityTexture, coords).za;

    // disabling motion blur for the player character (well.. every box mesh actually..)
    if(metadata.y == 2){
        FragColor = texture(screenTexture, coords);
        return;
    }
    // this works to reduce stuttering when rendering terrain close to the camera at high AA levels but
    // I should probably clamp velocity instead to something reasonable
    if(metadata.x < 500){
        FragColor = texture(screenTexture, coords);
        return;
    }

    velocity /= (0.5 * iterations * (1 + inv_strength));

    // suppress motion blur for slow-moving pixels
    float magnitude = velocity.length;
    if(magnitude < 2){
        velocity *= (0.5 * magnitude);
    }

    vec4 color = vec4(0);
    float sum_weight = 0.0;

    if(mode == 1) { // weighted mode makes the blur weaker further from the object, sharpening the image
        for(int i = -iterations; i < iterations; i++){
            // this test prevents the terrain motion from blurring boxes, but I should find a better way to solve this
            if(texture(velocityTexture, coords + i * velocity).a == metadata.y){
                float weight = 1.0 / (2 + abs(i));
                color += weight * texture(screenTexture, coords + i * velocity);
                sum_weight += weight;
            }
        }
    }
    else if(mode == 2) { // linear mode gives a stronger, smoother blur
        for(int i = -iterations; i < iterations; i++){
            float weight = 1.0;
            color += weight * texture(screenTexture, coords + i * velocity);
            sum_weight += weight;
        }
    } else {
        color = texture(screenTexture, coords);
        sum_weight = 1.0;
    }
    FragColor = color / sum_weight;
}


// TODO:
// Instead of reading from points along the +- velocity vector, we should write to points along it.
// That means that we have to accumulate the weight in the 4th component of the output buffer and
// do a second pass where we divide the color values for each pixel by the accumulated weight.

