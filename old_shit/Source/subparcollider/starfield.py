import glfw
from OpenGL.GL import *
from OpenGL.GL.shaders import compileShader, compileProgram

FRAGMENT_SHADER = """
#version 330 core

uniform float time;
uniform vec2 resolution;
out vec4 FragColor;

float random(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    vec2 w = f * f * (3.0 - 2.0 * f);

    return mix(mix(random(i), random(i + vec2(1.0, 0.0)), w.x),
               mix(random(i + vec2(0.0, 1.0)), random(i + vec2(1.0, 1.0)), w.x),
               w.y);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    uv *= 8.0;
    vec2 uv2 = vec2(uv.y, uv.x);
    float baseNoise = noise(uv * 250 + 50 + time * -0.1);
    float basedNoise = noise(uv2 * 250 + 3.14 + time * 0.07);
    float crossNoise = (basedNoise + baseNoise) * 0.5;
    float baseNoise2 = noise(uv2 * -100 + 22 + time * -0.25);
    float basedNoise2 = noise(uv * -100 + 1.444 + time * 0.14);
    float crossNoise2 = (basedNoise2 + baseNoise2) * 0.5;
    crossNoise = max(crossNoise, crossNoise2);
    //crossNoise = crossNoise2;
    float detailNoise = noise(uv2 * 150.0 + time * 0.15);
    float combinedNoise = mix(crossNoise, detailNoise, 0.6);
    float intensity = smoothstep(0.8, 1.0, combinedNoise) * 1.0;
    vec3 starColor = hsv2rgb(vec3(random(floor(gl_FragCoord.xy / 12) * 12), 0.7-intensity, intensity));
    FragColor = vec4(starColor, 1.0);
}


"""

def main():
    glfw.init()
    window = glfw.create_window(3840, 2160, "Starfield", None, None)
    glfw.make_context_current(window)
    fragment_shader = compileShader(FRAGMENT_SHADER, GL_FRAGMENT_SHADER)
    program = compileProgram(fragment_shader)
    resolution_location = glGetUniformLocation(program, "resolution")
    time_location = glGetUniformLocation(program, "time")
    glClearColor(0.0, 0.0, 0.0, 1.0)
    while not glfw.window_should_close(window):
        time = glfw.get_time()
        glClear(GL_COLOR_BUFFER_BIT)
        glUseProgram(program)
        glUniform2f(resolution_location, 3840, 2160)
        glUniform1f(time_location, time)
        glBegin(GL_TRIANGLES)
        glVertex2f(-1.0, -1.0)
        glVertex2f( 3.0, -1.0)
        glVertex2f(-1.0,  3.0)
        glEnd()
        glfw.swap_buffers(window)
        glfw.poll_events()
    glfw.terminate()

if __name__ == '__main__':
    main()

