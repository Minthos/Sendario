#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform isampler2D velocityTexture;
uniform float antialiasing;
uniform float inv_strength;
uniform int mode;
uniform int screen_width;
uniform int screen_height;

void main() {
    vec2 coords = TexCoords * antialiasing;
   
    if(mode == 0 && antialiasing == 1){
        FragColor = texture(screenTexture, coords);
        return;
    }

    int iterations = 16;
    int aa_stride = 4;
    vec2 velocity = texture(velocityTexture, coords).xy / 4096.0;
    // metadata.x: z value before log2 conversion
    // metadata.y: which shader produced the pixel (terrainfrag = 1, boxfrag = 2)
    ivec2 metadata = texture(velocityTexture, coords).za;

    // disabling motion blur for the player character (well.. every box mesh actually..)
    // also disabling motion blur but leaving antialiasing
    if(metadata.y == 2 || mode == 0){
        velocity = vec2(0);
    }
    // this works to reduce stuttering when rendering terrain close to the camera at high AA levels
    if(metadata.x < 50){
        velocity *= metadata.x;
        velocity /= 50;
    }

    velocity /= (0.5 * iterations * (1 + inv_strength));

    // suppress motion blur for slow-moving pixels
    float magnitude = velocity.length;
    if(magnitude < 2){
        velocity *= (0.5 * magnitude);
    }

    vec4 color = vec4(0);
    float sum_weight = 0.0;
    vec2 aa_size = vec2(0.25 / screen_width, 0.25 / screen_height);

    for(int i = -iterations; i < iterations; i++){
        vec2 aa_offset = vec2((((i / aa_stride) - 1.5) * aa_size.x), (((i % aa_stride) - 1.5) * aa_size.y));
        vec2 adjusted_coords = aa_offset + coords + i * velocity;
        // this test prevents the terrain motion from blurring boxes, but I should find a better way to solve this
        if(texture(velocityTexture, adjusted_coords).a == metadata.y){
            float weight = 1.0;
            if(mode == 1) { // weighted mode makes the blur weaker further from the object, sharpening the image
                weight = 1.0 / (2 + abs(i));
            }
            color += weight * texture(screenTexture, adjusted_coords);
            sum_weight += weight;
        }
    }
    FragColor = color / sum_weight;
}


// TODO:
// Instead of reading from points along the +- velocity vector, we should write to points along it.
// That means that we have to accumulate the weight in the 4th component of the output buffer and
// do a second pass where we divide the color values for each pixel by the accumulated weight.

