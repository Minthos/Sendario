#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "collisiontree.h"


auto now = std::chrono::high_resolution_clock::now;
using std::string;

GLuint shaderProgram;
GLuint vao, vbo, ebo;


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
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_CLAMP_TO_EDGE);

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

void convertMeshToOpenGLBuffers(const dMesh& mesh, GLuint& vao, GLuint& vbo, GLuint& ebo) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::vector<texvert> vertices;
    std::vector<GLuint> indices;


/*
    // original orientation, texture rotates 90 degrees between each face
    uint32_t faces[6 * 4] =
        {0, 1, 2, 3, // top
        0, 1, 4, 5, // front
        0, 2, 4, 6, // left
        7, 5, 3, 1, // right
        7, 6, 3, 2, // back
        7, 6, 5, 4}; // bottom
*/

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
        dTri* t1 = &mesh.tris[i * 2];
        dTri* t2 = &mesh.tris[i * 2 + 1];

        texvert verts[4] = {
            texvert(vec3(mesh.verts[faces[i * 4    ]]), texture_corners[0]),
            texvert(vec3(mesh.verts[faces[i * 4 + 1]]), texture_corners[1]),
            texvert(vec3(mesh.verts[faces[i * 4 + 2]]), texture_corners[2]),
            texvert(vec3(mesh.verts[faces[i * 4 + 3]]), texture_corners[3])};
       
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

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(texvert), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
//    glEnableVertexAttribArray(0);

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
int screenheight = 2160;
int frames_rendered = 0;
auto prevFrameTime = now();

void render(const dMesh& mesh) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, frames_rendered++ * 0.002f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(2,1.5,1.5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenwidth / (float)screenheight, 0.1f, 100.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    if(frames_rendered % 60 == 0){
    	auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now() - prevFrameTime).count();
        prevFrameTime = now();
        std::cout << 60000000.0 / frameDuration << '\n';
    }
}

int main() {
    initializeGLFW();
    GLFWwindow* window = createWindow(screenwidth, screenheight, "Takeoff Sendario");
    initializeGLEW();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    shaderProgram = mkShader("box");

    GLuint texture = loadTexture("textures/isqswjwki55a1.png");
//    GLuint texture = loadTexture("textures/F90hjZAbkAAeCkk.jpeg");
    glActiveTexture(GL_TEXTURE0); // Activate the texture unit first
    glBindTexture(GL_TEXTURE_2D, texture); // Bind your loaded texture
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0); // Set the texture uniform

    dMesh mesh = dMesh::createBox(glm::dvec3(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
    convertMeshToOpenGLBuffers(mesh, vao, vbo, ebo);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render(mesh);

        glfwSwapBuffers(window); // Swap the front and back buffers
        glfwPollEvents(); // Poll for and process events
    }

    mesh.destroy();
    glfwTerminate();
    return 0;
}


