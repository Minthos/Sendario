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

int winW = 1920;
int winH = 1080;
float canvasScale = 1.0;
int canvasW = winW * canvasScale;
int canvasH = winH * canvasScale;
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
	glm::vec4 color;
	glm::vec4 material; // material: diffuse, specular, reflective, refractive
};

struct Light {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	int id;
};

const int MAXDEPTH = 3;
float maxDepth = MAXDEPTH;
int numSpheres = 4;
Sphere* spheres = (Sphere*)malloc(sizeof(Sphere) * numSpheres);
int numLights = 7;
Light* lights = (Light*)malloc(sizeof(Light) * numLights);

const int WORKGROUP_SIZE = 8;

const GLchar* computeShaderSrc = R"(
#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, location = 0) writeonly uniform image2D destTex;
uniform vec2 canvasSize;
uniform int numSpheres;
uniform int numLights;
uniform int maxDepth;

const int MAXDEPTH = 3;

struct Ray {
	vec3 origin;
	int inside;
	vec3 direction;
	vec4 alpha;
};

struct Sphere {
	vec3 center;
	float radius;
	vec4 color;
	vec4 material; // material: diffuse, specular, reflective, refractive
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

vec3 reflect(vec3 L, vec3 N) {
	return L - 2.0 * dot(L, N) * N;
}

vec3 refract(vec3 L, vec3 N, float n1, float n2) {
	float r = n1 / n2;
	float cosI = -dot(N, L);
	float sinT2 = r * r * (1.0 - cosI * cosI);
	//if (sinT2 > 1.0) return vec3(0.0);  // Total internal reflection
	float cosT = sqrt(1.0 - sinT2);
	return r * L + (r * cosI - cosT) * N;
}

struct Result {
	vec4 color;
	Ray rays[2];
	float t;
};

Result trace(Ray ray) {
	float closestHit = 1000000000000.0;
	float t = 0.0;
	int hitIdx = -1;
	float fu = 0.0;
	Result result;
	result.color = vec4(0, 0, 0, 0);
	result.rays[0] = Ray(vec3(0), -1, vec3(0), vec4(0));
	result.rays[1] = Ray(vec3(0), -1, vec3(0), vec4(0));
	for (int i = 0; i < numLights; i++) {
		float t = raySphere(ray, lights[i].position, lights[i].radius);
		if (t > 0) {
			result.color = vec4(lights[i].color, 1);
			closestHit = t;
		}
	}
	for (int i = 0; i < numSpheres; i++) {
		t = raySphere(ray, spheres[i].center, spheres[i].radius);
		//if ((t > 0 && t < closestHit) || (ray.inside == i && -t < closestHit)) {
		if (t > 0 && t < closestHit) {
			closestHit = t;
			hitIdx = i;
		} else if (ray.inside == i) {
			fu = t;
		}
	}
	if(hitIdx >= 0) {
		result.color = vec4(0, 0, 0, 0);
		int i = hitIdx;
		t = closestHit;
		vec3 hitPoint = ray.origin + t * ray.direction;
		vec3 normal = (hitPoint - spheres[i].center) / spheres[i].radius;
		vec3 origin = hitPoint + 0.001 * normal;
		
		// diffuse and specular: x, y
		if(spheres[i].material.x > 0.0 || spheres[i].material.y > 0.0) {
			for (int l = 0; l < numLights; l++) {
				Ray shadowRay;
				shadowRay.direction = lights[l].position - hitPoint;
				float lightDist = length(shadowRay.direction);
				shadowRay.direction = shadowRay.direction / lightDist;
				shadowRay.origin = origin;
				vec3 light = lights[l].color;
				float shadow = 0.0;
				for (int s = 0; s < 3; s++) {
					float shadow_t = raySphere(shadowRay, spheres[s].center, spheres[s].radius);
					if (shadow_t > 0.0) {
						light = mix(light, spheres[s].color.rgb, spheres[s].color.a) * (1.0 - spheres[s].color.a);
					}
				}
				light = light * (1.0 / (0.1 + lightDist));
				float lambertian = max(0.0, dot(normal, shadowRay.direction)) * spheres[i].material.x;
				float gloss = 32.0;
				vec3 h = (-shadowRay.direction + ray.direction) * -0.5;
				vec3 halfDir = normalize(-shadowRay.direction + ray.direction);
				float specAngle = max(0.0, dot(halfDir, -normal));
				float specular = pow(specAngle, gloss) * spheres[i].material.y;
				vec3 contribution = lambertian * light * spheres[i].color.rgb + specular * light;
				result.color += vec4(contribution, spheres[i].color.a);
			}
		}
		// reflection: z
		float fresnel = 0;//max(0.0, min(1, 0.5 * (0.5 - spheres[i].material.x) * (1.0 + dot(ray.direction, normal))));
		result.rays[0].origin = origin;
		result.rays[0].alpha = ray.alpha * fresnel + ray.alpha * spheres[i].color * spheres[i].material.z;
		result.rays[0].direction = reflect(ray.direction, normal);

		// refraction: w, color.a
		float refractiveIndexAir = 1.0;
		result.rays[1].origin = hitPoint - (0.001 * normal);
		result.rays[1].direction = refract(ray.direction, normal, refractiveIndexAir, spheres[i].material.w);

		if (ray.inside != -1) {
			if (ray.inside == i) {
				result.rays[1].inside = -1;
				float transparency = pow(spheres[i].color.a, abs(t));
				vec4 c = mix(spheres[i].color, vec4(1), transparency);
				result.rays[1].alpha = ray.alpha * c;
				result.rays[0].alpha = result.rays[0].alpha * c;
			}
			else {
				float transparency = pow(spheres[ray.inside].color.a, abs(t * 2));
				vec4 c = mix(spheres[ray.inside].color, vec4(1), transparency);
				result.rays[0].alpha = result.rays[0].alpha * c;
				result.rays[0].inside = ray.inside;
				result.color = result.color * c;
			}
		}
		// entering sphere
		else if(ray.inside == -1 && spheres[i].color.a < 1.0) {
			result.rays[1].inside = i;
			result.rays[1].alpha = vec4(mix(ray.alpha.rgb, spheres[i].color.rgb, spheres[i].material.x), ray.alpha.a);
		}
		// is outide, stay outside
		else {
			;
		}
	} else {
		if(closestHit == 1000000000000.0) {
			// sky color
			if(lights[6].position.y > 0.0) {
				result.color = mix(vec4(0.6, 0.3, 0.2, 1.0), vec4(0.8, 0.8, 1.0, 1.0), sqrt(lights[6].position.y / 100.0)) * ray.alpha.a;
			}
			else {
				result.color = mix(vec4(0.6, 0.3, 0.2, 1.0), vec4(0.0, 0.0, 0.02, 1.0), sqrt(-lights[6].position.y / 100.0)) * ray.alpha.a;
			}
		}
		if (ray.inside != -1) {
			float transparency = pow(spheres[ray.inside].color.a, spheres[ray.inside].radius);
			vec4 c = mix(spheres[ray.inside].color, vec4(1), transparency);
			result.color = result.color * c;
		}
	}
	result.t = closestHit;
	return result;
}

void main() {
	vec2 uv;
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
	uv.x = (2.0 * float(storePos.x) / canvasSize.x - 1.0);
	uv.y = (2.0 * float(storePos.y) / canvasSize.y - 1.0);
	float aspectRatio = canvasSize.x / canvasSize.y;

	Ray ray[MAXDEPTH + 1];
	int ridx = 0;
	ray[ridx].origin = vec3(0);
	ray[ridx].inside = -1;
	for(int i = 0; i < numSpheres; i++) {
		if(length(spheres[i].center) < spheres[i].radius) {
			ray[ridx].inside = i;
		}
	}
	ray[ridx].direction = normalize(vec3(aspectRatio * uv.x, uv.y, -1));
	ray[ridx].alpha = vec4(1.0);
	vec4 accumulatedColor = vec4(0, 0, 0, 0);

	for (int ridx = 0; ridx >= 0; ridx--) {
		Result result = trace(ray[ridx]);
		accumulatedColor += result.color * ray[ridx].alpha;
		if (ridx < maxDepth) {
			if(result.rays[0].alpha.a > 0.02) {
				ray[ridx++] = result.rays[0];
			}
			if(result.rays[1].alpha.a > 0.02) {
				ray[ridx++] = result.rays[1];
			}
		}
	}

	//accumulatedColor = accumulatedColor / accumulatedColor.a;
	accumulatedColor.r = tanh(accumulatedColor.r);
	accumulatedColor.g = tanh(accumulatedColor.g);
	accumulatedColor.b = tanh(accumulatedColor.b);
	imageStore(destTex, storePos, accumulatedColor);
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

// material: diffuse, specular, reflective, refractive
void updateSpheres() {
	for (int i = 0; i < 3; i++) {
		float t = 3.0 + (now() - tZero).count() / 2000000000.0;
		spheres[i].center = glm::vec3(1.6 * cos(t - ((float)i * 2.1)), 0.0f, 1.6 * sin(t - ((float)i * 2.1)) - 4.0f);
		spheres[i].radius = 1.0f;
	}
	spheres[3].center = glm::vec3(0.0f, -10001.5f * cos(0.1), -1000.0f);
	spheres[3].radius = 10000.0f;
	spheres[0].color = glm::vec4(1.0f, 0.95f, 0.5f, 1.0f);
	spheres[0].material = glm::vec4(0.1f, 0.1f, 0.8f, 1.0f);
	spheres[1].color = glm::vec4(1.0f, 0.5f, 1.0f, 0.5f);
	spheres[1].material = glm::vec4(0.0f, 0.0f, 0.0f, 1.03f);
	spheres[1].radius = 1.5f;// * cos((now() - tZero).count() / 10000000000.0);
	//spheres[1].center = glm::vec3(0.0f, 0.0f, -1.5f);
	spheres[2].color = glm::vec4(0.8f, 0.8f, 1.0f, 1.0f);
	spheres[2].material = glm::vec4(1.0f, 0.5f, 0.0f, 1.07f);
	spheres[3].color = glm::vec4(0.2f, 0.6f, 0.25f, 1.0f);
	spheres[3].material = glm::vec4(0.8f, 0.7f, 0.2f, 1.0f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * numSpheres, spheres, GL_STATIC_DRAW);
}

void updateLights() {
	auto time = (now() - tZero).count() / -1230000000.0;
	for (int i = 0; i < numLights; i++) {
		lights[i].id = i;
		lights[i].radius = 0.05f;
		lights[i].position = glm::vec3(1.2f * (i % 3) * sin(time / (i + 1)), 1.0f + 0.9f * sin(time + i), -1.5f + 1.0f * cos(time + i));
	}
	lights[0].color = glm::vec3(0.3f, 0.3f, 2.0f);
	lights[1].color = glm::vec3(0.3f, 2.0f, 0.3f);
	lights[2].color = glm::vec3(0.3f, 0.3f, 2.0f);
	lights[3].color = glm::vec3(2.0f, 0.3f, 0.3f);
	lights[4].color = glm::vec3(0.3f, 2.0f, 0.3f);
	lights[5].color = glm::vec3(2.0f, 0.3f, 0.3f);
	lights[6].color = glm::vec3(100.0f, 90.0f, 80.0f);
	//lights[6].position = glm::vec3(100.0f * sin(time / 5.0), 100.0f * abs(cos(time / 5.0)), 0.0f);
	lights[6].position = glm::vec3(100.0f * sin(time / 5.0), 100.0f * cos(time / 5.0), 0.0f);
	lights[6].radius = 5.0f;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(computeProgram);
	chkerr();
	
	glUniform1i(glGetUniformLocation(computeProgram, "maxDepth"), (int)maxDepth);
	glUniform1i(glGetUniformLocation(computeProgram, "numSpheres"), numSpheres);
	glUniform1i(glGetUniformLocation(computeProgram, "numLights"), numLights);
	glUniform2f(glGetUniformLocation(computeProgram, "canvasSize"), (float)canvasW, (float)canvasH);
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
	glDispatchCompute(canvasW / WORKGROUP_SIZE, canvasH / WORKGROUP_SIZE, 1);
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
	canvasW = winW * canvasScale;
	canvasH = winH * canvasScale;
	canvasW = (canvasW / WORKGROUP_SIZE) * WORKGROUP_SIZE;
	canvasH = (canvasH / WORKGROUP_SIZE) * WORKGROUP_SIZE;

	resizeTexture(outputTexture, canvasW, canvasH);
}


int fps_strikes = 0;
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
		if(fps < fps_limit * 0.8) {
			std::cout << "FPS: " << fps << " Frame Time: " << busyTime/(1000.0 * frameCount) << " ms " << "CPU Busy: " << (1.0 - (idleTime / elapsed)) * 100.0f << "%" << " maxDepth: " << maxDepth << " scale: " << canvasScale << std::endl;
		}
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
	if(frameDuration > 100.0 * frameTimeLimit) {
		maxDepth = 1;
		canvasScale = 0.1;
		reshape(winW, winH);
	}
	if (fps < fps_limit * 0.8) {
		fps_strikes++;
		if(fps_strikes > 10) {
			canvasScale *= 0.9;
			canvasScale = std::max(0.33f, canvasScale);
			reshape(winW, winH);
			fps_strikes = 0;
		}
	} else if((idleTime / elapsed) > 0.98) {
		fps_strikes--;
		if(fps_strikes < -10) {
			maxDepth = std::min((float)MAXDEPTH, maxDepth + 1);
			canvasScale *= 1.11;
			canvasScale = std::min(1.0f, canvasScale);
			reshape(winW, winH);
			fps_strikes = 0;
		}
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
	chkerr();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	chkerr();

	glutMainLoop();
	free(spheres);
	return 0;
}

