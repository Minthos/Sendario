#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>

auto now = std::chrono::high_resolution_clock::now;

int winW = 800;
int winH = 600;
unsigned int frameCount = 0;
auto startTime = now();
auto tZero = now();

GLuint computeProgram;
GLuint quadProgram;

GLuint outputTexture;
GLuint quadVAO;

GLuint sphereBuffer;
GLuint lightBuffer;


struct Sphere {
	glm::vec3 center;
	float radius;
};

struct Light {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	int id;
};

const int MAXDEPTH = 5;
float maxDepth = 3;
int numSpheres = 3;
Sphere* spheres = (Sphere*)malloc(sizeof(Sphere) * numSpheres);
int numLights = 6;
Light* lights = (Light*)malloc(sizeof(Light) * numLights);

const int WORKGROUP_SIZE = 8;

const GLchar* computeShaderSrc = R"(
#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, location = 0) writeonly uniform image2D destTex;
uniform vec2 screenSize;
uniform int numSpheres;
uniform int numLights;
uniform int maxDepth;

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct Sphere {
	vec3 center;
	float radius;
};

struct Light {
	vec3 position;
	float radius;
	vec3 color;
	int id;
};

layout(std430, binding = 0) buffer SphereBuffer {
	Sphere spheres[];
};

layout(std430, binding = 1) buffer LightBuffer {
	Light lights[];
};

float raySphere(Ray ray, vec3 center, float radius) {
	vec3 oc = ray.origin - center;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - radius * radius;
	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0.0) {
		return -1.0;
	} else {
		return (b + sqrt(discriminant)) / (-2.0 * a);
	}
}

float lerp(float a, float b, float f) {
	return a + f * (b - a);
}

vec3 reflect(vec3 L, vec3 N) {
    return L - 2.0 * dot(L, N) * N;
}

vec3 refract(vec3 L, vec3 N, float n1, float n2) {
    float r = n1 / n2;
    float cosI = -dot(N, L);
    float sinT2 = r * r * (1.0 - cosI * cosI);
    if (sinT2 > 1.0) return vec3(0.0);  // Total internal reflection
    float cosT = sqrt(1.0 - sinT2);
    return r * L + (r * cosI - cosT) * N;
}


void main() {
	vec2 uv;
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
	uv.x = (2.0 * float(storePos.x) / screenSize.x - 1.0);
	uv.y = (2.0 * float(storePos.y) / screenSize.y - 1.0);
	float aspectRatio = screenSize.x / screenSize.y;

	const int MAXDEPTH = 5;
	Ray ray[1 << MAXDEPTH +1];
	int ridx = 0;
	ray[ridx].origin = vec3(0);
	ray[ridx].direction = normalize(vec3(aspectRatio * uv.x, uv.y, -1));
	vec4 color = vec4(0, 0, 0, 0); // transparent for miss

		float invdepth = 2.0; //(2.0 / 1.0 + maxDepth));
		for (int depth = 0; depth <= maxDepth; depth++) {
			invdepth *= 0.5;

			int dd = 1 << depth;
			int ddd = dd << 1;
			for (int ridx = dd - 1; (ridx + 1) < ddd; ridx++) {
				ray[ridx * 2 + 1].direction = vec3(0);
				ray[ridx * 2 + 2].direction = vec3(0);
				if(length(ray[ridx].direction) < 0.1) {
					continue;
				}
				float closestHit = 1000000000000.0;
				for (int i = 0; i < numLights; i++) {
					float t = raySphere(ray[ridx], lights[i].position, lights[i].radius);
					if (t > 0) {
						color += vec4(lights[i].color, 1) * invdepth;
						closestHit = t;
						break;
					}
				}
				for (int i = 0; i < numSpheres; i++) {
					float t = raySphere(ray[ridx], spheres[i].center, spheres[i].radius);
					if (t > 0 && t < closestHit) {
						closestHit = t;
						if (depth == 0) {
							color = vec4(0, 0, 0, 0);
						}
						vec3 hitPoint = ray[ridx].origin + t * ray[ridx].direction;
						vec3 normal = (hitPoint - spheres[i].center) / spheres[i].radius;
						vec3 origin = hitPoint + 0.0001 * normal;
						for (int l = 0; l < numLights; l++) {
							Ray shadowRay;
							shadowRay.direction = lights[l].position - hitPoint;
							float lightDist = length(shadowRay.direction);
							shadowRay.direction = shadowRay.direction / lightDist;
							shadowRay.origin = origin;
							bool inShadow = false;
							for (int s = 0; s < numSpheres; s++) {
								float shadow_t = raySphere(shadowRay, spheres[s].center, spheres[s].radius);
								if (shadow_t > 0.0) {
									inShadow = true;
									break;
								}
							}
							if (!inShadow) {
								float mattitude = 0.6;
								float gloss = 32.0;
								vec3 h = (-shadowRay.direction + ray[ridx].direction) * -0.5;
								vec3 halfDir = normalize(-shadowRay.direction + ray[ridx].direction);
								float specAngle = max(0.0, dot(halfDir, -normal));
								float specular = pow(specAngle, gloss);
								float lambertian = max(0.0, dot(normal, shadowRay.direction));
								float contribution = lerp(specular, lambertian, mattitude);
								color += vec4((contribution / lightDist) * lights[l].color, 0.0) * invdepth;
								//float fresnel = max(0, min(1, bias + scale * (1.0 + dot(ray[ridx].direction, normal))));
							}
						}
						Ray reflectionRay;
						reflectionRay.origin = origin;
						reflectionRay.direction = reflect(ray[ridx].direction, normal);
						float refractiveIndexAir = 1.0;
						float refractiveIndexSphere = 1.5;
						vec3 refractedDirection = refract(ray[ridx].direction, normal, refractiveIndexAir, refractiveIndexSphere);
						ray[ridx * 2 + 1] = reflectionRay;
						ray[ridx * 2 + 2].origin = origin;
						ray[ridx * 2 + 2].direction = refractedDirection;
						break;
					}
				}
			}
		}

	if(color.r > 1.0) {
		color.g += (color.r - 1.0) * 0.5;
		color.b += (color.r - 1.0) * 0.5;
		color.r = 1.0;
	}
	if(color.g > 1.0) {
		color.r += (color.g - 1.0) * 0.5;
		color.b += (color.g - 1.0) * 0.5;
		color.g = 1.0;
	}
	if(color.b > 1.0) {
		color.r += (color.b - 1.0) * 0.5;
		color.g += (color.b - 1.0) * 0.5;
		color.b = 1.0;
	}
	imageStore(destTex, storePos, color);
}
)";

const GLchar* vertexShaderSrc = R"(
	#version 430
	layout(location = 0) in vec2 position;
	out vec2 TexCoords;
	void main() {
		TexCoords = position * 0.5 + 0.5;
		gl_Position = vec4(position, 0.0f, 1.0f);
	}
)";

const GLchar* fragmentShaderSrc = R"(
	#version 430
	in vec2 TexCoords;
	out vec4 color;
	uniform sampler2D outputTexture;
	void main() {
		color = texture(outputTexture, TexCoords);
	}
)";

bool checkShaderCompilation(GLuint shader) {
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
		exit(-1);
		return false;
	}
	return true;
}

bool checkProgramLinking(GLuint program) {
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cerr << "Program Linking Error: " << infoLog << std::endl;
		exit(-1);
		return false;
	}
	return true;
}

void initComputeShader() {
	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &computeShaderSrc, NULL);
	glCompileShader(shader);

	if (!checkShaderCompilation(shader)) {
		return;
	}

	computeProgram = glCreateProgram();
	glAttachShader(computeProgram, shader);
	glLinkProgram(computeProgram);

	if (!checkProgramLinking(computeProgram)) {
		return;
	}

	glDeleteShader(shader);
}

void initQuadShader() {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
	glCompileShader(vertexShader);

	if (!checkShaderCompilation(vertexShader)) {
		return;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(fragmentShader);

	if (!checkShaderCompilation(fragmentShader)) {
		return;
	}

	quadProgram = glCreateProgram();
	glAttachShader(quadProgram, vertexShader);
	glAttachShader(quadProgram, fragmentShader);
	glLinkProgram(quadProgram);

	if (!checkProgramLinking(quadProgram)) {
		return;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void updateSpheres() {
	for (int i = 0; i < numSpheres; i++) {
		spheres[i].center = glm::vec3(2.2f * i - 2.2f, 0.0f, -3.0f);
		spheres[i].radius = 1.0f;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * numSpheres, spheres, GL_STATIC_DRAW);
}

void updateLights() {
	for (int i = 0; i < numLights; i++) {
		lights[i].id = i;
		lights[i].radius = 0.05f;
		auto time = (now() - tZero).count() / 1000000000.0;
		lights[i].position = glm::vec3(2.2f * (i % 3) * sin(time * i), 1.9f * sin(time + i), -2.5f + 1.5 * cos(time + i));
	}
	lights[0].color = glm::vec3(0.3f, 0.3f, 1.0f);
	lights[1].color = glm::vec3(0.3f, 1.0f, 0.3f);
	lights[2].color = glm::vec3(0.3f, 0.3f, 1.0f);
	lights[3].color = glm::vec3(1.0f, 0.3f, 0.3f);
	lights[4].color = glm::vec3(0.3f, 1.0f, 0.3f);
	lights[5].color = glm::vec3(1.0f, 0.3f, 0.3f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Light) * numLights, lights, GL_STATIC_DRAW);
}

void initGL() {
	initComputeShader();
	initQuadShader();

	glGenBuffers(1, &sphereBuffer);
	updateSpheres();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereBuffer);

	glGenBuffers(1, &lightBuffer);
	updateLights();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightBuffer);

	glGenTextures(1, &outputTexture);
	glBindTexture(GL_TEXTURE_2D, outputTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, winW, winH, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	GLfloat quadVertices[] = {
		-1.0f,  1.0f, 
		-1.0f, -1.0f, 
		 1.0f, -1.0f, 
		 1.0f,  1.0f
	};
	glGenVertexArrays(1, &quadVAO);
	GLuint quadVBO;
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL Error: " << error << std::endl;
	}
}

void resizeTexture(GLuint &texture, int width, int height) {
	if (texture != 0) {
		glDeleteTextures(1, &texture);
	}
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void chkerr() {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL Error: " << error << std::endl;
	}
}

void display() {
	chkerr();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(computeProgram);
	chkerr();
	
	glUniform1i(glGetUniformLocation(computeProgram, "maxDepth"), (int)maxDepth);
	glUniform1i(glGetUniformLocation(computeProgram, "numSpheres"), numSpheres);
	glUniform1i(glGetUniformLocation(computeProgram, "numLights"), numLights);
	glUniform2f(glGetUniformLocation(computeProgram, "screenSize"), (float)winW, (float)winH);
	glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	chkerr();
	
	// hook into data source here
	updateSpheres();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereBuffer);
	chkerr();
	updateLights();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightBuffer);
	chkerr();
	
	// TRACE ALL THE RAYS!
	glDispatchCompute(winW / WORKGROUP_SIZE, winH / WORKGROUP_SIZE, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	chkerr();

	glUseProgram(quadProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outputTexture);
	chkerr();
	glUniform1i(glGetUniformLocation(quadProgram, "outputTexture"), 0);
	glBindVertexArray(quadVAO);
	// draw the screen
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
	glutSwapBuffers();
	chkerr();
	frameCount++;
}

void reshape(int width, int height) {
	winW = width;
	winH = height;
	glViewport(0, 0, width, height);
	resizeTexture(outputTexture, width, height);
}


double fps_limit = 120.0;
double idleTime = 0.0;
double busyTime = 0.0;
auto prevFrameTime = now();

void idle() {
	auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now() - prevFrameTime).count();
	busyTime += frameDuration;
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now() - startTime).count();
	float fps = frameCount / (elapsed / 1000000.0);
	if (elapsed >= 1000000) {
		
		std::cout << "FPS: " << fps << " Frame Time: " << busyTime/(1000.0 * frameCount) << " ms " << "CPU Busy: " << (1.0 - (idleTime / elapsed)) * 100.0f << "%" << " maxDepth: " << maxDepth << std::endl;
		idleTime = 0.0;
		busyTime = 0.0;
		frameCount = 0;
		startTime = now();
	}
	float frameTimeLimit = 1000000.0f / fps_limit;
	if (frameDuration < frameTimeLimit) {
		auto timeBeforeSleep = now();
		usleep(frameTimeLimit - frameDuration);
		idleTime += std::chrono::duration_cast<std::chrono::microseconds>(now() - timeBeforeSleep).count();
	}
	if (maxDepth > 0.5 && fps < fps_limit * 0.5) {
		maxDepth -= 0.1;
	} else if((maxDepth + 0.1 < MAXDEPTH) && ((idleTime / elapsed) > 0.98)) {
		maxDepth += 0.1;
	}
	prevFrameTime = now();
	glutPostRedisplay();
}


void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
#ifdef DEBUG
	std::cerr << (type == GL_DEBUG_TYPE_ERROR ? "GL ERROR " : "") << message << std::endl;
#else
	if(type == GL_DEBUG_TYPE_ERROR) {
		std::cerr << (type == GL_DEBUG_TYPE_ERROR ? "GL ERROR " : "") << message << std::endl;
	}
#endif
}

int main(int argc, char** argv) {
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(winW, winH);
	glutCreateWindow("Raycasting with Compute Shader");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	initGL();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	// render a frame manually since otherwise it would only render after a window resize event
	display();
	glutMainLoop();
	free(spheres);
	return 0;
}

