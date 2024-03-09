#include "collisiontree.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <vector>



// Rendering quality settings
float anisotropy = 16.0f; // should be 4 with no upscaling, 16 with upscaling
int upscaling_factor = 2; // 1 (no upscaling) and 2 (4 samples per pixel) are good values
int motion_blur_mode = 1; // 0: off, 1: nonlinear (sharp), 2: linear (blurry)
float motion_blur_invstr = 2.0f; // motion blur amount. 0: very high, 1: high, 2: medium, 3: low, 5: very low


auto now = std::chrono::high_resolution_clock::now;
using std::string;

std::unordered_map<string, GLuint> shaders;
std::unordered_map<string, GLuint> textures;

char* readShaderSource(const char* filePath) {
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    size_t bytesRead = fread(buffer, 1, length, file);
    if (bytesRead != (size_t)length) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    buffer[length] = '\0';

    fclose(file);
    return buffer;
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

GLuint mkShader(string name) {
    string vert_path = name + "vert.glsl";
    string frag_path = name + "frag.glsl";
    char *vert_src = readShaderSource(vert_path.c_str());
    char *frag_src = readShaderSource(frag_path.c_str());

    GLuint vert = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, frag_src);
    free(vert_src);
    free(frag_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    checkShader(GL_LINK_STATUS, program);
    return program;
}

GLuint loadTexture(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture" << std::endl;
        exit(1);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return texture;
}

struct texvert {
    glm::vec3 xyz;
    glm::vec2 uv;

    texvert(glm::vec3 a, glm::vec2 b) { xyz = a; uv = b; }
};

struct RenderObject {
    PhysicsObject *po;
    GLuint shader;
    GLuint texture;
    GLuint vao, vbo, ebo;

    glm::mat4 prev;
    bool firstTime;

    RenderObject(PhysicsObject *ppo) { po = ppo; firstTime = true; }
};

struct GameObject {
    std::vector<RenderObject> ro;
};

void convertMeshToOpenGLBuffers(RenderObject *obj) {
    glGenVertexArrays(1, &obj->vao);
    glBindVertexArray(obj->vao);

    std::vector<texvert> vertices;
    std::vector<GLuint> indices;

    // this ordering reverses the texture on two sides so a texture tiles correctly horizontally
    uint32_t faces[6 * 4] =
        {0, 1, 2, 3, // top
        0, 1, 4, 5, // front
        0, 2, 4, 6, // left
        3, 1, 7, 5, // right
        3, 2, 7, 6, // back
        5, 4, 7, 6}; // bottom

    glm::vec2 texture_corners[4] = {
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f)};

    for (uint32_t i = 0; i < 6; ++i) {
        dTri* t1 = &obj->po->mesh.tris[i * 2];
        dTri* t2 = &obj->po->mesh.tris[i * 2 + 1];

        texvert verts[4] = {
            texvert(vec3(obj->po->mesh.verts[faces[i * 4    ]]), texture_corners[0]),
            texvert(vec3(obj->po->mesh.verts[faces[i * 4 + 1]]), texture_corners[1]),
            texvert(vec3(obj->po->mesh.verts[faces[i * 4 + 2]]), texture_corners[2]),
            texvert(vec3(obj->po->mesh.verts[faces[i * 4 + 3]]), texture_corners[3])};
       
        vertices.insert(vertices.end(), {verts[0], verts[1], verts[2], verts[3]});

        // correct the winding order so we can use backface culling
        bool ccw = ((i == 2) || (i == 3) || (i == 5) || (i == 0));
        if(ccw) {
            indices.insert(indices.end(), {i * 4, i * 4 + 1, i * 4 + 2});
            indices.insert(indices.end(), {i * 4 + 3, i * 4 + 2, i * 4 + 1});
        } else {
            indices.insert(indices.end(), {i * 4 + 1, i * 4 + 2, i * 4 + 3});
            indices.insert(indices.end(), {i * 4 + 2, i * 4 + 1, i * 4});
        }
    }

    glGenBuffers(1, &obj->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(texvert), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &obj->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

GLFWwindow* createWindow(int width, int height, const char* title) {
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    return window;
}

void initializeGLEW() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        exit(EXIT_FAILURE);
    }
}

int screenwidth = 3840;
int screenheight = 2123;
int frames_rendered = 0;
auto prevFrameTime = now();
GLuint framebuffer, colorTex, velocityTex;
GLuint ppshader;
GLuint quadVAO, quadVBO;
float quadVertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

void initializeFramebuffer() {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &velocityTex);
    glBindTexture(GL_TEXTURE_2D, velocityTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupTextures(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, velocityTex, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer not complete!" << std::endl;
}

void resizeFramebuffer(int w, int h) {
    int width = w * upscaling_factor;
    int height = h * upscaling_factor;
    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, velocityTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
    GLuint depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
    setupTextures(width, height);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void reshape(GLFWwindow* window, int width, int height) {
    screenwidth = width;
    screenheight = height;
    resizeFramebuffer(screenwidth, screenheight);
}

void render(RenderObject *obj) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), obj->po->zoneSpacePosition());
    model = glm::rotate(model, frames_rendered * 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(2,1.5,1.5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenwidth / (float)screenheight, 0.1f, 100.0f);
    glm::mat4 transform = projection * view * model;

    glUseProgram(obj->shader);
    glUniformMatrix4fv(glGetUniformLocation(obj->shader, "current"), 1, GL_FALSE, &transform[0][0]);
    if(obj->firstTime) {
        glUniformMatrix4fv(glGetUniformLocation(obj->shader, "previous"), 1, GL_FALSE, &transform[0][0]);
    } else {
        glUniformMatrix4fv(glGetUniformLocation(obj->shader, "previous"), 1, GL_FALSE, &obj->prev[0][0]);
    }
    obj->prev = transform;
    obj->firstTime = false;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj->texture);
    glBindVertexArray(obj->vao);
    glDrawElements(GL_TRIANGLES, obj->po->mesh.num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main() {
    initializeGLFW();
    GLFWwindow* window = createWindow(screenwidth, screenheight, "Takeoff Sendario");
    glfwSetWindowSizeCallback(window, reshape);
    initializeGLEW();

    initializeFramebuffer();
    resizeFramebuffer(screenwidth, screenheight);
    ppshader = mkShader("pp_motionblur");
    glUseProgram(ppshader);
    glUniform1i(glGetUniformLocation(ppshader, "screenTexture"), 0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    shaders["box"] = mkShader("box");
    textures["isqswjwki55a1.png"] = loadTexture("textures/isqswjwki55a1.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures["isqswjwki55a1.png"]);
    glUniform1i(glGetUniformLocation(shaders["box"], "tex"), 0);


    std::vector<GameObject> objs;
    objs.push_back(GameObject());
    GameObject *spinningCube = &objs[0];

    spinningCube->ro.push_back(RenderObject(new PhysicsObject(dMesh::createBox(glm::dvec3(0.0, 0.0, 0.0), 1.0, 1.0, 1.0), NULL)));
    spinningCube->ro.push_back(RenderObject(new PhysicsObject(dMesh::createBox(glm::dvec3(1.2, 0.0, 0.0), 1.0, 1.0, 1.0), NULL)));
    spinningCube->ro.push_back(RenderObject(new PhysicsObject(dMesh::createBox(glm::dvec3(-1.2, 0.0, 0.0), 1.0, 1.0, 1.0), NULL)));

    for(int i = 0; i < spinningCube->ro.size(); i++) {
        RenderObject *obj = &spinningCube->ro[i];
        convertMeshToOpenGLBuffers(obj);
        obj->shader = shaders["box"];
        obj->texture = textures["isqswjwki55a1.png"];
    }


    // Main loop
    while (!glfwWindowShouldClose(window)) {
        prevFrameTime = now();
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(int i = 0; i < objs.size(); i++) {
            for(int j = 0; j < objs[i].ro.size(); j++) {
                render(&objs[i].ro[j]);
            }
        }

        // Bind back to the default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the default framebuffer

        glUseProgram(ppshader);
        glActiveTexture(GL_TEXTURE0); // Activate the first texture unit for the color texture
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glUniform1i(glGetUniformLocation(ppshader, "screenTexture"), 0); // Pass texture unit 0 to the shader

        glActiveTexture(GL_TEXTURE1); // Activate the second texture unit for the velocity texture
        glBindTexture(GL_TEXTURE_2D, velocityTex);
        glUniform1i(glGetUniformLocation(ppshader, "velocityTexture"), 1); // Pass texture unit 1 to the shader

        glUniform1i(glGetUniformLocation(ppshader, "mode"), motion_blur_mode);
        glUniform1f(glGetUniformLocation(ppshader, "inv_strength"), motion_blur_invstr);
        glUniform1f(glGetUniformLocation(ppshader, "upscaling_factor"), upscaling_factor);

        // render the color+velocity buffer to the screen buffer with a quad and apply post-processing
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind velocity texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to the default framebuffer

        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now() - prevFrameTime).count();
        glfwSwapBuffers(window);

//        usleep(40000);


        if(++frames_rendered % 40 == 0){
            std::cout << frameDuration / 1000.0 << " ms (theoretically " << 1000000.0 / frameDuration <<" fps) \n";
        }
        if(frameDuration < 5000.0){
			usleep(5000.0 - frameDuration);
        }

        glfwPollEvents();
    }

    for(int i = 0; i < objs.size(); i++) {
        for(int j = 0; j < objs[i].ro.size(); j++) {
            objs[i].ro[j].po->mesh.destroy();
        }
    }

    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorTex);
    glDeleteTextures(1, &velocityTex);

    glfwTerminate();
    return 0;
}


