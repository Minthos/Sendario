#include "terragen.h"
#include "physics.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
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
int antialiasing = 2; // 1 (no upscaling) and 2 (4 samples per pixel) are good values
int motion_blur_mode = 1; // 0 = off, 1 = nonlinear (sharp), 2 = linear (blurry)
float motion_blur_invstr = 5.0f; // motion blur amount. 1.0 = very high. 5.0 = low.

Unit *player_character = nullptr;

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

void checkShader(GLenum status_enum, GLuint shader, const char* name) {
    GLint success = 1;
    GLchar infoLog[512] = {0};
    glGetShaderiv(shader, status_enum, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation error: %s\n%s\n", name, infoLog);
    }
}

GLuint compileShader(GLenum shaderType, const char* source, const char* name) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    checkShader(GL_COMPILE_STATUS, shader, name);
    return shader;
}

GLuint mkShader(string name) {
    string vert_path = name + "vert.glsl";
    string frag_path = name + "frag.glsl";
    char *vert_src = readShaderSource(vert_path.c_str());
    char *frag_src = readShaderSource(frag_path.c_str());

    GLuint vert = compileShader(GL_VERTEX_SHADER, vert_src, vert_path.c_str());
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, frag_src, frag_path.c_str());
    free(vert_src);
    free(frag_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    checkShader(GL_LINK_STATUS, program, name.c_str());
    return program;
}

GLuint loadTexture(const char* filename, bool smooth) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture " << filename << std::endl;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    std::cout << "loaded texture " << filename << " with " << channels << " channels\n";
    GLint format = channels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return texture;
}

struct texvert {
    glm::vec3 xyz;
    glm::vec3 uv;

    texvert(glm::vec3 a, glm::vec3 b) { xyz = a; uv = b; }
};

struct RenderObject {
    PhysicsObject *po;
    GLuint shader;
    GLuint texture;
    GLuint vao, vbo, ebo;
    glm::mat4 prev;
    bool firstTime;

    RenderObject(PhysicsObject *ppo) { po = ppo; firstTime = true; }
    ~RenderObject() {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }
};

// uploads a mesh composed of one or more box meshes to the gpu
// TODO: find a better way to set texture coordinates.
void upload_boxen_mesh(RenderObject *obj) {
    glGenVertexArrays(1, &obj->vao);
    glBindVertexArray(obj->vao);

    std::vector<texvert> vertices;
    std::vector<GLuint> indices;

    // this ordering reverses the texture on two sides so a texture tiles "correctly" horizontally
    uint32_t faces[6 * 4] =
        {0, 1, 2, 3, // top
        0, 1, 4, 5, // front
        0, 2, 4, 6, // left
        3, 1, 7, 5, // right
        3, 2, 7, 6, // back
        5, 4, 7, 6}; // bottom
    glm::vec3 texture_corners[4] = {
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f)};
    for (uint32_t i = 0; i < obj->po->mesh.num_tris / 2; ++i) {
        dTri* t1 = &obj->po->mesh.tris[i * 2];
        dTri* t2 = &obj->po->mesh.tris[i * 2 + 1];
        texvert verts[4] = {
            texvert(vec3(obj->po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4    ]]), texture_corners[0]),
            texvert(vec3(obj->po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 1]]), texture_corners[1]),
            texvert(vec3(obj->po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 2]]), texture_corners[2]),
            texvert(vec3(obj->po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 3]]), texture_corners[3])};
        vertices.insert(vertices.end(), {verts[0], verts[1], verts[2], verts[3]});
        // correct the winding order so we can use backface culling
        bool ccw = ((i % 6 == 2) || (i % 6 == 3) || (i % 6 == 5) || (i % 6 == 0));
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void upload_terrain_mesh(RenderObject *obj, Celestial *celestial) {
    glGenVertexArrays(1, &obj->vao);
    glBindVertexArray(obj->vao);
    std::vector<texvert> vertices;
    std::vector<GLuint> indices;
    for (uint32_t i = 0; i < obj->po->mesh.num_tris; ++i) {
        dTri* t = &obj->po->mesh.tris[i];
        indices.insert(indices.end(), {t->verts[0], t->verts[1], t->verts[2]});
        glm::vec3 floatverts[3] = {
            glm::vec3(obj->po->mesh.verts[ t->verts[0] ]),
            glm::vec3(obj->po->mesh.verts[ t->verts[1] ]),
            glm::vec3(obj->po->mesh.verts[ t->verts[2] ])};
        glm::vec3 normal = glm::normalize(glm::cross(floatverts[1] - floatverts[0], floatverts[2] - floatverts[0]));
        float inclination = glm::angle(normal, glm::vec3(t->normal));
        float insolation = glm::dot(normal, glm::vec3(0.4, 0.4, 0.4));
        for(int j = 0; j < 3; j++){
            vertices.insert(vertices.end(), {floatverts[j], glm::vec3(inclination, insolation, t->elevations[j])});
        }
    }
    glGenBuffers(1, &obj->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(texvert), vertices.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &obj->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
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
int framerate_handicap = 1;
//int screenheight = 80;
//int screenwidth = 120;
//int framerate_handicap = 10000;
int frames_rendered = 0;
auto prevFrameTime = now();
bool game_paused = false;

glm::vec3 camera_target = vec3(0,0,0);
glm::quat camera_rot;
double camera_initial_x;
double camera_initial_y;
double camera_zoom = 3.0;
bool camera_dirty = true;

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
    int width = w * antialiasing;
    int height = h * antialiasing;
    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, velocityTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
    GLuint depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);
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
    std::cout << "Resized window to " << height << "x" << width << "\n";
}

void render(RenderObject *obj) {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), obj->po->zoneSpacePosition());
    glm::mat4 rotation = glm::mat4(glm::quat(obj->po->rot));
    glm::mat4 view = glm::lookAt(camera_target,
            camera_target + (camera_rot * glm::vec3(0,0,1) * glm::max(1.0f, (float)camera_zoom)),
             glm::vec3(0,1,0));


//            - (camera_rot * vec3(1.0f, 0.0f, 0.0f)) * glm::max(1.0f, (float)camera_zoom),
//            camera_target, glm::vec3(0,1,0));
//    view = glm::mat4(camera_rot) * view;

    glm::mat4 camera_offset = glm::translate(glm::mat4(1.0f), (glm::quat(player_character->body.rot) * vec3(0.0f, 0.0f, -1.0f)) * glm::max(1.0f, (float)camera_zoom));
    view = glm::mat4(camera_rot) * camera_offset * glm::translate(glm::mat4(1.0f), -camera_target);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f * glm::min(1.0f, (float)camera_zoom)),
            (float)screenwidth / (float)screenheight, 0.001f, 1e38f);
    glm::mat4 transform = projection * view * translation * rotation;

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

dvec3 input_vector(GLFWwindow* window) {
    dvec3 vel(0.0);
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        vel.z = -1.0;
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        vel.z = 1.0;
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        vel.x = -1.0;
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        vel.x = 1.0;
    }   
    return vel;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS){
        switch(key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case GLFW_KEY_SPACE:
                game_paused = !game_paused;
                break;
        }
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(!((camera_initial_x == 0 && camera_initial_y == 0) ||
                abs(xpos - camera_initial_x) > 200.0 || abs(ypos - camera_initial_y) > 200.0)){
        camera_rot = glm::angleAxis((float)(xpos - camera_initial_x) / 1000.0f, glm::vec3(0.0, 1.0, 0.0)) * camera_rot;
        camera_rot = glm::angleAxis((float)(ypos - camera_initial_y) / 1000.0f, glm::vec3(1.0, 0.0, 0.0)) * camera_rot;
        camera_rot = glm::normalize(camera_rot);
    }
    camera_initial_x = xpos;
    camera_initial_y = ypos;
    camera_dirty = true;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera_zoom *= 1.0 + (-0.3 * yoffset);
    camera_dirty = true;
}

int main(int argc, char** argv) {
    int seed = 52;
    int lod = 40;
    if(argc > 2){
        seed = atol(argv[1]);
        lod = atol(argv[2]);
    } else {
        std::cout << "\n\nPlease specify prng seed and LOD distance\n";
        std::cout << "Usage: " << argv[0] << " seed LOD\n";
        std::cout << "Using default values " << seed << " and " << lod << "\n\n\n";
    }

    // seed, name, radius, roughness, parent body
    Celestial glitch(seed, (double)lod, "Glitch", 6.371e6, 0.2, NULL);
    glitch.orbiting_bodies.emplace_back(seed + 1, (double)lod, "Jank", 1e6, 0.2, &glitch);
    glitch.orbiting_bodies.emplace_back(seed + 2, (double)lod, "Kludge", 1e6, 0.2, &glitch);
    glitch.orbiting_bodies.emplace_back(seed + 3, (double)lod, "Artifact", 1e6, 0.2, &glitch);
    glitch.orbiting_bodies.emplace_back(seed + 4, (double)lod, "Haxx", 1e6, 0.2, &glitch);

    initializeGLFW();
    GLFWwindow* window = createWindow(screenwidth, screenheight, "Takeoff Sendario");
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwSetWindowSizeCallback(window, reshape);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    camera_rot = glm::quat(1, 0, 0, 0);
    initializeGLEW();

    initializeFramebuffer();
    resizeFramebuffer(screenwidth, screenheight);
    ppshader = mkShader("pp_motionblur");
    glUseProgram(ppshader);
    glUniform1i(glGetUniformLocation(ppshader, "screenTexture"), 0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    shaders["box"] = mkShader("box");
    shaders["terrain"] = mkShader("terrain");
    textures["isqswjwki55a1.png"] = loadTexture("textures/isqswjwki55a1.png", true);
    textures["green_transparent_wireframe_box_64x64.png"] = loadTexture("textures/green_transparent_wireframe_box_64x64.png", false);
    textures["tree00.png"] = loadTexture("textures/tree00.png", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures["isqswjwki55a1.png"]);
    glUniform1i(glGetUniformLocation(shaders["box"], "tex"), 0);

    nonstd::vector<Unit> units;
    nonstd::vector<RenderObject> ros;

    units.emplace_back(); // 1
    player_character = &units[0]; // 2
    player_character->addComponent(dMesh::createBox(glm::dvec3(0.0, 0.0, 0.0), 1.0, 1.0, 1.0)); // 3
    player_character->addComponent(dMesh::createBox(glm::dvec3(1.2, 0.0, 0.0), 1.0, 1.0, 0.01));
    player_character->addComponent(dMesh::createBox(glm::dvec3(-1.2, 0.0, 0.0), 1.0, 0.05, 1.0));
    player_character->bake(); // 4

    ros.emplace_back(&player_character->body); // 5
    ros.emplace_back(&glitch.body); // 5
    player_character->body.ro = &ros[0]; // 6
    glitch.body.ro = &ros[1]; // 6

    upload_boxen_mesh(player_character->body.ro);
    player_character->body.ro->shader = shaders["box"];
    player_character->body.ro->texture = textures["isqswjwki55a1.png"];

    upload_terrain_mesh(glitch.body.ro, &glitch);
    glitch.body.ro->shader = shaders["terrain"];
    glitch.body.ro->texture = textures["isqswjwki55a1.png"];

    ttnode* zone_origo = glitch.terrain[0x2aaaaaaaa8];

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        if(!game_paused){

            if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
                camera_rot = glm::angleAxis(-0.01f, glm::vec3(0.0, 0.0, 1.0)) * camera_rot;
            } if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
                camera_rot = glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)) * camera_rot;                              
            }

            player_character->body.rot = glm::conjugate(camera_rot * glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0, 1.0, 0.0)));
            //ttnode* northpole = glitch.terrain[0x2aaaaaaaa8];

            double dt = 0.008;
            player_character->body.pos += player_character->body.rot * input_vector(window) * dt * 10.0;
            ttnode* tile = glitch.terrain[player_character->body.pos + zone_origo->verts[0]];

            player_character->body.pos.y = 1.0 + tile->elevation_projected(player_character->body.pos, &glitch.body.mesh);

            // optimization: compute view matrix here instead of in render()
            camera_target = vec3(player_character->body.pos);

            //glitch.body.rot = glm::normalize(glm::angleAxis(0.0004, glm::dvec3(0.0, 0.0, 0.0)) * glitch.body.rot);
        }

        if(game_paused && !camera_dirty){
            usleep(8000.0);
            glfwPollEvents();
            continue;
        }
        camera_dirty = false;

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//        ground->body.rot = glm::angleAxis(0.01, glm::dvec3(0.0, 1.0, 0.0)) * ground->body.rot;
        render(glitch.body.ro);
        ctleaf l = ctleaf(&player_character->body);
        CollisionTree t = CollisionTree(dvec3(0.0), &l, 1);

        nonstd::vector<ctnode*> stack;
        stack.push_back(t.root);
        while(stack.size()){
            ctnode* node = stack[stack.size() - 1];
            stack.pop_back();
            if(node->count > 1) {
                stack.push_back(&t.root[node->left_child]);
                stack.push_back(&t.root[node->left_child + 1]);
            } else {
                ctleaf *leaf = &t.leaves[node->first_leaf];
                render(leaf->object->ro);
              
#ifdef DEBUG
                dvec3 hi = node->hi.todvec3();
                dvec3 lo = node->lo.todvec3();
                dvec3 center = (hi + lo) * 0.5;
                dvec3 size = (hi - lo);
                PhysicsObject greencube = PhysicsObject(dMesh::createBox(center, size.x, size.y, size.z), NULL);
                RenderObject ro = RenderObject(&greencube);
                upload_boxen_mesh(&ro);
                ro.shader = shaders["box"];
                ro.texture = textures["green_transparent_wireframe_box_64x64.png"];
                glDisable(GL_CULL_FACE);
                render(&ro);
                glEnable(GL_CULL_FACE);
#endif
            }
        }

        t.destroy();

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
        glUniform1f(glGetUniformLocation(ppshader, "antialiasing"), antialiasing);

        // render the color+velocity buffer to the screen buffer with a quad and apply post-processing
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind velocity texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to the default framebuffer

        ++frames_rendered;
        if(frames_rendered % framerate_handicap == 0) {
            glfwSwapBuffers(window);
            glfwPollEvents();

            auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now() - prevFrameTime).count();
            if(frames_rendered % (240 * framerate_handicap) == 0){
                std::cout << frameDuration / 1000.0 << " ms (" << 1000000.0 / frameDuration <<" fps)";
                if(framerate_handicap > 1){
                    std::cout << " " << framerate_handicap * (1000000.0 / frameDuration) << " theoretically";
                }
                std::cout << "\n";
            }
            if(frameDuration < 1000.0){ // limit the game to 1000 fps if the system/libraries don't limit it for us
    			usleep(1000.0 - frameDuration);
            }
            prevFrameTime = now();
        }

//        usleep(40000);


    }

    for(int i = 0; i < ros.size(); i++) {
        ros[i].po->mesh.destroy();
    }

    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorTex);
    glDeleteTextures(1, &velocityTex);

    glfwTerminate();
    return 0;
}


