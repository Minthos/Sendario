#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


extern "C" {
    #include "renderer.h"
}

const char *vertexShaderSource = R"glsl(
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec4 vertexPosClip;

void main()
{
    fragPos = vec3(model * vec4(position, 1.0)); // transform vertex from object space to world space
    vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to camera space
    vertexPosClip = posClip;  // Pass the clip space position to the fragment shader
    gl_Position = posClip;
}
)glsl";

// Setting both gl_Position and vertexPosClip to the same value might seem redundant.
// However, the GLSL specification stipulates that we cannot directly use gl_Position in the fragment
// shader, so we need a separate out variable to pass this data. In other words, gl_Position is not
// interpolated and accessible in the fragment shader, so we can't use it for calculations there.

const char *fragmentShaderSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec4 vertexPosClip;

struct light {
    vec3 position;
    vec3 color; // W/sr per component
};

const int MAX_LIGHTS = 1;

uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform mat4 view;
uniform vec3 sphereCenter;
uniform vec3 cameraPos;
uniform float sphereRadius;
uniform vec3 materialDiffuse;
uniform vec3 materialEmissive; // W/sr/m^2
uniform float gamma;
uniform float exposure;

out vec4 fragColor;

void main()
{
    vec4 sphereCenterCamera = view * vec4(sphereCenter, 1.0);
    vec3 ray = normalize(fragPos - cameraPos);
    vec3 sphereToCamera = cameraPos - sphereCenter;
    float b = dot(ray, sphereToCamera);
    float c = dot(sphereToCamera, sphereToCamera) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;

    vec3 radiance = vec3(0.0, 0.0, 0.0);
    if (discriminant < 0.0)
    {
	// eerie green glow for the debug
        //fragColor = vec4(0.0, 1.0, 0.0, 0.1); 
    }
    else
    {
        float t = -b - sqrt(discriminant);
        vec3 intersectionPos = cameraPos + ray * t;
        vec3 surfaceNormal = normalize(intersectionPos - sphereCenter);

	vec3 accumulatedLight = vec3(0.0, 0.0, 0.0);

	for (int i = 0; i < MAX_LIGHTS; ++i) {
	    vec3 lightVector = lightPositions[i] - fragPos;
	    float squaredDistance = dot(lightVector, lightVector);
	    vec3 lightDirection = lightVector / sqrt(squaredDistance);
	    float attenuation = 1.0 / squaredDistance;
	    float lambertian = max(dot(surfaceNormal, lightDirection), 0.0);
	    accumulatedLight += attenuation * materialDiffuse * lightColors[i] * lambertian;
	}

	// divide materialEmissive by 4 pi
	radiance = materialEmissive * 0.079577 + accumulatedLight;	
	
	// tone mapping radiance to the 0,1 range by dividing by total brightness. darkness is the inverse of brightness.
	float darkness = 3.0 / (3.0 + exposure + radiance.r + radiance.g + radiance.b);
	radiance = radiance * darkness;
	
	// gamma correction
	fragColor = vec4(pow(radiance, vec3(1.0 / gamma)), 1.0);
    }


    const float constantForDepth = 1.0;
    const float farDistance = 3e18;
    const float offsetForDepth = 1.0;
    gl_FragDepth = (log(constantForDepth * vertexPosClip.z + offsetForDepth) / log(constantForDepth * farDistance + offsetForDepth));
}

)glsl";


/*
from GPT:

The easiest way to implement ACES tone mapping in your shader might be to use the ACES filmic tone mapping curve, which is a piecewise function designed to produce visually pleasing results across a wide range of inputs.

Below is a GLSL implementation of the ACES filmic tone mapping curve:

```glsl
vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

void main()
{
    // Perform lighting calculations as before

    // Apply ACES Filmic Tone Mapping
    vec3 color = RRTAndODTFit(radiance * 0.6);
    color *= 1.0 / RRTAndODTFit(vec3(0.18));
    
    // Apply gamma correction
    color = pow(color, vec3(1.0 / gamma));

    // Set the final color
    fragmentColor = vec4(color, 1.0);

    // ... Rest of the shader
}
```

In this code, `RRTAndODTFit` is the ACES RRT (Reference Rendering Transform) and ODT (Output Device Transform) combined. The number `0.6` is an exposure bias which you might want to expose as a tweakable parameter in your game. The number `0.18` comes from middle grey and is used to maintain consistent brightness. The result of the tone mapping step is then gamma corrected and written to the output color.

Keep in mind that ACES is a large standard and the full ACES pipeline involves much more than just tone mapping, including color spaces, white balance, color grading, etc. However, just using the ACES tone curve can be a good first step to achieving more physically accurate rendering.

As always, the best tone mapping method to use depends on the specifics of your game and the aesthetic you're aiming for, so it's worth trying out a few different methods to see what works best in your case.

 */


const char *skyboxVertSource = R"glsl(
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 vertexPosClip;

void main()
{
    fragPos = position; // transform vertex from object space to world space
    vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to camera space
    vertexPosClip = position;
    gl_Position = posClip.xyww;
}
)glsl";

const char *skyboxVert2Source = R"glsl(
#version 330 core
layout (location = 0) in vec3 position;

//uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec4 vertexPosClip;

void main()
{
    fragPos = position;
    vec4 pos = projection * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
)glsl";

const char *skyboxFragSource = R"glsl(
#version 330 core

uniform float time;
uniform vec2 resolution;

in vec3 fragPos;
in vec3 vertexPosClip;
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
    //vec2 uv = gl_FragCoord.xy / resolution;
    vec2 uv = fragPos.xy;
    //vec2 uv = vertexPosClip.xy;

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
//    FragColor = vec4(0.5, 0.0, 0.5, 1.0);
}
)glsl";


void checkShader(GLenum status_enum, GLuint shader) {
    GLint success = 1;
    GLchar infoLog[512] = {0};
    glGetShaderiv(shader, status_enum, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation error: %s\n", infoLog);
    }
}

GLuint compileShader(GLenum shaderType, const char* source) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    checkShader(GL_COMPILE_STATUS, shader);
    return shader;
}

GLfloat vertices[] = {
    -1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, 1.0f,
     1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f
};

GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
    4, 5, 6,
    4, 6, 7,
    0, 1, 5,
    0, 4, 5,
    3, 2, 6,
    3, 6, 7,
    0, 3, 7,
    0, 4, 7,
    1, 2, 6,
    1, 5, 6
};

typedef struct {
    sphere *spheres;
    int numSpheres;
    sphere *nextBatch;
    int nextNum;
    render_misc renderMisc;
    pthread_t renderer_tid;
    pthread_mutex_t mutex;
    bool shouldExit;
    GLFWwindow *window;
} SharedData;

SharedData sharedData;

void *rendererThread(void *arg) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        sharedData.shouldExit = true;
        return 0;
    }

    // Create a window
    sharedData.window = glfwCreateWindow(3840, 2160, "Subparcollider", nullptr, nullptr);
    if (!sharedData.window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        sharedData.shouldExit = true;
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(sharedData.window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        sharedData.shouldExit = true;
        glfwTerminate();
        return 0;
    }

    // shaders for celestial objects
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShader(GL_LINK_STATUS, shaderProgram);

    // shader for the star background
    GLuint skyboxVert = compileShader(GL_VERTEX_SHADER, skyboxVertSource);
    GLuint skyboxFrag = compileShader(GL_FRAGMENT_SHADER, skyboxFragSource);
    GLuint skyboxProgram = glCreateProgram();
    glAttachShader(skyboxProgram, skyboxFrag);
    glAttachShader(skyboxProgram, skyboxVert);
    glLinkProgram(skyboxProgram);
    checkShader(GL_LINK_STATUS, skyboxProgram);
 
    // buffers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // uniforms
    //glUseProgram(shaderProgram);
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint sphereCenterLoc = glGetUniformLocation(shaderProgram, "sphereCenter");
    GLuint sphereRadiusLoc = glGetUniformLocation(shaderProgram, "sphereRadius");
    GLuint cameraPosLoc = glGetUniformLocation(shaderProgram, "cameraPos");
    GLuint lightPositionsLoc = glGetUniformLocation(shaderProgram, "lightPositions");
    GLuint lightColorsLoc = glGetUniformLocation(shaderProgram, "lightColors");
    GLuint diffuseLoc = glGetUniformLocation(shaderProgram, "materialDiffuse");
    GLuint emissiveLoc = glGetUniformLocation(shaderProgram, "materialEmissive");
    GLuint gammaLoc = glGetUniformLocation(shaderProgram, "gamma");
    GLuint exposureLoc = glGetUniformLocation(shaderProgram, "exposure");

    //glUseProgram(sparklyProgram);
    GLuint skyboxModelLoc = glGetUniformLocation(skyboxProgram, "model");
    GLuint skyboxViewLoc = glGetUniformLocation(skyboxProgram, "view");
    GLuint skyboxProjLoc = glGetUniformLocation(skyboxProgram, "projection");
    GLint timeLocation = glGetUniformLocation(skyboxProgram, "time");
    GLint resolutionLocation = glGetUniformLocation(skyboxProgram, "resolution");

    // viewport
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(sharedData.window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)screenWidth / (float)screenHeight, 1.0f, 1e12f);
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(1.0f, 0.0f, 0.0f);

    // Rendering loop
    while (!glfwWindowShouldClose(sharedData.window) && !sharedData.shouldExit) {
        usleep(8000);

        // Swap pointers with the thread that copies the data
	// This lets the rendering thread and physics simulation both run at full tilt
	// without having to wait for each other
        pthread_mutex_lock(&sharedData.mutex);
        if(sharedData.nextBatch != NULL) {
            if(sharedData.spheres != NULL) {
                free(sharedData.spheres);
            }
            sharedData.spheres = sharedData.nextBatch;
            sharedData.numSpheres = sharedData.nextNum;
            sharedData.nextBatch = NULL;
        }
        pthread_mutex_unlock(&sharedData.mutex);

	// Camera
        glUseProgram(shaderProgram);
        cameraPos = glm::vec3(sharedData.renderMisc.camPosition[0], sharedData.renderMisc.camPosition[1], sharedData.renderMisc.camPosition[2]);
        cameraTarget = glm::vec3(sharedData.renderMisc.camDirection[0], sharedData.renderMisc.camDirection[1], sharedData.renderMisc.camDirection[2]);
        glUniform3f(cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// vertex buffer
        glBindVertexArray(VAO);
	
	// draw the skybox
        glUseProgram(skyboxProgram);
        float time = static_cast<float>(glfwGetTime());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(skyboxModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(timeLocation, time);
        glUniform2f(resolutionLocation, 3840, 2160);
        glDisable(GL_DEPTH_TEST);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glEnable(GL_DEPTH_TEST);

	// clear the depth buffer so the skybox is behind everything else
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
	
	// Brightness controls
	glUniform1f(gammaLoc, 2.2f);
	glUniform1f(exposureLoc, 200.0f);

	// Lights
	for (int i = 0; i < MAX_LIGHTS; ++i) {
    	    glUniform3fv(lightPositionsLoc + i, 1, &sharedData.renderMisc.lights[i].position[0]);
            glUniform3fv(lightColorsLoc + i, 1, &sharedData.renderMisc.lights[i].color[0]);
        }

        // Geometry
        if (sharedData.spheres != NULL) {
            for (int i = 0; i < sharedData.numSpheres; ++i) {
                sphere currentSphere = sharedData.spheres[i];
                glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(currentSphere.radius));

                // translate our model view to position the sphere in world space
                model = glm::translate(glm::mat4(1.0f), glm::vec3(currentSphere.x, currentSphere.y, currentSphere.z));
                model = model * scaling;
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                // send the position of the sphere directly to the fragment shader, bypassing the vertex shader
                glUniform3f(sphereCenterLoc, currentSphere.x, currentSphere.y, currentSphere.z);
                glUniform1f(sphereRadiusLoc, currentSphere.radius);
               
	       	material* mat = &sharedData.renderMisc.materials[currentSphere.material_idx];


		glm::vec3 diffuseComponent = glm::vec3(mat->diffuse[0], mat->diffuse[1], mat->diffuse[2]); // 0 to 1
		glm::vec3 emissiveComponent = glm::vec3(mat->emissive[0], mat->emissive[1], mat->emissive[2]); // W/m^2
	
		// colors looked washed out so I did a thing. not quite vibrance so I'll call it vibe. texture saturation? but we don't have textures.
		float vibe = 3.0;
		diffuseComponent = glm::vec3(std::pow(diffuseComponent.r, vibe), std::pow(diffuseComponent.g, vibe), std::pow(diffuseComponent.b, vibe));
		glUniform3fv(diffuseLoc, 1, glm::value_ptr(diffuseComponent));
		glUniform3fv(emissiveLoc, 1, glm::value_ptr(emissiveComponent));

                // Draw the sphere
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
        }
        glBindVertexArray(0);

        glfwSwapBuffers(sharedData.window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

// thread safe entry point copies all the data to keep things dumb.
extern "C" void render(sphere* spheres, size_t sphereCount, render_misc renderMisc) {
    pthread_mutex_lock(&sharedData.mutex);
    if(sharedData.nextBatch != NULL) {
        free(sharedData.nextBatch);
    }
    sharedData.nextBatch = (sphere*)malloc(sizeof(sphere) * sphereCount);
    memcpy(sharedData.nextBatch, spheres, sizeof(sphere) * sphereCount);
    sharedData.nextNum = sphereCount;
    sharedData.renderMisc = renderMisc;
    pthread_mutex_unlock(&sharedData.mutex);
}

extern "C" void startRenderer() {
    // set up shared data
    sharedData.spheres = NULL;
    sharedData.numSpheres = 0;
    sharedData.nextBatch = NULL;
    sharedData.nextNum = 0;
    sharedData.shouldExit = false;
    pthread_mutex_init(&sharedData.mutex, NULL);

    // start the rendering thread
    pthread_create(&sharedData.renderer_tid, NULL, rendererThread, &sharedData);
}

extern "C" void stopRenderer() {
    sharedData.shouldExit = true;
    pthread_join(sharedData.renderer_tid, NULL);
    pthread_mutex_destroy(&sharedData.mutex);
}


