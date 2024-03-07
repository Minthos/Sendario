#version 430

in vec2 texCoord;
uniform sampler2D tex;

out vec4 fragColor;

void main() {
    //fragColor = vec4(1.0);
    fragColor = texture(tex, texCoord);
}

