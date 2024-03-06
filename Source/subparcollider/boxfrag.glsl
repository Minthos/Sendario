#version 430

in vec3 normal;

out vec4 fragColor;

void main() {
    float brightness = dot(normalize(normal), normalize(vec3(0, 0, 1)));
    vec3 color = vec3(0.1, 0.5, 0.8) * brightness;
    fragColor = vec4(color, 1.0);
}

