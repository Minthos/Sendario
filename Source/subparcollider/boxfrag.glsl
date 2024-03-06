#version 430

in vec3 normal;

out vec4 fragColor;

void main() {
    float brightness = 1.0;
    vec3 color = vec3(0.1, 0.5, 0.8) * brightness;
    fragColor = vec4(color, 1.0);
}

