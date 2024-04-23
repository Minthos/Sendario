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
#include <thread>

#ifndef __clang__
#define BOOST_STACKTRACE_USE_BACKTRACE
#include <boost/stacktrace.hpp>
#endif

#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>

// Rendering quality settings
float anisotropy = 16.0f; // should be 4 with no upscaling, 16 with upscaling
int antialiasing = 2; // 1 (no upscaling) and 2 (4 samples per pixel) are good values
int motion_blur_mode = 1; // 0 = off, 1 = nonlinear (sharp), 2 = linear (blurry)
float motion_blur_invstr = 5.0f; // motion blur amount. 1.0 = very high. 5.0 = low.
int gpu_transfer_batch_size = 100000;

GLFWwindow* window = nil;
Unit *player_character = nil;

using std::string;

std::unordered_map<string, GLuint> shaders;
std::unordered_map<string, GLuint> textures;

void print_backtrace(int sig, siginfo_t *info, void *secret) {
#ifndef __clang__
	boost::stacktrace::stacktrace st;
    for (std::size_t i = 0; i < st.size(); ++i) {
		std::stringstream ss;
		ss << st[i];
		std::string s = ss.str();
    	size_t pos = s.find(':');
		if(pos != std::string::npos){
			// subtract 2 from the line number because apparently that's necessary? ahh the joys of programming
			std::cout << i << "# " << s.substr(0, pos) << " " << std::atoi(s.c_str() + pos + 1) - 2 << std::endl;
		} else {
			std::cout << i << "# " << s << std::endl;
		}
    }
#endif
}

char* readShaderSource(const char* filePath) {
    FILE* file = fopen(filePath, "rb");
    if (file == nil) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 1);
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

void checkGLerror() {
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR) { 
        std::cout << "OpenGL Error: ";
        switch (err) {
            case GL_INVALID_ENUM: std::cout << "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: std::cout << "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: std::cout << "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW: std::cout << "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: std::cout << "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY: std::cout << "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default: std::cout << "Unknown Error"; break;
        }
        std::cout << std::endl;
        assert(0);
    }
}

void checkShader(GLenum status_enum, GLuint shader, const char* name) {
    GLint success = 1;
    GLchar infoLog[512] = {0};
    glGetShaderiv(shader, status_enum, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nil, infoLog);
        fprintf(stderr, "Shader compilation error: %s\n%s\n", name, infoLog);
    }
}

void checkProgram(GLenum status_enum, GLuint program, const char* name) {
    GLint success = 1;
    GLchar infoLog[512] = {0};
    glGetProgramiv(program, status_enum, &success);
    if(!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nil, infoLog);
        std::cout << "Shader Program Validation Failed: " << infoLog << std::endl;
    }
}

GLuint compileShader(GLenum shaderType, const char* source, const char* name) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nil);
    glCompileShader(shader);
    checkShader(GL_COMPILE_STATUS, shader, name);
    return shader;
}

GLuint mkShader(string name) {
    string vert_path = name + "vert.glsl";
    string frag_path = name + "frag.glsl";
    char *vert_src = readShaderSource(vert_path.c_str());
    char *frag_src = readShaderSource(frag_path.c_str());
    checkGLerror();

    GLuint vert = compileShader(GL_VERTEX_SHADER, vert_src, vert_path.c_str());
    checkGLerror();
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, frag_src, frag_path.c_str());
    checkGLerror();
    free(vert_src);
    free(frag_src);
    GLuint program = glCreateProgram();
    checkGLerror();
    glAttachShader(program, vert);
    checkGLerror();
    glAttachShader(program, frag);
    checkGLerror();
    glLinkProgram(program);
    checkGLerror();
    checkProgram(GL_LINK_STATUS, program, name.c_str());
    checkGLerror();
    glValidateProgram(program);
    checkGLerror();
    checkProgram(GL_VALIDATE_STATUS, program, name.c_str());
    checkGLerror();
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
    if (glewIsSupported("GL_EXT_texture_filter_anisotropic") && anisotropy > 0) {
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


struct RenderObject {
    PhysicsObject *po;
    GLuint shader;
    GLuint texture;
    GLuint vao, vbo, ebo;
    void *vbo_mapped;
    void *ebo_mapped;
    glm::mat4 prev;
    bool firstTime;

    RenderObject(PhysicsObject *ppo) { po = ppo; firstTime = true; }
    ~RenderObject() {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }

    // uploads a mesh composed of one or more box meshes to the gpu
    // TODO: find a better way to set texture coordinates.
    void upload_boxen_mesh() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

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
        for (uint32_t i = 0; i < po->mesh.num_tris / 2; ++i) {
            dTri* t1 = &po->mesh.tris[i * 2];
            dTri* t2 = &po->mesh.tris[i * 2 + 1];
            texvert verts[4] = {
                texvert(vec3(po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4    ]]), VERTEX_TYPE_NONE, texture_corners[0]),
                texvert(vec3(po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 1]]), VERTEX_TYPE_NONE, texture_corners[1]),
                texvert(vec3(po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 2]]), VERTEX_TYPE_NONE, texture_corners[2]),
                texvert(vec3(po->mesh.verts[8 * (i / 6) + faces[(i % 6) * 4 + 3]]), VERTEX_TYPE_NONE, texture_corners[3])};
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
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(texvert), vertices.data(), GL_STATIC_DRAW);
        // Position attribute
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(texvert), (void*)0);
        glEnableVertexAttribArray(0);
        // Type id attribute (this doesn't work so I packed it into the w component of the position attribute as a workaround)
//        glVertexAttribIPointer(1, 1, GL_INT, sizeof(texvert), (void*)(3 * sizeof(GLfloat)));
//        glEnableVertexAttribArray(1);
        // Texture coordinate attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(texvert), (void*)(3 * sizeof(GLfloat) + sizeof(GLint)));
        glEnableVertexAttribArray(1);
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }

    void prepare_buffers_chunked(dMesh *mesh) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh->num_tris * 3 * sizeof(texvert), nil, GL_STATIC_DRAW);
        // Position attribute
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(texvert), (void*)0);
        glEnableVertexAttribArray(0);
        // Type id attribute (this doesn't work so I packed it into the w component of the position attribute as a workaround)
//        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(texvert), (void*)(3 * sizeof(GLfloat)));
//        glEnableVertexAttribArray(1);
        // Texture coordinate attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(texvert), (void*)(3 * sizeof(GLfloat) + sizeof(GLint)));
        glEnableVertexAttribArray(1);
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_tris * 3 * sizeof(GLuint), nil, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }

    // can prepare the data ahead of time to offload the main thread a little, but it's low priority. this is fast enough.
    uint32_t upload_terrain_mesh_chunked(dMesh *mesh, uint32_t progress) {
        auto begin = now();
        std::vector<texvert> vertices;
        vertices.reserve(gpu_transfer_batch_size * 3);
        std::vector<GLuint> indices;
        indices.reserve(gpu_transfer_batch_size * 3);
        uint32_t limit = min(mesh->num_tris, progress + gpu_transfer_batch_size);
        for (uint32_t i = progress; i < limit; ++i) {
            dTri* t = &mesh->tris[i];
            indices.insert(indices.end(), {t->verts[0], t->verts[1], t->verts[2]});
            glm::vec3 floatverts[3] = {
                glm::vec3(mesh->verts[ t->verts[0] ]),
                glm::vec3(mesh->verts[ t->verts[1] ]),
                glm::vec3(mesh->verts[ t->verts[2] ])};
            glm::vec3 normal = glm::normalize(glm::cross(floatverts[1] - floatverts[0], floatverts[2] - floatverts[0]));
            float inclination = glm::angle(normal, glm::vec3(t->normal));
            float insolation = glm::dot(normal, glm::vec3(0.4, 0.4, 0.4));
            if(t->type_id == VERTEX_TYPE_TERRAIN){
                for(int j = 0; j < 3; j++){
                    vertices.insert(vertices.end(), {floatverts[j], t->type_id, glm::vec3(inclination, insolation, t->elevations[j])});
                }
            } else {
                for(int j = 0; j < 3; j++){
                    vertices.insert(vertices.end(), {floatverts[j], t->type_id, glm::vec3(inclination, insolation, t->elevations[j])});
                }
            }
        }
        auto begin_upload = now();
        glBindVertexArray(vao);
        glBufferSubData(GL_ARRAY_BUFFER, progress * 3 * sizeof(texvert), vertices.size() * sizeof(texvert), vertices.data());
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, progress * 3 * sizeof(GLuint), indices.size() * sizeof(GLuint), indices.data());
        glBindVertexArray(0);
        auto end = now();
//        std::cout << "prepared mesh in: " << std::chrono::duration_cast<std::chrono::microseconds>(begin_upload - begin).count() / 1000.0 << " ms\n";
//        std::cout << "uploaded mesh in: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin_upload).count() / 1000.0 << " ms\n";
        return limit;
    }
};

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
    GLFWwindow* window = glfwCreateWindow(width, height, title, nil, nil);
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
auto prevFrameTime = now();
bool game_paused = false;
bool mouse_capture = true;
bool autorun = false;

glm::vec3 camera_target = vec3(0,0,0);
glm::quat camera_rot;
double camera_initial_x;
double camera_initial_y;
double camera_zoom = 3.0;
bool camera_dirty = true;
glm::dvec3 local_gravity_normalized;

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, velocityTex, 0);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nil);
    glBindTexture(GL_TEXTURE_2D, velocityTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16I, width, height, 0, GL_RGBA_INTEGER, GL_INT, nil);
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

    // Before drawing, validate the program
    glValidateProgram(obj->shader);
    GLint validationStatus;
    glGetProgramiv(obj->shader, GL_VALIDATE_STATUS, &validationStatus);
    if(validationStatus == GL_FALSE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(obj->shader, 512, NULL, infoLog);
        std::cout << "Shader Program Validation Failed: " << infoLog << std::endl;
    }

    glDrawElements(GL_TRIANGLES, obj->po->mesh.num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    checkGLerror();
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
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        vel *= 100.0;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS){
        vel *= 10000.0;
    }
    if(autorun){
        vel.z -= 10.0;
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
            case GLFW_KEY_GRAVE_ACCENT:
                autorun = !autorun;
                break;
        }
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}


static void mouselook_callback_gimbal_locked(GLFWwindow* window, double xpos, double ypos)
{
    if (!((camera_initial_x == 0 && camera_initial_y == 0) ||
          abs(xpos - camera_initial_x) > 200.0 || abs(ypos - camera_initial_y) > 200.0)) {

        camera_rot = camera_rot * glm::angleAxis((float)(xpos - camera_initial_x) / -1000.0f, vec3(local_gravity_normalized));
        camera_rot = glm::angleAxis((float)(ypos - camera_initial_y) / 1000.0f, glm::vec3(1.0, 0.0, 0.0)) * camera_rot;
        camera_rot = glm::normalize(camera_rot);
    }
    camera_initial_x = xpos;
    camera_initial_y = ypos;
    camera_dirty = true;
}

static void mouselook_callback(GLFWwindow* window, double xpos, double ypos)
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

RenderObject *terrain0 = nil;
RenderObject *terrain1 = nil;
Celestial *glitch = nil;
terrain_upload_status_enum terrain_upload_status = idle;
dvec3 origo;
dvec3 vantage;
ttnode* zone;
dvec3 player_global_pos;
dvec3 delta;
dMesh mesh_in_waiting;
dMesh the_old_mesh;
TerrainTree terrain_in_waiting;
TerrainTree the_old_terrain;
mutex terrain_lock;

void terrain_thread_entry(int seed, double lod) {
    double current_lod = 1.0;
    double time_taken = 500000.0;
    uint32_t last_eviction = 0;
terrain_lock.lock();
    terrain_upload_status = generating;
    glitch = new Celestial(seed, current_lod, "Glitch", 6.371e6, 0.2, nil); // initial terrain generation must block the main thread
    vantage = dvec3(0, glitch->terrain.radius, 0);
    origo = vantage;
    mesh_in_waiting = glitch->body.mesh;
    terrain_upload_status = done_generating_first_time;
terrain_lock.unlock();
    while(terrain_upload_status == done_generating_first_time) {
        usleep(1000.0);
    }
    while(terrain_upload_status != should_exit) {
        terrain_upload_status = generating;

        dvec3 estimated_vantage = zone->spheroidPosition(player_global_pos, glitch->terrain.radius);
        double lod_increase = 10000000.0 / (time_taken + 1000000.0);
        if(POTATO_MODE){
            lod_increase = 0.5;
            current_lod = min(current_lod + lod_increase, lod);
            current_lod = min(current_lod, max(1.0, (current_lod * 100) / (1.0 + glm::length(origo - estimated_vantage))));
        } else {
            current_lod = min(current_lod + lod_increase, lod);
            current_lod = min(current_lod, max(10.0, (current_lod * 1000) / (1.0 + glm::length(origo - estimated_vantage))));
        }
        if(verbose) std::cout << "generating terrain mesh with LOD " << current_lod << "\n";
        auto begin = now();

/////// the real stuff
terrain_lock.lock();
        vantage = zone->spheroidPosition(player_global_pos, glitch->terrain.radius);
terrain_lock.unlock();

        // this is just a "hardcoded heuristic" that should work fine on my computer. a more intelligent way to do this
        // would be to evict nodes based on how much free RAM the computer has.
        if((glitch->terrain.nodes.count > gpu_transfer_batch_size * lod) || (LOW_MEMORY_MODE && (frame_counter % 120 == 119))){
            last_eviction += ((frame_counter - last_eviction) / 2);
            terrain_in_waiting = glitch->terrain.omitting_copy(last_eviction);
            std::cout << "evicted " << glitch->terrain.nodes.count - terrain_in_waiting.nodes.count << " terrain nodes. " << terrain_in_waiting.nodes.count << " nodes in the new tree.\n";
        } else {
            terrain_in_waiting = glitch->terrain.copy();
        }

        terrain_in_waiting.LOD_DISTANCE_SCALE = current_lod;
        dMesh tmp = terrain_in_waiting.buildMesh(vantage, 3, &terrain_upload_status);

terrain_lock.lock();
        mesh_in_waiting = tmp;
///////
        if(terrain_upload_status == should_exit){
            return;
        }
        terrain_upload_status = done_generating;
        auto end = now();
        double time_taken = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
terrain_lock.unlock();
        if(time_taken > 10000.0 * lod) {
            // uncomment to see something broken
            //current_lod *= (1000000.0 / duration);
        }
        while(terrain_upload_status != idle) { // wait for main thread to finish shoveling
            if(terrain_upload_status == should_exit){
                return;
            }
            usleep(1000.0);
        }
        if(current_lod >= lod) {
            do{
                estimated_vantage = zone->spheroidPosition(player_global_pos, glitch->terrain.radius);
                for(double i = 0.0; i < 0.1; i += 0.01) {
                    if(terrain_upload_status == should_exit){
                        return;
                    }
                    usleep(10000.0);
                }
            } while(glm::length(origo - estimated_vantage) < 20.0);
            if(verbose) std::cout << "origo: " << str(origo) << " estimated vantage: " << str(estimated_vantage) << " estimated delta: " << str(origo - estimated_vantage) << "\n";
        }
    }
}

int main(int argc, char** argv) {
	struct sigaction sa;
	sa.sa_sigaction = print_backtrace;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);

    auto start_time = now();
    int seed = 52;
    int lod = 20;
    for(int i = 1; i < argc; i++){
        if(!strncmp(argv[i], "-v", min(2, strlen(argv[i])))){
            verbose = true;
            std::cout << "using verbose mode\n";
        }
        if(!strncmp(argv[i], "--potato", min(8, strlen(argv[i])))){
            POTATO_MODE = true;
            std::cout << "using potato mode\n";
        }
        if(!strncmp(argv[i], "--lowmem", min(8, strlen(argv[i])))){
            LOW_MEMORY_MODE = true;
            std::cout << "using low memory mode\n";
        }
        if(!strncmp(argv[i], "--nocapture", min(11, strlen(argv[i])))){
            mouse_capture = false;
            std::cout << "not capturing mouse\n";
        }
        if(!strncmp(argv[i], "seed=", min(5, strlen(argv[i])))){
            seed = atol(argv[i] + 5);
        }
        if(!strncmp(argv[i], "lod=", min(4, strlen(argv[i])))){
            lod = (double)atol(argv[i] + 4);
            assert(lod > 0);
        }
        if(!strncmp(argv[i], "bs=", min(3, strlen(argv[i])))){
            gpu_transfer_batch_size = atol(argv[i] + 3);
            assert(gpu_transfer_batch_size >= 1000);
        }
        if(!strncmp(argv[i], "aa=", min(3, strlen(argv[i])))){
            antialiasing = atol(argv[i] + 3);
            assert(antialiasing >= 1 && antialiasing <= 8);
        }
        if(!strncmp(argv[i], "af=", min(3, strlen(argv[i])))){ 
            int anisotropic_filtering = atol(argv[i] + 3);
            assert(anisotropic_filtering >= 0 && anisotropic_filtering <= 16);
            anisotropy = (float)anisotropic_filtering;
        }
        if(!strncmp(argv[i], "blur=", min(5, strlen(argv[i])))){ 
            int motion_blur = atol(argv[i] + 5);
            motion_blur_invstr = (20.0f / ((float)motion_blur + 0.1f)) - 1.0f;
            assert(motion_blur >= 0 && motion_blur <= 50);
            if(motion_blur == 0){
                motion_blur_mode = 0;
            }
            std::cout << "using motion blur " << motion_blur << " (invstr " << motion_blur_invstr << ")\n";
        }
    }
    if(argc < 2 || verbose){
        std::cout << "\n\n";
        std::cout << "Usage: " << argv[0] << " [-v] [--potato] [--lowmem] [--nocapture] [seed=n] [lod=n] [bs=n] [aa=n] [af=n] [blur=n] [blurmode=n]\n";
        std::cout << "-v: print debug information to console.\n";
        std::cout << "--potato: compatibility mode for single-core CPUs and debugging with valgrind.\n";
        std::cout << "--lowmem: conserve RAM by caching less of the procedurally generated content.\n";
        std::cout << "--nocapture: don't capture the mouse pointer.\n";
        std::cout << "seed: the random seed used to generate the world. 52 is default.\n";
        std::cout << "lod: the target level of detail for terrain rendering. 1 or higher.\n";
        std::cout << "bs: gpu transfer batch size. minimum 1000, default 100000.\n";
        std::cout << "aa: antialiasing. 1 to 8. Number of samples per pixel is the square of this number so 2 is 4x, 4 is 16x.\n";
        std::cout << "af: anisotropic filtering. 0, to 16.\n";
        std::cout << "blur: the amount of motion blur. 0 to 50.\n";
        std::cout << "\nexamples:\nlow: aa=1 af=0 blur=0 bs=10000 lod=10\n";
        std::cout << "recommended: aa=2 af=16 blur=3 lod=20\n";
        std::cout << "ultra: aa=4 af=16 blur=3 lod=50\n\n";
        if(!verbose) std::cout << "No options have been specified. Using recommended settings.\n\n";
    }
    initializeGLFW();
    window = createWindow(screenwidth, screenheight, "Takeoff Sendario");
    glfwSwapInterval(0); // disabling vsync can reveal performance issues earlier
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwSetWindowSizeCallback(window, reshape);
    glfwSetKeyCallback(window, key_callback);
    if(mouse_capture) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    //glfwSetCursorPosCallback(window, mouselook_callback);
    glfwSetCursorPosCallback(window, mouselook_callback_gimbal_locked);
    glfwSetScrollCallback(window, scroll_callback);
    camera_rot = glm::quat(1, 0, 0, 0);
    initializeGLEW();
    checkGLerror();

    std::thread terrain_thread(terrain_thread_entry, seed, lod);

    initializeFramebuffer();
    checkGLerror();
    resizeFramebuffer(screenwidth, screenheight);
    checkGLerror();
    ppshader = mkShader("pp_motionblur");
    checkGLerror();
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
//    checkGLerror();
//    glUniform1i(glGetUniformLocation(ppshader, "screenTexture"), 0);
//    glUniform1i(glGetUniformLocation(ppshader, "velocityTexture"), 1);
//    checkGLerror();
    glEnable(GL_DEPTH_TEST);
    checkGLerror();
//    glEnable(GL_CULL_FACE);
    checkGLerror();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    checkGLerror();
    glEnable(GL_BLEND);
    checkGLerror();
    shaders["box"] = mkShader("box");
    shaders["terrain"] = mkShader("terrain");
    checkGLerror();
    textures["isqswjwki55a1.png"] = loadTexture("textures/isqswjwki55a1.png", true);
    textures["green_transparent_wireframe_box_64x64.png"] = loadTexture("textures/green_transparent_wireframe_box_64x64.png", false);
    textures["tree00.png"] = loadTexture("textures/tree00.png", true);
    checkGLerror();
    glActiveTexture(GL_TEXTURE0);
    checkGLerror();
    glBindTexture(GL_TEXTURE_2D, textures["isqswjwki55a1.png"]);
    checkGLerror();
    glUniform1i(glGetUniformLocation(shaders["box"], "tex"), 0);
    checkGLerror();

    nonstd::vector<Unit> units;
    nonstd::vector<RenderObject> ros;
    units.emplace_back();
    player_character = &units[0];
    player_character->addComponent(dMesh::createBox(glm::dvec3(0.0, 0.0, 0.0), 1.0, 1.0, 1.0));
    player_character->addComponent(dMesh::createBox(glm::dvec3(1.2, 0.0, 0.0), 1.0, 1.0, 0.01));
    player_character->addComponent(dMesh::createBox(glm::dvec3(-1.2, 0.0, 0.0), 1.0, 0.05, 1.0));
    player_character->bake();
    ros.emplace_back(&player_character->body);
    player_character->body.ro = &ros[0];
    player_character->body.ro->upload_boxen_mesh();
    player_character->body.ro->shader = shaders["box"];
    player_character->body.ro->texture = textures["isqswjwki55a1.png"];


    // seed 52 destinations:
    // 0x506283d088 has a very steep mountain peak
    // 0x4b20532488 nearby has over 8000 elevation
    // 0x27c917dc88 also in that area could be a nice location for a ski resort (player global (741646, 5.94832e+06, 2.17329e+06))
    //
    // 0x5263c2aaad has a cool beach with a flat area with furrows in a checkerboard-like pattern, nice place to have a music festival
    // or maybe a resort town. I got there by glitching through the planet, which was also interesting.
    //
    // The beach at 0x519d196b69 is enormous and the area has lots of beautiful scenery

    uint32_t terrain_upload_progress = 0;
    // Main loop
    while (!glfwWindowShouldClose(window)) {
terrain_lock.lock(); // grab mutex
        if(terrain_upload_status == done_generating_first_time) {
            terrain0 = new RenderObject(&glitch->body);
            terrain0->shader = shaders["terrain"];
            terrain0->texture = textures["isqswjwki55a1.png"];
            terrain0->prepare_buffers_chunked(&glitch->body.mesh);
            terrain0->upload_terrain_mesh_chunked(&glitch->body.mesh, 0);
            glitch->body.ro = terrain0;
            player_character->body.zone = 0x2aaaaaaaa8;
            zone = glitch->terrain[0x2aaaaaaaa8];
            std::cout << "origo: " << str(origo) << " glitch->terrain.radius:" << glitch->terrain.radius << " zone->elevation(): " << zone->elevation() << "\n";
            player_global_pos = origo + zone->elevation();
            local_gravity_normalized = -normalize(origo);
            player_character->body.pos = dvec3(0, zone->elevation(), 0);
            delta = dvec3(0,0,0);
            terrain_upload_status = idle;
        }
        if(terrain_upload_status == done_generating) {
            if(terrain_upload_progress == 0){
                terrain1 = new RenderObject(&glitch->body);
                terrain1->shader = shaders["terrain"];
                terrain1->texture = textures["isqswjwki55a1.png"];
                terrain1->prepare_buffers_chunked(&mesh_in_waiting);
            }
            terrain_upload_progress = terrain1->upload_terrain_mesh_chunked(&mesh_in_waiting, terrain_upload_progress);
            if(terrain_upload_progress == mesh_in_waiting.num_tris){
                terrain_upload_status = done_uploading;
                terrain_upload_progress = 0;
            }
        }
        if(terrain_upload_status == done_uploading) {
            delete terrain0;
            terrain0 = terrain1;
            glitch->body.ro = terrain0;
            the_old_mesh.destroy();
            the_old_mesh = glitch->body.mesh;
            glitch->body.mesh = mesh_in_waiting;
            the_old_terrain.destroy();
            the_old_terrain = glitch->terrain;
            glitch->terrain = terrain_in_waiting;


            ttnode* ozone = glitch->terrain[origo]; // the old zone
            ttnode* vzone = glitch->terrain[vantage]; // the new zone
            double avgElevation = (ozone->elevations[0] + vzone->elevations[0]) / 2.0;
            delta = (vantage + glm::normalize(vantage) * avgElevation) - (origo + (glm::normalize(origo) * avgElevation));
            // delta seems to be correct now

            zone = glitch->terrain[vantage];
            local_gravity_normalized = -normalize(vantage);
            player_character->body.pos -= delta;
            player_character->body.zone = zone->path;
            origo = vantage;
            player_global_pos = origo + player_character->body.pos;

            if(verbose) {
                std::cout << "zone: " << zone->str() << "\n";
                std::cout << "player local (" << player_character->body.pos.x << ", " << player_character->body.pos.y << ", " << player_character->body.pos.z << ")\n";
                std::cout << "player global (" << player_global_pos.x << ", " << player_global_pos.y << ", " << player_global_pos.z << ")\n";
//                std::cout << "elevation: " << tile->elevation() << "\n";//_kludgehammer(player_character->body.pos, &glitch->body.mesh) << "\n";
            }

            terrain_upload_status = idle;
        }
terrain_lock.unlock(); // release mutex

//        ttnode* tile = glitch->terrain[player_global_pos];
        checkGLerror();

        if(!game_paused){
            local_gravity_normalized = -glm::normalize(player_global_pos);
            double dt = 0.008;
            if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
                camera_rot = glm::angleAxis(-0.01f, glm::vec3(0.0, 0.0, 1.0)) * camera_rot;
            } if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
                camera_rot = glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)) * camera_rot;                              
            }
            player_character->body.rot = glm::conjugate(camera_rot * glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0, 1.0, 0.0)));
            player_character->body.pos += player_character->body.rot * input_vector(window) * dt * 10.0;
            player_global_pos = origo + player_character->body.pos;

            ttnode* tile = glitch->terrain[player_global_pos];

//            double terrain_elevation = zone->elevation();
//            double terrain_elevation = tile->elevation();
//            double terrain_elevation = tile->elevation_naive(player_character->body.pos, &glitch->body.mesh);
            double altitude = tile->player_altitude(player_character->body.pos, &glitch->body.mesh, local_gravity_normalized);
//            double terrain_elevation = tile->elevation_projected(player_character->body.pos, &glitch->body.mesh, local_gravity_normalized);
//            dvec3 player_elevation_3d = player_global_pos - tile->spheroidPosition(player_global_pos, glitch->terrain.radius);
            // this player_elevation is always positive, even when terrain_elevation is negative..
//            double player_elevation = glm::length(player_elevation_3d);

//            double player_elevation = glm::length(player_global_pos) - glitch->terrain.radius;

//            player_character->body.pos += local_gravity_normalized * (player_elevation - terrain_elevation);
            player_character->body.pos += altitude * local_gravity_normalized;
            player_global_pos = origo + player_character->body.pos;

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
        setupTextures(screenwidth * antialiasing, screenheight * antialiasing); // shouldn't be necessary
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render(terrain0);

        ctleaf l = ctleaf(&player_character->body);
        CollisionTree t = CollisionTree(dvec3(0.0), &l, 1);
        checkGLerror();

        static nonstd::vector<ctnode*> stack;
        stack.push_back(t.root);
        while(stack.size()){
            ctnode* node = stack.pop_back();
            if(node->count > 1) {
                stack.push_back(&t.root[node->left_child]);
                stack.push_back(&t.root[node->left_child + 1]);
            } else {
                ctleaf *leaf = &t.leaves[node->first_leaf];
                render(leaf->object->ro);
#ifdef DEBUG
//#ifdef THIS_CAUSES_OPENGL_ERRORS
                dvec3 hi = node->hi.todvec3();
                dvec3 lo = node->lo.todvec3();
                dvec3 center = (hi + lo) * 0.5;
                dvec3 size = (hi - lo);
                // it's really fucking weird that size has a 0 component
                if( ! (size.x == 0 || size.y == 0 || size.z == 0) ){

                    PhysicsObject greencube = PhysicsObject(dMesh::createBox(center, size.x, size.y, size.z), nil);
                    RenderObject ro = RenderObject(&greencube);
        			checkGLerror();
                    ro.upload_boxen_mesh();
//        			glFlush();
                    ro.shader = shaders["box"];
                    ro.texture = textures["green_transparent_wireframe_box_64x64.png"];
        			checkGLerror();
//                    glDisable(GL_CULL_FACE);
                    render(&ro);
        			checkGLerror();
//                    glEnable(GL_CULL_FACE);
                }
#endif
            }
        }
        t.destroy();
        checkGLerror();

        // Bind back to the default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the default framebuffer, actually we don't need to clear the color buffer
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
        glUniform1i(glGetUniformLocation(ppshader, "screen_width"), screenwidth);
        glUniform1i(glGetUniformLocation(ppshader, "screen_height"), screenheight);
        // render the color+velocity buffer to the screen buffer with a quad and apply post-processing
        glBindVertexArray(quadVAO);

        glValidateProgram(ppshader);
        checkGLerror();
        checkProgram(GL_VALIDATE_STATUS, ppshader, "ppshader");

        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkGLerror();
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind velocity texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to the default framebuffer
        glBindVertexArray(0);
        glFlush();
        checkGLerror();
        ++frame_counter;
//        usleep(100000.0);
        if(frame_counter % framerate_handicap == 0) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            if(POTATO_MODE){
                usleep(1000.0);
            }
            auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now() - prevFrameTime).count();
            if(frame_counter % (240 * framerate_handicap) == 0){
                if(verbose) std::cout << frameDuration / 1000.0 << " ms (" << 1000000.0 / frameDuration <<" fps)";
                if(framerate_handicap > 1){
                    std::cout << " " << framerate_handicap * (1000000.0 / frameDuration) << " theoretically";
                }
                if(verbose) std::cout << "\n";
            }
            // limit the game to 120 fps if the system/libraries don't limit it for us
            if(frameDuration < 1000000.0 / 120.0 && !
                    (terrain_upload_status == done_generating &&
                    terrain_upload_progress + gpu_transfer_batch_size >= mesh_in_waiting.num_tris)){
                usleep(1000000.0 / 120.0 - frameDuration);
            }
            prevFrameTime = now();
        }
    }
    terrain_upload_status = should_exit;
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorTex);
    glDeleteTextures(1, &velocityTex);
    glfwTerminate();
    terrain_thread.join();
    delete the_old_terrain.generator;
    delete terrain0;
    delete glitch;
    for(int i = 0; i < ros.size(); i++) {
        ros[i].po->mesh.destroy();
    }
    std::cout << "\n" << std::chrono::duration_cast<std::chrono::seconds>(now() - start_time).count() <<
        " seconds closer to the singularity.\n";
    std::cout << "Learn C, Python, English, math and physics. Stack sats. Lift something heavy. Go for a walk.\n";
    return 0;
}

