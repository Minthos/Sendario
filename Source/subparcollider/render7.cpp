#include <GL/glew.h>
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

    GLuint vert;// = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag;// = compileShader(GL_FRAGMENT_SHADER, frag_src);
    free(vert_src);
    free(frag_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    checkShader(GL_LINK_STATUS, program);
    return program;
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
        // TODO: also convert triangle normals
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void setupRenderer(const dMesh& mesh) {
    shaderProgram = mkShader("box");

    convertMeshToOpenGLBuffers(mesh, vao, vbo, ebo);
}


void render(const dMesh& mesh) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(4,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main() {
    dMesh m = dMesh::createBox(dvec3(0), 1.0, 1.0, 1.0);
    setupRenderer(m);
    while(true){
        render(m);
    }
}


