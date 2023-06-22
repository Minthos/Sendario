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

void main()
{
    fragPos = vec3(model * vec4(position, 1.0)); // transform vertex from object space to world space
    gl_Position = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to camera space
}
)glsl";


const char *fragmentShaderSource = R"glsl(
#version 330 core

in vec3 fragPos;

uniform mat4 view;
uniform vec3 sphereCenter;
uniform vec3 cameraPos;
uniform float sphereRadius;
uniform vec3 lightDirection;

out vec4 fragColor;

void main()
{   
    // Transform the sphere center from world space to camera space
    vec4 sphereCenterCamera = view * vec4(sphereCenter, 1.0);
    
    // Calculate the ray from the camera to the fragment in camera space
    vec3 ray = normalize(fragPos - cameraPos);
    
    // Calculate the intersection of the ray with the sphere
    vec3 sphereToCamera = cameraPos - sphereCenter;
    float b = dot(ray, sphereToCamera);
    float c = dot(sphereToCamera, sphereToCamera) - sphereRadius * sphereRadius;
    float d = b * b - c;
    
    // If the ray does not intersect the sphere, set the color of the fragment to transparent
    if (d < 0.0)
    {
        fragColor = vec4(0.0, 1.0, 0.0, 0.1);
    }
    else
    {
        // Calculate the intersection point
        float t = -b - sqrt(d);
        vec3 intersection = cameraPos + ray * t;

        // Calculate the surface normal at the intersection point
        vec3 surfaceNormal = normalize(intersection - sphereCenter);
        
        // Calculate the diffuse lighting
        float diffuse = max(dot(surfaceNormal, -lightDirection), 0.03);

        // Set the color of the fragment based on the lighting
        fragColor = vec4(vec3(diffuse), 1.0);
    }
    fragColor = vec4(1.0);
}
)glsl";

const char *sparklyShaderSource = R"glsl(
#version 330 core

uniform vec2 u_resolution;
uniform float u_time;

float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise (vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{   
    vec2 st = gl_FragCoord.xy / u_resolution.xy;

    float flicker = pow(noise(vec2(st.x, u_time * 10.0)), 3.0);
    float stars = pow(noise(vec2(st.x * 10.0, st.y * 10.0)), 3.0);

    vec3 color = vec3(stars * flicker);

    gl_FragColor = vec4(color, 1.0);
}
)glsl";


void checkShader(GLenum status_enum, GLuint shader) {
    GLint success;
    GLchar infoLog[512];
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

    // Make the window's context current
    glfwMakeContextCurrent(sharedData.window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        sharedData.shouldExit = true;
        glfwTerminate();
        return 0;
    }

    // compile the vertex and fragment shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // linkw them into one program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShader(GL_LINK_STATUS, shaderProgram);

    // prepare buffers
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

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint sphereCenterLoc = glGetUniformLocation(shaderProgram, "sphereCenter");
    GLuint sphereRadiusLoc = glGetUniformLocation(shaderProgram, "sphereRadius");
    GLuint lightDirectionLoc = glGetUniformLocation(shaderProgram, "lightDirection");
    GLuint cameraPosLoc = glGetUniformLocation(shaderProgram, "cameraPos");

    int screenWidth, screenHeight;
    glfwGetFramebufferSize(sharedData.window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    // Camera setup
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)screenWidth / (float)screenHeight, 1.0f, 1e12f);
    glm::vec3 cameraPos = glm::vec3(0.0f, 1e10f, 0.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

    // Rendering loop
    while (!glfwWindowShouldClose(sharedData.window) && !sharedData.shouldExit) {
        usleep(8000);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glm::mat4 model = glm::mat4(1.0f);

        // Read data
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

        int camIdx = sharedData.numSpheres - 1;
        cameraPos = glm::vec3(sharedData.spheres[camIdx].x, sharedData.spheres[camIdx].y, sharedData.spheres[camIdx].z);
        cameraTarget = glm::vec3(sharedData.renderMisc.camDirection[0], sharedData.renderMisc.camDirection[1], sharedData.renderMisc.camDirection[2]);
        glUniform3f(cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
 
        // Render spheres
        glBindVertexArray(VAO);
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
                glm::vec3 lightDirection = glm::normalize(glm::vec3(1.0f, -1.0f, -1.0f));
                glUniform3fv(lightDirectionLoc, 1, glm::value_ptr(lightDirection));

                // Draw the cube
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


