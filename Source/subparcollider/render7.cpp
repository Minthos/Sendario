#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "collisiontree.h"


using std::string;

GLuint shaderProgram; // Assume this is setup and linked with your shaders
GLuint vao, vbo, ebo; // VAO, VBO, and EBO handlers


// Function to read shader from file
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

    buffer[length] = '\0'; // Null-terminate the string

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return texture;
}

void convertMeshToOpenGLBuffers(const dMesh& mesh, GLuint& vao, GLuint& vbo, GLuint& ebo) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    for (uint32_t i = 0; i < mesh.num_verts; ++i) {
        vertices.push_back(static_cast<GLfloat>(mesh.verts[i].x));
        vertices.push_back(static_cast<GLfloat>(mesh.verts[i].y));
        vertices.push_back(static_cast<GLfloat>(mesh.verts[i].z));
        vertices.push_back(i & 1 ? 0.0f : 1.0f);
        vertices.push_back(i & 2 ? 0.0f : 1.0f);
    }

    for (uint32_t i = 0; i < mesh.num_tris; ++i) {
        indices.insert(indices.end(), {mesh.tris[i].verts[0], mesh.tris[i].verts[1], mesh.tris[i].verts[2]});
    }

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

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

void render(const dMesh& mesh) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(4,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenwidth / (float)screenheight, 0.1f, 100.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main() {
    initializeGLFW();
    GLFWwindow* window = createWindow(screenwidth, screenheight, "Takeoff Sendario");
    initializeGLEW();
    shaderProgram = mkShader("box");

    GLuint texture = loadTexture("textures/F90hjZAbkAAeCkk.jpeg");
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


