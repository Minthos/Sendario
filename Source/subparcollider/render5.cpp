#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <vector>
#include <array>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C" {
    #include "renderer.h"
}

const char *skyboxVertSource = R"glsl(
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;

void main()
{
    fragPos = position; // transform vertex from object space to world space
    vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to camera space
    gl_Position = posClip.xyww;
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
    vec2 uv = fragPos.xy;
    if(abs(fragPos.x) > 0.999){
        uv = fragPos.zy;
    } else if(abs(fragPos.y) > 0.999){
        uv = fragPos.xz;
    }
    uv *= 8.0;
    vec2 uv2 = vec2(uv.y, uv.x);
    float baseNoise = noise(uv * 250 + 50 + (time + 5.0) * -0.001);
    float basedNoise = noise(uv2 * 250 + 3.14 + (time + 10.0) * 0.0007);
    float crossNoise = (basedNoise + baseNoise) * 0.5;
    float baseNoise2 = noise(uv2 * -100 + 22 + (time + 15.0) * -0.0025);
    float basedNoise2 = noise(uv * -100 + 1.444 + (time + 23.0) * 0.0014);
    float crossNoise2 = (basedNoise2 + baseNoise2) * 0.5;
    crossNoise = max(crossNoise, crossNoise2);
    float detailNoise = noise(uv2 * 150.0 + time * 0.0015);
    float combinedNoise = mix(crossNoise, detailNoise, 0.6);
    //float intensity = smoothstep(0.8, 1.0, combinedNoise) * 1.0;
    float intensity = smoothstep(0.8, 1.0, combinedNoise) * 2.0;
    vec3 starColor = hsv2rgb(vec3(random(floor(gl_FragCoord.xy / 12) * 12), 0.7-intensity, intensity));
    FragColor = vec4(starColor, 1.0);
}
)glsl";

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
    vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to clip space
    vertexPosClip = posClip;  // Pass the clip space position to the fragment shader
    gl_Position = posClip;
}
)glsl";

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
//    vec4 sphereCenterCamera = view * vec4(sphereCenter, 1.0);
    vec3 ray = normalize(fragPos - cameraPos);
    vec3 sphereToCamera = cameraPos - sphereCenter;
    float b = dot(ray, sphereToCamera);
    float c = dot(sphereToCamera, sphereToCamera) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;

    vec3 radiance = vec3(0.0, 0.0, 0.0);
    if (discriminant < 0.0 || b > 0.0)
    {
        // eerie green glow for the debug
        //fragColor = vec4(0.0, 1.0, 0.0, 0.1); 
        //fragColor = vec4(fragPos, 0.25); 
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

Render a single composite per draw call. 24 vertices per boxoid so that faces don't have to share vertices.
Use a uniform buffer to store the edge roundness values. Each vertex gets the index to its face passed alongside
its coordinates to the vertex shader and from there to the fragment shader.

*/
const char *boxoidVertSource = R"glsl(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in int faceIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 fragNormal;
out vec4 vertexPosClip;
flat out int fragIndex;

void main()
{
    fragPos = vec3(model * vec4(position, 1.0)); // transform vertex from object space to world space
    vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to clip space
    vertexPosClip = posClip;  // Pass the clip space position to the fragment shader
    gl_Position = posClip;
    fragNormal = normal;
    fragIndex = faceIndex;
}
)glsl";

const char *boxoidFragSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec4 vertexPosClip;
flat in int fragIndex;

struct light {
    vec3 position;
    vec3 color; // W/sr per component
};

const int MAX_LIGHTS = 1;

uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform mat4 view;
uniform vec3 boxoidCenter;
uniform vec3 cameraPos;
uniform vec3 materialDiffuse;
uniform vec3 materialEmissive; // W/sr/m^2
uniform float gamma;
uniform float exposure;

out vec4 fragColor;

void main()
{
//    vec4 boxoidCenterCamera = view * vec4(boxoidCenter, 1.0);
    vec3 ray = normalize(fragPos - cameraPos);
    vec3 sphereToCamera = cameraPos - boxoidCenter;
    float b = dot(ray, sphereToCamera);
    //float c = dot(sphereToCamera, sphereToCamera) - sphereRadius * sphereRadius;
    float c = dot(sphereToCamera, sphereToCamera) - 1;
    float discriminant = b * b - c;
    vec3 radiance = vec3(0.0, 0.0, 0.0);
    if (discriminant < 0.0 || b > 0.0)
    {
        fragColor = vec4(fragNormal, 1.0);
        //fragColor = vec4(0.0, 1.0, 0.0, 0.1); 
        //fragColor = vec4(fragPos, 0.25);
        //switch(fragIndex)
        //{
        //    case 0: fragColor = vec4(1.0, 0.0, 0.0, 1.0); break;
        //    case 1: fragColor = vec4(0.0, 1.0, 0.0, 1.0); break;
        //    case 2: fragColor = vec4(0.0, 0.0, 1.0, 1.0); break;
        //    case 3: fragColor = vec4(1.0, 1.0, 0.0, 1.0); break;
        //    case 4: fragColor = vec4(1.0, 0.0, 1.0, 1.0); break;
        //    case 5: fragColor = vec4(0.0, 1.0, 1.0, 1.0); break;
        //    default: fragColor = vec4(1.0); // white
        //}
    }
    else
    {
        float t = -b - sqrt(discriminant);
        vec3 intersectionPos = cameraPos + ray * t;
        vec3 surfaceNormal = normalize(intersectionPos - boxoidCenter);
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

glm::vec3 vectorize(float in[3]) {
    return glm::vec3(in[0], in[1], in[2]);
}

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

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    int faceIndex;
};

// edges
// -- vertical
// 0 left front corner
// 1 left back corner
// 2 right front corner
// 3 right back corner
// -- transverse
// 4 bottom front
// 5 bottom back
// 6 top front
// 7 top back
// -- inline
// 8 bottom left
// 9 bottom right
// 10 top left
// 11 top right

glm::vec3 vlerp(glm::vec3 a, glm::vec3 b, float f) {
    return (a * f) + (b * (1.0f - f));
}
/*      corners
	{2.5, -1.8, 1.0, // right bottom rear
	-2.5, -1.8, 1.0, // left bottom rear
	2.5, -1.8, -1.3, // right bottom front
	-2.5, -1.8, -1.3, // left bottom front
	2.5, 1.8, 1.0, // right top rear
	-2.5, 1.8, 1.0, // left top rear
	2.5, 1.8, -1.3, // right top front
	-2.5, 1.8, -1.3}, // left top front
*/
void setupBoxoid(Boxoid box, GLuint& VAO, GLuint& VBO, GLuint& EBO) {
    Vertex vertices[24];
    GLuint indices[36];
    GLuint faceCornerIndices[6][4] = {
        {0, 1, 5, 4}, // right - bottom rear - left -- left - top rear - right // rear plate
        {3, 2, 6, 7}, // left - bottom front - right -- right top front - left // front plate
        {1, 0, 2, 3}, // left - bottom rear - right -- right bottom front left // bottom plate
        {4, 5, 7, 6}, // right top rear left -- left top front right		   // top plate
        {0, 4, 6, 2}, // right bottom rear - top -- right top front - bottom   // right plate
        {1, 3, 7, 5}  // left bottom rear - front -- left top front - rear	   // left plate
    };
    glm::vec3 corners[8];
    glm::vec3 cornerNormals[8];
    glm::vec3 center = glm::vec3(0, 0, 0);
    for(int i = 0; i < 8; i++) {
        corners[i] = vectorize(&box.corners[i * 3]);
        center += corners[i];
    }
    center *= (1.0/8.0);
    for(int i = 0; i < 8; i++) {
        cornerNormals[i] = glm::normalize(corners[i] - center);
    }
    for (int i = 0; i < 6; i++) {
        indices[i * 6 + 0] = 4 * i + 0;
        indices[i * 6 + 1] = 4 * i + 1;
        indices[i * 6 + 2] = 4 * i + 2;
        indices[i * 6 + 3] = 4 * i + 0;
        indices[i * 6 + 4] = 4 * i + 2;
        indices[i * 6 + 5] = 4 * i + 3;
        float time = static_cast<float>(glfwGetTime());
        glm::vec3 faceNormal = glm::normalize(glm::cross(
					corners[faceCornerIndices[i][0]] - corners[faceCornerIndices[i][2]],
					corners[faceCornerIndices[i][1]] - corners[faceCornerIndices[i][3]]));
        for(int j = 0; j < 4; j++) {
			int iself = faceCornerIndices[i][j];
			int iplus = faceCornerIndices[i][(j + 1) % 4];
			int iminus = faceCornerIndices[i][(j + 3) % 4];
            vertices[i * 4 + j].position = corners[iself];
            vertices[i * 4 + j].faceIndex = i;
			float curvature1 = box.curvature[i * 2 + (j % 2)];
			float curvature2 = box.curvature[i * 2 + ((j + 1) % 2)];
			float flatness1 = 1.0f - curvature1;
			float flatness2 = 1.0f - curvature2;
            glm::vec3 edgeU = glm::normalize(corners[iself] - corners[iminus]);
            glm::vec3 edgeV = glm::normalize(corners[iself] - corners[iplus]);
			glm::vec3 component1 = edgeV * 0.5f * curvature1 + faceNormal * flatness1;
			glm::vec3 component2 = edgeU * 0.5f * curvature2 + faceNormal * flatness2;
			vertices[i * 4 + j].normal = glm::normalize(component1 + component2);
			//vertices[i * 4 + j].normal = faceNormal;
        }
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, faceIndex));
    glEnableVertexAttribArray(2);
}

typedef struct {
    Sphere *spheres;
    int numSpheres;
    Sphere *nextBatch;
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

    // shaders for boxoids
    GLuint boxoidVert = compileShader(GL_VERTEX_SHADER, boxoidVertSource);
    GLuint boxoidFrag = compileShader(GL_FRAGMENT_SHADER, boxoidFragSource);
    GLuint boxoidProgram = glCreateProgram();
    glAttachShader(boxoidProgram, boxoidFrag);
    glAttachShader(boxoidProgram, boxoidVert);
    glLinkProgram(boxoidProgram);
    checkShader(GL_LINK_STATUS, boxoidProgram);
 
    // shader for the star background
    GLuint skyboxVert = compileShader(GL_VERTEX_SHADER, skyboxVertSource);
    GLuint skyboxFrag = compileShader(GL_FRAGMENT_SHADER, skyboxFragSource);
    GLuint skyboxProgram = glCreateProgram();
    glAttachShader(skyboxProgram, skyboxFrag);
    glAttachShader(skyboxProgram, skyboxVert);
    glLinkProgram(skyboxProgram);
    checkShader(GL_LINK_STATUS, skyboxProgram);
 
    // buffers
    GLuint boxoidVAO, boxoidVBO, boxoidEBO;
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // uniforms
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

    GLuint boxoidModelLoc = glGetUniformLocation(boxoidProgram, "model");
    GLuint boxoidViewLoc = glGetUniformLocation(boxoidProgram, "view");
    GLuint boxoidProjLoc = glGetUniformLocation(boxoidProgram, "projection");
    GLuint boxoidCenterLoc = glGetUniformLocation(boxoidProgram, "boxoidCenter");
    GLuint boxoidCameraPosLoc = glGetUniformLocation(boxoidProgram, "cameraPos");
    GLuint boxoidLightPositionsLoc = glGetUniformLocation(boxoidProgram, "lightPositions");
    GLuint boxoidLightColorsLoc = glGetUniformLocation(boxoidProgram, "lightColors");
    GLuint boxoidDiffuseLoc = glGetUniformLocation(boxoidProgram, "materialDiffuse");
    GLuint boxoidEmissiveLoc = glGetUniformLocation(boxoidProgram, "materialEmissive");
    GLuint boxoidGammaLoc = glGetUniformLocation(boxoidProgram, "gamma");
    GLuint boxoidExposureLoc = glGetUniformLocation(boxoidProgram, "exposure");

    GLuint skyboxModelLoc = glGetUniformLocation(skyboxProgram, "model");
    GLuint skyboxViewLoc = glGetUniformLocation(skyboxProgram, "view");
    GLuint skyboxProjLoc = glGetUniformLocation(skyboxProgram, "projection");
    GLuint timeLocation = glGetUniformLocation(skyboxProgram, "time");
    GLuint resolutionLocation = glGetUniformLocation(skyboxProgram, "resolution");

    // viewport
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(sharedData.window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 1.0f, 1e12f);
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

        // vertex buffer
        glBindVertexArray(sphereVAO);
        
        // Camera
        cameraPos = glm::vec3(sharedData.renderMisc.camPosition[0], sharedData.renderMisc.camPosition[1], sharedData.renderMisc.camPosition[2]);
        cameraTarget = glm::vec3(sharedData.renderMisc.camForward[0], sharedData.renderMisc.camForward[1], sharedData.renderMisc.camForward[2]);
        cameraUp = glm::vec3(sharedData.renderMisc.camUp[0], sharedData.renderMisc.camUp[1], sharedData.renderMisc.camUp[2]);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
       
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

        glUseProgram(boxoidProgram);
        glUniform3f(boxoidCameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniformMatrix4fv(boxoidViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(boxoidProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(boxoidGammaLoc, 2.2f);
        glUniform1f(boxoidExposureLoc, 200.0f);
        for (int i = 0; i < MAX_LIGHTS; ++i) {
                glUniform3fv(boxoidLightPositionsLoc + i, 1, &sharedData.renderMisc.lights[i].position[0]);
            glUniform3fv(boxoidLightColorsLoc + i, 1, &sharedData.renderMisc.lights[i].color[0]);
        }
        
        glUseProgram(shaderProgram);
        glUniform3f(cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(gammaLoc, 2.2f);
        glUniform1f(exposureLoc, 200.0f);
        for (int i = 0; i < MAX_LIGHTS; ++i) {
                glUniform3fv(lightPositionsLoc + i, 1, &sharedData.renderMisc.lights[i].position[0]);
            glUniform3fv(lightColorsLoc + i, 1, &sharedData.renderMisc.lights[i].color[0]);
        }

        // Geometry
        if (sharedData.spheres != NULL) {
            for (int i = 0; i < sharedData.numSpheres; ++i) {
                Sphere currentSphere = sharedData.spheres[i];
                glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(currentSphere.radius));

                // translate our model view to position the sphere in world space
                model = glm::translate(glm::mat4(1.0f), glm::vec3(currentSphere.position[0], currentSphere.position[1], currentSphere.position[2]));
                model = model * scaling;
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                // send the position of the sphere directly to the fragment shader, bypassing the vertex shader
                glUniform3f(sphereCenterLoc, currentSphere.position[0], currentSphere.position[1], currentSphere.position[2]);
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
                if(i < 5) {
                    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                }
            }
        }

        if(sharedData.numSpheres >= 5) {
            glUseProgram(boxoidProgram);
            
            glm::vec3 center = glm::vec3(sharedData.spheres[5].position[0], sharedData.spheres[5].position[1], sharedData.spheres[5].position[2]);
            Boxoid box = {
                {2.5, -1.8, 1.0,
                -2.5, -1.8, 1.0,
                2.5, -1.8, -1.3,
                -2.5, -1.8, -1.3,
                2.5, 1.8, 1.0,
                -2.5, 1.8, 1.0,
                2.5, 1.8, -1.3,
                -2.5, 1.8, -1.3},
                {1.0, 1.0,
                1.0, 1.0,
                1.0, 1.0,
                1.0, 1.0,
                1.0, 1.0,
                1.0, 1.0},
                4, 0};
			/*Boxoid box = {
                {2.5, -1.8, 1.0, // right bottom rear
                -2.5, -1.8, 1.0, // left bottom rear
                2.5, -1.8, -1.3, // right bottom front
                -2.5, -1.8, -1.3, // left bottom front
                2.5, 1.8, 1.0, // right top rear
                -2.5, 1.8, 1.0, // left top rear
                2.5, 1.8, -1.3, // right top front
                -2.5, 1.8, -1.3}, // left top front
                {1.0, 1.0,
                0.0, 0.0,
                1.0, 1.0,
                0.0, 0.0,
                1.0, 1.0,
                0.0, 0.0},
                4, 0};*/
            for (int i = 0; i < 8; i++) {
                box.corners[i * 3] += center.x;
                box.corners[i * 3 + 1] += center.y;
                box.corners[i * 3 + 2] += center.z;
            }
            model = glm::translate(glm::mat4(1.0f), center);
            glBindVertexArray(0);
            setupBoxoid(box, boxoidVAO, boxoidVBO, boxoidEBO);
            
            // translate our model view to position the sphere in world space
            model = glm::translate(glm::mat4(1.0f), glm::vec3());
            glUniformMatrix4fv(boxoidModelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(boxoidCenterLoc, center[0], center[1], center[2]);
               
            glm::vec3 diffuseComponent = vectorize(sharedData.renderMisc.materials[box.material_idx].diffuse);
            glm::vec3 emissiveComponent = vectorize(sharedData.renderMisc.materials[box.material_idx].emissive);

            float vibe = 3.0;
            diffuseComponent = glm::vec3(std::pow(diffuseComponent.r, vibe), std::pow(diffuseComponent.g, vibe), std::pow(diffuseComponent.b, vibe));
            glUniform3fv(diffuseLoc, 1, glm::value_ptr(diffuseComponent));
            glUniform3fv(emissiveLoc, 1, glm::value_ptr(emissiveComponent));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        else {
            printf("numSpheres: %d\n", sharedData.numSpheres);
        }
        glBindVertexArray(0);
        glfwSwapBuffers(sharedData.window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

// thread safe entry point copies all the data to keep things dumb.
extern "C" void render(Sphere* spheres, size_t sphereCount, render_misc renderMisc) {
    pthread_mutex_lock(&sharedData.mutex);
    if(sharedData.nextBatch != NULL) {
        free(sharedData.nextBatch);
    }
    sharedData.nextBatch = (Sphere*)malloc(sizeof(Sphere) * sphereCount);
    memcpy(sharedData.nextBatch, spheres, sizeof(Sphere) * sphereCount);
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


