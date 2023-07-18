#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <array>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

extern "C" {
	#include "hrenderer.h"
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

const char *sphereVertSource = R"glsl(
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

const char *sphereFragSource = R"glsl(
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

const char *boxoidVertSource = R"glsl(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 light;
layout (location = 2) in vec3 tex;
layout (location = 3) in int flags;

uniform mat4 rotation;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 fragNormal;
out vec4 vertexPosClip;
out vec3 fragLight;
out vec3 fragTex;
flat out int fragFlags;

void main()
{
	fragPos = vec3(model * rotation * vec4(position, 1.0)); // transform vertex from object space to world space
	fragNormal = vec3(rotation * vec4(normal, 1.0));
	fragLight = light;
	fragTex = tex;
	vec4 posClip = projection * view * vec4(fragPos, 1.0); // transform vertex from world space to clip space
	vertexPosClip = posClip;  // Pass the clip space position to the fragment shader
	gl_Position = posClip;
	fragFlags = flags;
}
)glsl";

const char *boxoidFragSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec4 vertexPosClip;
in vec3 fragLight;
flat in int fragFlags;

struct light {
	vec3 position;
	vec3 color; // W/sr per component
};

const int MAX_LIGHTS = 1;
const int VERT_CORNER = 1;
const int VERT_OMIT = 2;

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
	vec3 accumulatedLight = vec3(0.0, 0.0, 0.0);
	vec4 invNormals = (vec4(1.0f) - vec4(fragNormal, 0.0));
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		vec3 lightVector = lightPositions[i] - fragPos;
		float squaredDistance = dot(lightVector, lightVector);
		vec3 lightDirection = lightVector / sqrt(squaredDistance);
		float attenuation = 1.0 / squaredDistance;
		float lambertian = max(dot(fragNormal, lightDirection), 0.0);
		//accumulatedLight += attenuation * materialDiffuse * lightColors[i] * lambertian;
		accumulatedLight += attenuation * invNormals.xyz * lightColors[i] * lambertian;
	}
	vec3 radiance = materialEmissive * 0.079577 + accumulatedLight;
	float darkness = 3.0 / (3.0 + exposure + radiance.r + radiance.g + radiance.b);
	radiance = radiance * darkness;
	fragColor = vec4(pow(radiance, vec3(1.0 / gamma)), 1.0) * 0.75 + invNormals * 0.25;
	const float constantForDepth = 1.0;
	const float farDistance = 3e18;
	const float offsetForDepth = 1.0;
	gl_FragDepth = (log(constantForDepth * vertexPosClip.z + offsetForDepth) / log(constantForDepth * farDistance + offsetForDepth));
	//fragColor = vec4(fragNormal, 0.5);
}

)glsl";

struct CompositeRenderObject;

typedef struct {
	Sphere *spheres;
	int numSpheres;
	Sphere *nextBatch;
	int nextNum;

	Objref *orefs;
    size_t norefs;
	Objref *nextOrefs;
    size_t nextNorefs;
	
	// samcro, ncro
	CompositeRenderObject* cro;
	size_t ncro;
	CompositeRenderObject* pendingCreate;
	CompositeRenderObject* pendingUpdate;

	render_misc renderMisc;
	pthread_t renderer_tid;
	pthread_mutex_t mutex;
	bool shouldExit;
	GLFWwindow *window;
} SharedData;

SharedData sharedData;
SharedData* sd;

struct BufferObject {
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	GLuint num_indices;
	
	BufferObject() {
		if(sharedData.renderer_tid == pthread_self()){
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
		}
		num_indices = 0;
	}

	void bind() {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	}

	void unbind() {
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void destroy() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		num_indices = 0;
	}
};

glm::quat quaternize(float in[4]) {
	// convert from hamilton WXYZ convention to JPL XYZW convention
	return glm::quat(-in[1], -in[2], -in[3], in[0]);
}

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

GLuint mkShader(const char* vertexSource, const char* fragmentSource) {
	GLuint vert = compileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	checkShader(GL_LINK_STATUS, program);
	return program;
}

GLfloat sphereVertices[] = {
	-1.0f,  1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	 1.0f, -1.0f, 1.0f,
	 1.0f,  1.0f, 1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f
};

GLuint sphereIndices[] = {
	0, 1, 2,
	0, 2, 3,
	4, 5, 6,
	4, 6, 7,
	5, 0, 1,
	5, 0, 4,
	3, 2, 6,
	3, 6, 7,
	7, 0, 3,
	7, 0, 4,
	6, 1, 2,
	6, 1, 5
};

// right - bottom rear - left -- left - top rear - right // rear plate
// left - bottom front - right -- right top front - left // front plate
// left - bottom rear - right -- right bottom front left // bottom plate
// right top rear left -- left top front right		   // top plate
// right bottom rear - top -- right top front - bottom   // right plate
// left bottom rear - front -- left top front - rear	 // left plate
// this boxoid is spherical
int numExampleBoxoids = 4;
Boxoid exampleBoxoids[4] =
{
	// cylinder
	{{-1.0f,  1.0f, 3.0f,
	-1.0f, -1.0f, 3.0f,
	 1.0f, -1.0f, 3.0f,
	 1.0f,  1.0f, 3.0f,
	-1.0f,  1.0f, -3.0f,
	-1.0f, -1.0f, -3.0f,
	 1.0f, -1.0f, -3.0f,
	 1.0f,  1.0f, -3.0f},
	{0.0, 0.0, // omitted
	0.0, 0.0, // omitted
	0.0, 1.0,
	0.0, 1.0,
	0.0, 1.0,
	0.0, 2.0},
	4, 0x03},
	// sled-like shape
	{{-2.0f,  1.0f, 1.0f,
	-2.0f, -1.0f, 1.0f,
	 2.0f, -1.0f, 1.0f,
	 2.0f,  1.0f, 1.0f,
	-2.0f,  1.5f, -1.0f,
	-2.0f, -1.0f, -1.0f,
	 2.0f, -1.0f, -1.0f,
	 2.0f,  1.5f, -1.0f},
	{1.0, 1.0,
	1.0, 1.0,
	1.0, 1.0,
	0.0, -1.0,
	1.0, 0.0,
	0.0, 2.0},
	4, 0x03},
	// ball that's supposed to be round but isn't because our tessellation is wrong
	{{-1.0f,  1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	 1.0f, -1.0f, 1.0f,
	 1.0f,  1.0f, 1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f},
	{1.0, 1.0,
	1.0, 1.0,
	1.0, 1.0,
	1.0, 1.0,
	1.0, 1.0,
	1.0, 1.0},
	4, 0x0},
	// box with 1 slanted face
	{{-2.0f,  1.0f, 1.0f,
	-2.0f, -1.0f, 1.0f,
	 2.0f, -1.0f, 1.0f,
	 2.0f,  1.0f, 1.0f,
	-2.0f,  0.7f, -1.0f,
	-2.0f, -0.7f, -1.0f,
	 2.0f, -1.0f, -1.0f,
	 2.0f,  1.0f, -1.0f},
	{0.0, 0.0,
	0.0, 0.0,
	0.0, 0.0,
	0.0, 0.0,
	0.0, 0.0,
	0.0, 0.0},
	4, 0x0}
};

float min(float a, float b) {
	if(a < b) {
		return a;
	}
	return b;
}

float max(float a, float b) {
	if(a > b) {
		return a;
	}
	return b;
}

float ilerpHalf(GLuint a, GLuint b) {
	return ((a + b) >> 1) & 0xFFFFFF00 | (a & b & 0x000000FF);
}

GLuint ilerp(GLuint a, GLuint b, float f) {
	return (GLuint((((float)b) * f) + (((float)a) * (1.0f - f))) & 0xFFFFFF00) | (a & b & 0x000000FF);
}

float lerp(float a, float b, float f) {
	return (b * f) + (a * (1.0f - f));
}

glm::vec3 vlerpHalf(glm::vec3 a, glm::vec3 b) {
	return (b + a) * 0.5f;
}

glm::vec3 vlerp(glm::vec3 a, glm::vec3 b, float f) {
	return (b * f) + (a * (1.0f - f));
}

GLuint VERT_ORIGINAL = 1;
GLuint VERT_CORNER = 2;
GLuint VERT_SHIFTED = 3;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 light;
	glm::vec3 tex;
	GLuint flags;

	Vertex() {}
	Vertex(glm::vec3 pposition, glm::vec3 pnormal, glm::vec3 plight, glm::vec3 ptex, GLuint pflags) {
		position=pposition;
		normal=pnormal;
		light=plight;
		tex=ptex;
		flags=pflags;
	}
};

struct Triangle;

struct Edge {
	GLuint verts[3];
	Edge* subdivisions[2];
	Triangle* tris[2];
	glm::vec3 normal;
	
	Edge() {}
	Edge(GLuint vert0, GLuint vert1) {
		verts[0] = vert0;
		verts[1] = vert1;
		verts[2] = -1;
		subdivisions[0] = nullptr;
		subdivisions[1] = nullptr;
		tris[0] = nullptr;
		tris[1] = nullptr;
	}
};

struct Triangle {
	GLuint verts[3];
	Edge *edges[3];
	glm::vec3 normal;
	GLuint faceIndex;

	Triangle() {}
	Triangle(GLuint pverts[3], Edge *pedges[3], GLuint pfaceIndex, Vertex* vertData, glm::vec3 centre) {
		verts[0] = pverts[0];
		verts[1] = pverts[1];
		verts[2] = pverts[2];
		edges[0] = pedges[0];
		edges[1] = pedges[1];
		edges[2] = pedges[2];
		// demand that all triangles are created in strict order
		// first edge must connect points 1 and 2
		assert(edges[0]->verts[0] == verts[0] || edges[0]->verts[1] == verts[0]);
		assert(edges[0]->verts[0] == verts[1] || edges[0]->verts[1] == verts[1]);
		// ssecond edge must connect points 2 and 3
		assert(edges[1]->verts[0] == verts[1] || edges[1]->verts[1] == verts[1]);
		assert(edges[1]->verts[0] == verts[2] || edges[1]->verts[1] == verts[2]);
		// third edge must connect points 3 and 1
		assert(edges[2]->verts[0] == verts[2] || edges[2]->verts[1] == verts[2]);
		assert(edges[2]->verts[0] == verts[0] || edges[2]->verts[1] == verts[0]);
		faceIndex = pfaceIndex;
		normal = glm::normalize(glm::cross((vertData[verts[0]].position - vertData[verts[1]].position), (vertData[verts[0]].position - vertData[verts[2]].position)));
		float dotProduct = glm::dot(normal, vertData[verts[0]].position - centre);
		if(dotProduct < 0.0f){
			normal = -normal;
		}
	}
};

void setEdgePtrs(Triangle* tri) {
	for(int i = 0; i < 3; i++) {
		if(tri->edges[i]->tris[0] == nullptr) {
			tri->edges[i]->tris[0] = tri;
		} else {
			tri->edges[i]->tris[1] = tri;
		}
	}
}

struct Mesh {
	glm::vec3 centre;
	glm::vec3 faceNormals[6];
	glm::vec3 faceCentres[6];
	GLuint numVerts;
	GLuint numTris;
	GLuint numEdges;
	GLuint numIndices;
	Vertex* verts;
	Triangle* tris;
	Edge* edges;
	GLuint* indices;

	Mesh(glm::vec3 pcentre, glm::vec3 pfaceNormals[6], glm::vec3 pfaceCentres[6], Vertex* pverts, GLuint pnumVerts, Triangle* ptris, GLuint pnumTris, Edge* pedges, GLuint pnumEdges, GLuint* pindices, GLuint pnumIndices) {
		centre = pcentre;
		for(int i = 0; i < 6; i++){
			faceNormals[i] = pfaceNormals[i];
			faceCentres[i] = pfaceCentres[i];
		}
		verts = pverts;
		numVerts = pnumVerts;
		tris = ptris;
		numTris = pnumTris;
		edges = pedges;
		numEdges = pnumEdges;
		indices = pindices;
		numIndices = pnumIndices;
	}
	Mesh() {}
};

void deleteMeshes(Mesh *meshes, size_t numMeshes) {
	for(size_t i = 0; i < numMeshes; i++) {
		free(meshes[i].verts);
		free(meshes[i].tris);
		free(meshes[i].edges);
		free(meshes[i].indices);
	}
}

glm::vec3 avgOf3(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	return (a + b + c) * (1.0f / 3.0f);
}

Vertex avgOf3(Vertex* a, Vertex* b, Vertex* c) {
	return Vertex(avgOf3(a->position, b->position, c->position), glm::normalize(avgOf3(a->normal, b->normal, c->normal)), avgOf3(a->light, b->light, c->light), avgOf3(a->tex, b->tex, c->tex), ((a->flags & 0xFFFFFF00 + b->flags & 0xFFFFFF00 + c->flags & 0xFFFFFF00 / 3) & 0xFFFFFF00) | ((a->flags & b->flags & c->flags) & 0xFF));
}

Vertex vertex_interpolate_half(Vertex* a, Vertex* b) {
	return Vertex(vlerpHalf(a->position, b->position), glm::normalize(vlerpHalf(a->normal, b->normal)), vlerpHalf(a->light, b->light), vlerpHalf(a->tex, b->tex), ilerpHalf(a->flags, b->flags));
}
/*
Vertex vertex_interpolate(Vertex* a, Vertex* b, float c) {
	//return Vertex(vlerp(a->position, b->position, c), glm::normalize(vlerp(a->normal, b->normal, c)), vlerp(a->light, b->light, c), a->flags & b->flags);
	return Vertex(vlerp(a->position, b->position, c), glm::normalize(vlerp(a->normal, b->normal, c)), vlerp(a->light, b->light, c), ilerp(a->flags, b->flags, c));
}
*/
glm::vec3 nearestPointOnPlane(glm::vec3 origin, glm::vec3 onPlane, glm::vec3 normal) {
    return onPlane + glm::dot(origin - onPlane, normal) * normal;
}

// Loop subdivision that doesn't create duplicate edges/vertices, runs in linear time
// it looks like the mesh needs a little bit smoothing at iterations 3 and above.
Mesh tessellateMesh(Mesh* original, int iteration, Boxoid* box) {
	GLuint numVerts = original->numEdges + original->numVerts;
	GLuint numTris = original->numTris * 4;
	GLuint numEdges = original->numEdges * 2 + original->numTris * 3;
	GLuint numIndices = numTris * 3;
	Vertex* verts = static_cast<Vertex*>(malloc(numVerts * sizeof(Vertex)));
	Triangle* tris = static_cast<Triangle*>(malloc(numTris * sizeof(Triangle)));
	Edge* edges = static_cast<Edge*>(malloc(numEdges * sizeof(Edge)));
	GLuint* indices = static_cast<GLuint*>(malloc(numIndices * sizeof(GLuint)));
	GLuint edgeIndex = 0;
	GLuint vertIndex = 0;
	GLuint triIndex = 0;
	GLuint indexIndex = 0;
	memcpy(verts, original->verts, original->numVerts * sizeof(Vertex));
	vertIndex += original->numVerts;

	// for each edge create a vertex and 2 new edges
	for(int i = 0; i < original->numEdges; i++) {
		Edge* edge = &(original->edges[i]);
		verts[vertIndex] = vertex_interpolate_half(&verts[edge->verts[0]], &verts[edge->verts[1]]);
		verts[vertIndex].flags &= ~(VERT_ORIGINAL | VERT_SHIFTED);
		if(edge->tris[1] == nullptr){
			verts[vertIndex].normal = edge->tris[0]->normal;
		} else {
			verts[vertIndex].flags &= ~VERT_CORNER;
			verts[vertIndex].normal = glm::normalize(edge->tris[0]->normal + edge->tris[1]->normal);
		}
		edge->verts[2] = vertIndex;
		edges[i * 2] = Edge(edge->verts[0], edge->verts[2]);
		edges[i * 2 + 1] = Edge(edge->verts[1], edge->verts[2]);
		edge->subdivisions[0] = &edges[i * 2];
		edge->subdivisions[1] = &edges[i * 2 + 1];
		vertIndex++;
		edgeIndex += 2;
	}

	// for each triangle create 3 new edges and 4 new triangles
	for(int i = 0; i < original->numTris; i++) {
		Triangle *tri = &original->tris[i];
		GLuint newVertices[3];
		Edge *edgeEdges[6];
		Edge *centreEdges[3];
		Triangle *newTris[4];
		for(int j = 0; j < 3; j++) {
			if(tri->edges[j]->subdivisions[0]->verts[0] == tri->verts[j]) {
				edgeEdges[j * 2] = tri->edges[j]->subdivisions[0];
				edgeEdges[j * 2 + 1] = tri->edges[j]->subdivisions[1];
			} else {
				assert(tri->edges[j]->subdivisions[1]->verts[0] == tri->verts[j]);
				edgeEdges[j * 2] = tri->edges[j]->subdivisions[1];
				edgeEdges[j * 2 + 1] = tri->edges[j]->subdivisions[0];
			}
			edges[edgeIndex] = Edge(tri->edges[j]->verts[2], tri->edges[(j + 1) % 3]->verts[2]);
			centreEdges[j] = &edges[edgeIndex];
			newVertices[j] = tri->edges[j]->verts[2];
			edgeIndex++;
		}
		for(int j = 0; j < 3; j++) {
			GLuint vertTemp[3] = {tri->verts[j], newVertices[j], newVertices[(j + 2) % 3]};
			Edge *edgeTemp[3] = {edgeEdges[(j * 2) % 6], centreEdges[(j + 2) % 3], edgeEdges[(j * 2 + 5) % 6]};
			tris[triIndex] = Triangle(vertTemp, edgeTemp, tri->faceIndex, verts, original->centre);
			newTris[j] = &tris[triIndex];
			setEdgePtrs(newTris[j]);
			triIndex++;
		}
		tris[triIndex] = Triangle(newVertices, centreEdges, tri->faceIndex, verts, original->centre);
		newTris[3] = &tris[triIndex];
		setEdgePtrs(newTris[3]);
		triIndex++;
		for(int j = 0; j < 4; j++) {
			indices[indexIndex++] = newTris[j]->verts[0];
			indices[indexIndex++] = newTris[j]->verts[1];
			indices[indexIndex++] = newTris[j]->verts[2];
		}
		// adjust vertex positions outwards to reflect each face's curvature in u,v dimensions
		for(int j = 0; j < 3; j++) {
			Vertex* v = &verts[newVertices[j]];
			if((v->flags & VERT_SHIFTED) == 0) {
				GLuint fi = tri->faceIndex;
				glm::vec3 toCentre = v->position - original->centre;
				glm::vec3 centreDirection = glm::normalize(toCentre);
				glm::vec3 onSpheroid = centreDirection * v->light.y;
				
				double offset = double(v->flags & 0xFFFFFF00) / double(0x1 << 24);
				glm::vec3 intersectionPoint = (toCentre * (float)(1.0 / offset)) + original->centre;
				float intersectionDistance;
				bool success = glm::intersectRayPlane(v->position, -tri->normal, original->faceCentres[fi], original->faceNormals[fi], intersectionDistance);
				if(success) {
					intersectionPoint = (intersectionPoint + v->position + (-tri->normal * intersectionDistance)) * 0.5f;
				}

				glm::vec3 onBox = intersectionPoint;
				float ucurve = 0.0f;
				float vcurve = 0.0f;
				if(fi == 0 || fi ==1 ) {
					ucurve = v->tex.x;
					vcurve = v->tex.y;
				} else if(fi == 2 || fi == 3) {
					ucurve = v->tex.z;
					vcurve = v->tex.y;
				} else if(fi == 4 || fi == 5) {
					ucurve = v->tex.z;
					vcurve = v->tex.x;
				}
				// this is the magic that makes cylindres, domes, boxes etc somewhat smoothly interpolate. transitions are rough.
				float ubulge = box->curvature[fi * 2] * (cos(ucurve * M_PI/4.0f) - 0.7071067811865476f);
				float vbulge = box->curvature[fi * 2 + 1] * (cos(vcurve * M_PI/4.0f) - 0.7071067811865476f);
				float curvature = ubulge + vbulge;
				v->position = onBox + (tri->normal + centreDirection) * (curvature * 0.5f);
				
				// this makes spherical surfaces really smooth and flat surfaces really flat but doesn't do much to surfaces that
				// have curvature not close to 0 and 1 on both axes (now tries to do something with negatively curved surfaces, wish it luck)
				float curviness = max(0.0f, min(1.0f, box->curvature[fi * 2] * box->curvature[fi * 2 + 1]));
				if(box->curvature[fi * 2] < 0.0f && box->curvature[fi * 2 + 1] < 0.0f) {
					curviness = -curviness;
				}
				v->position = vlerp(v->position, onSpheroid, curviness);
				float flatness = -0.5f + 1.5f / (1.5 + 5 * (abs(box->curvature[fi * 2]) + abs(box->curvature[fi * 2 + 1])));
				v->normal = glm::normalize(vlerp(v->normal, original->faceNormals[fi], flatness));
				v->normal = glm::normalize(vlerp(v->normal, centreDirection, curviness));
				
				v->flags |= VERT_SHIFTED;
				offset = (double)glm::length(v->position - original->centre) / (double)glm::length(onBox - original->centre);
				offset *= double(0x1 << 24);
				v->flags = (((GLuint)offset) & 0xFFFFFF00) | (v->flags & 0x000000FF);
			}
		}
	}
	// do some smoothing to counteract numeric instability, algo has some drawbacks and improvements are welcome
	for(int i = 0; i < numTris; i++) {
		Vertex avgVert;
		float smoothingMagnitude = 0.2f * (iteration - 3);
		float deviance = 0.0f;
		Triangle* t = &tris[i];
		Triangle* neighbors[3];
		for(int j = 0; j < 3; j++) {
			neighbors[j] = (Triangle*) ((size_t)t->edges[j]->tris[0] + (size_t)t->edges[j]->tris[1] - (size_t)t);
			if(neighbors[j] == nullptr){
				goto neeext;
			}
		}
		GLuint opposites[3];
		for(int j = 0; j < 3; j++) {
			for(int k = 0; k < 3; k++) {
				GLuint v = neighbors[j]->verts[k];
				if(v != t->verts[0] && v != t->verts[1] && v != t->verts[2]){
					opposites[j] = v;
				}
			}
		}
		for(int j = 0; j < 3; j++) {
			deviance += glm::length(neighbors[j]->normal - neighbors[(j + 1) %3 ]->normal);
		}
		avgVert = avgOf3(&verts[opposites[0]], &verts[opposites[1]], &verts[opposites[2]]);
		for(int j = 0; j < 3; j++) {
			Vertex* v = &verts[tris[i].verts[j]];
			if(iteration > 3){
				v->position = vlerp(v->position, avgVert.position, smoothingMagnitude / (1.0f + deviance));
			}
			v->normal = avgVert.normal;
			//v->normal = glm::normalize(avgVert.normal + v->normal);
		}
neeext:
		;
	}
	printf("1 mesh subdivided. %d verts, %d tris, %d edges, %d indices\n", numVerts, numTris, numEdges, numIndices);
	return Mesh(original->centre, original->faceNormals, original->faceCentres, verts, numVerts, tris, numTris, edges, numEdges, indices, numIndices);
}

Mesh boxoidToMesh(Boxoid box) {
	Vertex * verts = static_cast<Vertex *>(malloc(8 * sizeof(Vertex)));
	Triangle * tris = static_cast<Triangle *>(malloc(12 * sizeof(Triangle)));
	int t = 0;
	int e = 0;
	int indexIndex = 0;
	Edge * edges = static_cast<Edge *>(malloc(36 * sizeof(Edge)));
	GLuint * indices = static_cast<GLuint *>(malloc(36 * sizeof(GLuint)));
	glm::vec3 faceNormals[6];
	glm::vec3 faceCentres[6];
	
	glm::vec3 corners[8];
	glm::vec3 centre = glm::vec3(0, 0, 0);
	for(int i = 0; i < 8; i++) {
		corners[i] = vectorize(&box.corners[i * 3]);
		centre += corners[i];
	}
	centre *= (1.0f/8.0f);
	for(int i = 0; i < 8; i++) {
		verts[i] = Vertex(corners[i],
			glm::normalize(corners[i] - centre),
			glm::vec3(0.0f, glm::length(corners[i] - centre), 1.0f),
			glm::vec3(sphereVertices[i * 3], sphereVertices[i * 3 + 1], sphereVertices[i * 3 + 2]),
			VERT_ORIGINAL | VERT_CORNER | (0x1 << 24));
	}
	for(int i = 0; i < 12; i++) {
		if(box.missing_faces & (0x1 << (i/2))) {
			// skip this triangle
		} else {
			indices[indexIndex++] = sphereIndices[i * 3];
			indices[indexIndex++] = sphereIndices[i * 3 + 1];
			indices[indexIndex++] = sphereIndices[i * 3 + 2];
			GLuint tmpVerts[3] = {sphereIndices[i * 3], sphereIndices[i * 3 + 1], sphereIndices[i * 3 + 2]}; 
			edges[e] = Edge(tmpVerts[0], tmpVerts[1]);
			edges[e + 1] = Edge(tmpVerts[1], tmpVerts[2]);
			edges[e + 2] = Edge(tmpVerts[2], tmpVerts[0]);
			Edge* tmpEdges[3] = {&edges[e], &edges[e + 1], &edges[e + 2]};
			e += 3;
			tris[t] = Triangle(tmpVerts, tmpEdges, i / 2, verts, centre);
			setEdgePtrs(&tris[t]);
			faceNormals[i / 2] = tris[t].normal;
			t++;
		}
		if((i % 2) == 0){
			faceCentres[i / 2] = (corners[sphereIndices[i * 3]] + corners[sphereIndices[i * 3 + 1]] +
								  corners[sphereIndices[i * 3 + 2]] + corners[sphereIndices[i * 3 + 5]]) * 0.25f;
		}
	}
	// remove duplicate edges
	Edge* realEdges = static_cast<Edge*>(malloc(18 * sizeof(Edge)));
	int r = 0;
	for(int i = 0; i < e; i++) {
		Edge* a = &edges[i];
		Edge* b = nullptr;
		for(int j = 0; j < r; j++) {
			b = &realEdges[j];
			if((a->verts[0] == b->verts[0] && a->verts[1] == b->verts[1] ||
				(a->verts[1] == b->verts[0] && a->verts[0] == b->verts[1]))) {
				// we found a duplicate!
				goto neeext;
			}
		}
		// didn't find anything, we have a brand new edge
		realEdges[r] = *a;
		b = &realEdges[r];
		r++;
neeext:
		// before discarding a, let's update any pointers pointing to it to point to b instead
		for(int k = 0; k < t; k++) {
			for(int l = 0; l < 3; l++) {
				if(tris[k].edges[l] == a) {
					tris[k].edges[l] = b;
				}
			}
		}
	}
	free(edges);
	printf("1 mesh generated. 8 verts, %d tris, %d edges, %d indices\n", t, r, indexIndex);
	return Mesh(centre, faceNormals, faceCentres, verts, 8, tris, t, realEdges, r, indices, indexIndex);
}

void uploadMeshes(const Mesh* meshes, int numMeshes, BufferObject* bo);

// opengl and multithreading: we create the BufferObjects on the rendering thread.
// The tessellation can be done on any thread (main thread for now).
// copying vertex and index data to the gpu is done on rendering thread.
#define MAX_LOD 3
struct CompositeRenderObject {
	CompositeRenderObject* next;
	Composite c;
	Composite update;
	BufferObject bo[MAX_LOD + 1];
	Mesh* meshes[MAX_LOD + 1];
	
	CompositeRenderObject(Composite pc, CompositeRenderObject* pnext) {
		next = pnext;
		c = pc;
		update = pc;
		for(int i = 0; i <= MAX_LOD; i++) {
			meshes[i] = nullptr;
		}
	}

	void destroy() {
		for(int i = 0; i <= MAX_LOD; i++) {
			bo[i].destroy();
			deleteMeshes(meshes[i], c.nb);
			meshes[i] = nullptr;
		}
	}

	void tessellate() {
		for(int i = 0; i <= MAX_LOD; i++) {
			meshes[i] = (Mesh*)malloc(c.nb * sizeof(Mesh));
			for(int j = 0; j < c.nb; j++) {
				if(i == 0) {
					meshes[i][j] = boxoidToMesh(c.b[j]);
				} else {
					meshes[i][j] = tessellateMesh(&meshes[i-1][j], i, &c.b[j]);
				}
			}
		}
	}

	void submitMeshUpdate(CompositeRenderObject *latest) {
		for(int i = 0; i <= MAX_LOD; i++) {
			deleteMeshes(meshes[i], c.nb);
			free(meshes[i]);
			meshes[i] = latest->meshes[i];
		}
		update = latest->c;
	}
	
	void createBuffers() {
		assert(sharedData.renderer_tid == pthread_self());
		for(int i = 0; i <= MAX_LOD; i++) {
			bo[i] = BufferObject();
		}
	}

	void copyToBuffers() {
		// this function should only be called on the rendering thread
		assert(sharedData.renderer_tid == pthread_self());
		if(c.nb != update.nb) {
			free(c.b);
			c.b = update.b;
			c.nb = update.nb;
		}
		for(int i = 0; i <= MAX_LOD; i++) {
			uploadMeshes(meshes[i], c.nb, &bo[i]);
		}
	}
};


void uploadMeshes(const Mesh* meshes, int numMeshes, BufferObject* bo)
{
	bo->bind();
	GLuint totalNumVerts = 0;
	GLuint totalNumIndices = 0;
	for (size_t i = 0; i < numMeshes; i++) {
		totalNumVerts += meshes[i].numVerts;
		totalNumIndices += meshes[i].numIndices;
	}
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts * sizeof(Vertex), nullptr, GL_STATIC_DRAW);
	GLuint* adjustedIndices = (GLuint*)malloc(totalNumIndices * sizeof(GLuint));
	GLuint indexOffset = 0;
	GLuint vertexOffset = 0;
	for (size_t i = 0; i < numMeshes; i++) {
		const Mesh& mesh = meshes[i];
		glBufferSubData(GL_ARRAY_BUFFER, vertexOffset * sizeof(Vertex), mesh.numVerts * sizeof(Vertex), mesh.verts);
		for (GLuint j = 0; j < mesh.numIndices; j++) {
			adjustedIndices[indexOffset + j] = mesh.indices[j] + vertexOffset;
		}
		indexOffset += mesh.numIndices;
		vertexOffset += mesh.numVerts;
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalNumIndices * sizeof(GLuint), adjustedIndices, GL_STATIC_DRAW);
	free(adjustedIndices);
	
	
	// Probably don't need to do this here
	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, light));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, tex));
	glVertexAttribPointer(4, 1, GL_UNSIGNED_INT, GL_FALSE, stride, (void*)offsetof(Vertex, flags));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	bo->num_indices = totalNumIndices;

	printf("uploaded %d indices to vertex buffer\n", totalNumIndices);
}

void renderMeshes(BufferObject* bo)
{
	bo->bind();
	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, light));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, tex));
	glVertexAttribPointer(4, 1, GL_UNSIGNED_INT, GL_FALSE, stride, (void*)offsetof(Vertex, flags));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glDrawElements(GL_TRIANGLES, bo->num_indices, GL_UNSIGNED_INT, nullptr);
}

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

	// shaders
	GLuint sphereProgram = mkShader(sphereVertSource, sphereFragSource);
	GLuint boxoidProgram = mkShader(boxoidVertSource, boxoidFragSource);
	GLuint skyboxProgram = mkShader(skyboxVertSource, skyboxFragSource);
 
	// buffers
	BufferObject boxoidBO = BufferObject();
	GLuint sphereVAO, sphereVBO, sphereEBO;
	glGenVertexArrays(1, &sphereVAO);
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &sphereEBO);
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertices), sphereVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphereIndices), sphereIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// uniforms
	GLuint sphereModelLoc = glGetUniformLocation(sphereProgram, "model");
	GLuint sphereViewLoc = glGetUniformLocation(sphereProgram, "view");
	GLuint sphereProjLoc = glGetUniformLocation(sphereProgram, "projection");
	GLuint sphereCenterLoc = glGetUniformLocation(sphereProgram, "sphereCenter");
	GLuint sphereRadiusLoc = glGetUniformLocation(sphereProgram, "sphereRadius");
	GLuint sphereCameraPosLoc = glGetUniformLocation(sphereProgram, "cameraPos");
	GLuint sphereLightPositionsLoc = glGetUniformLocation(sphereProgram, "lightPositions");
	GLuint sphereLightColorsLoc = glGetUniformLocation(sphereProgram, "lightColors");
	GLuint sphereDiffuseLoc = glGetUniformLocation(sphereProgram, "materialDiffuse");
	GLuint sphereEmissiveLoc = glGetUniformLocation(sphereProgram, "materialEmissive");
	GLuint sphereGammaLoc = glGetUniformLocation(sphereProgram, "gamma");
	GLuint sphereExposureLoc = glGetUniformLocation(sphereProgram, "exposure");

	GLuint boxoidRotationLoc = glGetUniformLocation(boxoidProgram, "rotation");
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
	GLuint skyboxTimeLoc = glGetUniformLocation(skyboxProgram, "time");
	GLuint skyboxResolutionLoc = glGetUniformLocation(skyboxProgram, "resolution");

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
		
		// Swap pointers and copy data between cpu and gpu
		// This lets the rendering thread and physics simulation both run at full tilt
		// without having to wait for each other
		// currently we do tessellation on the physics thread, cpu usage is fairly low
		pthread_mutex_lock(&sharedData.mutex);

		if(sharedData.nextOrefs != NULL) {
			if(sharedData.orefs != NULL) {
				free(sharedData.orefs);
			}
			sharedData.orefs = sharedData.nextOrefs;
			sharedData.norefs = sharedData.nextNorefs;
			sharedData.nextOrefs = NULL;
			sharedData.nextNorefs = 0;
		}

		if(sharedData.nextBatch != NULL) {
			if(sharedData.spheres != NULL) {
				free(sharedData.spheres);
			}
			sharedData.spheres = sharedData.nextBatch;
			sharedData.numSpheres = sharedData.nextNum;
			sharedData.nextBatch = NULL;
			sharedData.nextNum = 0;
		}

		while(sd->pendingCreate != NULL) {
			sd->pendingCreate->createBuffers();
			sd->pendingCreate->copyToBuffers();
			sd->pendingCreate = sd->pendingCreate->next;
		}

		while(sd->pendingUpdate != NULL) {
			sd->pendingUpdate->copyToBuffers();
			sd->pendingUpdate = sd->pendingUpdate->next;
		}

		pthread_mutex_unlock(&sharedData.mutex);
		
		// Camera
		cameraPos = glm::vec3(sharedData.renderMisc.camPosition[0], sharedData.renderMisc.camPosition[1], sharedData.renderMisc.camPosition[2]);
		cameraTarget = glm::vec3(sharedData.renderMisc.camForward[0], sharedData.renderMisc.camForward[1], sharedData.renderMisc.camForward[2]);
		cameraUp = glm::vec3(sharedData.renderMisc.camUp[0], sharedData.renderMisc.camUp[1], sharedData.renderMisc.camUp[2]);
		glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
		glm::mat4 model = glm::mat4(1.0f);
	   
		// draw the skybox
		glBindVertexArray(sphereVAO);
		glUseProgram(skyboxProgram);
		float time = static_cast<float>(glfwGetTime());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(skyboxModelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(skyboxTimeLoc, time);
		glUniform2f(skyboxResolutionLoc, 3840, 2160);
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
		
		glUseProgram(sphereProgram);
		glUniform3f(sphereCameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
		glUniformMatrix4fv(sphereViewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(sphereProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform1f(sphereGammaLoc, 2.2f);
		glUniform1f(sphereExposureLoc, 200.0f);
		for (int i = 0; i < MAX_LIGHTS; ++i) {
			glUniform3fv(sphereLightPositionsLoc + i, 1, &sharedData.renderMisc.lights[i].position[0]);
			glUniform3fv(sphereLightColorsLoc + i, 1, &sharedData.renderMisc.lights[i].color[0]);
		}

		// Geometry
		// Spheres
		if (sharedData.spheres != NULL) {
			// vertex buffer
			glBindVertexArray(sphereVAO);
			for (int i = 0; i < sharedData.numSpheres; ++i) {
				Sphere currentSphere = sharedData.spheres[i];
				glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(currentSphere.radius));

				// translate our model view to position the sphere in world space
				model = glm::translate(glm::mat4(1.0f), glm::vec3(currentSphere.position[0], currentSphere.position[1], currentSphere.position[2]));
				model = model * scaling;
				glUniformMatrix4fv(sphereModelLoc, 1, GL_FALSE, glm::value_ptr(model));

				// send the position of the sphere directly to the fragment shader, bypassing the vertex shader
				glUniform3f(sphereCenterLoc, currentSphere.position[0], currentSphere.position[1], currentSphere.position[2]);
				glUniform1f(sphereRadiusLoc, currentSphere.radius);
			   
				material* mat = &sharedData.renderMisc.materials[currentSphere.material_idx];
				glm::vec3 diffuseComponent = glm::vec3(mat->diffuse[0], mat->diffuse[1], mat->diffuse[2]); // 0 to 1
				glm::vec3 emissiveComponent = glm::vec3(mat->emissive[0], mat->emissive[1], mat->emissive[2]); // W/m^2
		
				// colors looked washed out so I did a thing. not quite vibrance so I'll call it vibe. texture saturation? but we don't have textures.
				float vibe = 3.0;
				diffuseComponent = glm::vec3(std::pow(diffuseComponent.r, vibe), std::pow(diffuseComponent.g, vibe), std::pow(diffuseComponent.b, vibe));
				glUniform3fv(sphereDiffuseLoc, 1, glm::value_ptr(diffuseComponent));
				glUniform3fv(sphereEmissiveLoc, 1, glm::value_ptr(emissiveComponent));

				// Draw the sphere
				if(i < 5) {
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
				}
			}
		}

		// Boxoids
		for(int i = 0; i < sd->norefs; i++) {
			// for now correctly assuming all orefs are to composites
			CompositeRenderObject* cro = &sd->cro[sd->orefs[i].id];
			cro->c.position[0] = sd->orefs[i].position[0];
			cro->c.position[1] = sd->orefs[i].position[1];
			cro->c.position[2] = sd->orefs[i].position[2];
			cro->c.orientation[0] = sd->orefs[i].orientation[0];
			cro->c.orientation[1] = sd->orefs[i].orientation[1];
			cro->c.orientation[2] = sd->orefs[i].orientation[2];
			cro->c.orientation[3] = sd->orefs[i].orientation[3];
			glm::vec3 center = vectorize(cro->c.position);
			// uniforms
			glUseProgram(boxoidProgram);
			model = glm::translate(glm::mat4(1.0f), center);
			glUniformMatrix4fv(boxoidModelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(boxoidCenterLoc, center[0], center[1], center[2]);
			// separate rotation from translation so we can rotate normals in the vertex shader
			//glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 3.14f * sin(time / 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rotation = glm::mat4(quaternize(cro->c.orientation));
			glUniformMatrix4fv(boxoidRotationLoc, 1, GL_FALSE, glm::value_ptr(rotation));
			glm::vec3 diffuseComponent = vectorize(sharedData.renderMisc.materials[cro->c.b[0].material_idx].diffuse);
			glm::vec3 emissiveComponent = vectorize(sharedData.renderMisc.materials[cro->c.b[0].material_idx].emissive);
			float vibe = 3.0;
			diffuseComponent = glm::vec3(std::pow(diffuseComponent.r, vibe), std::pow(diffuseComponent.g, vibe), std::pow(diffuseComponent.b, vibe));
			glUniform3fv(boxoidDiffuseLoc, 1, glm::value_ptr(diffuseComponent));
			glUniform3fv(boxoidEmissiveLoc, 1, glm::value_ptr(emissiveComponent));
		    int lod = sd->renderMisc.buttonPresses % MAX_LOD;
			renderMeshes(&cro->bo[lod]);
			//printf("rendered a mesh at LOD %d, position: %f %f %f\n", lod, center.x, center.y, center.z);
		}
		glBindVertexArray(0);
		glfwSwapBuffers(sharedData.window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);
	glDeleteProgram(sphereProgram);
	glfwTerminate();
	return 0;
}

// submit a composite to the 3d engine to prepare it for rendering
extern "C" Objref submitComposite(Composite c) {
	pthread_mutex_lock(&sharedData.mutex);
	// don't run out of buffer space
	if(sd->ncro == 0) {
		sd->cro = (CompositeRenderObject*)malloc(64 * sizeof(CompositeRenderObject));
	} else if ((sd->ncro % 64) == 0) {
		CompositeRenderObject* biggerBuffer = (CompositeRenderObject*)malloc((sd->ncro + 64) * sizeof(CompositeRenderObject));
		memcpy(biggerBuffer, sd->cro, sd->ncro);
		free(sd->cro);
		sd->cro = biggerBuffer;
	}
	// create CompositeRenderObject
	sd->cro[sd->ncro] = CompositeRenderObject(c, NULL);
	Objref oref = Objref();
	memcpy(oref.orientation, c.orientation, 4*sizeof(float));
	memcpy(oref.position, c.position, 3*sizeof(float));
	oref.id = sd->ncro;
	oref.type = COMPOSITE;
	sd->ncro++;
	pthread_mutex_unlock(&sharedData.mutex);
	
	// tessellate geometry without hogging the mutex
	sd->cro[oref.id].tessellate();

	// send to render thread for buffering
	pthread_mutex_lock(&sharedData.mutex);
	sd->cro[oref.id].next = sd->pendingCreate;
	sd->pendingCreate = &sd->cro[oref.id];
	pthread_mutex_unlock(&sharedData.mutex);

	return oref;
}

// update the geometry of an existing composite
extern "C" void updateComposite(Objref oref, Composite* c) {
	// tessellate geometry on a copy of the renderobject
	CompositeRenderObject cro = sd->cro[oref.id];
	cro.c = *c;
	cro.tessellate();
	
	// swap out some pointers
	pthread_mutex_lock(&sharedData.mutex);	
	sd->cro[oref.id].submitMeshUpdate(&cro);

	// send to render thread for buffering
	sd->cro[oref.id].next = sd->pendingUpdate;
	sd->pendingUpdate = &sd->cro[oref.id];
	pthread_mutex_unlock(&sharedData.mutex);
}

// thread safe entry point copies all the data to keep things dumb.
//extern "C" void render(Sphere* spheres, size_t sphereCount, render_misc renderMisc) {
extern "C" void render(Objref* oref, size_t nobj, Sphere* spheres, size_t numSpheres, render_misc renderMisc) {
	pthread_mutex_lock(&sharedData.mutex);
	if(sharedData.nextOrefs != NULL) {
		free(sharedData.nextOrefs);
	}
	if(sharedData.nextBatch != NULL) {
		free(sharedData.nextBatch);
	}
	sharedData.nextOrefs = (Objref*)malloc(sizeof(Objref) * nobj);
	sharedData.nextBatch = (Sphere*)malloc(sizeof(Sphere) * numSpheres);
	memcpy(sharedData.nextOrefs, oref, sizeof(Objref) * nobj);
	memcpy(sharedData.nextBatch, spheres, sizeof(Sphere) * numSpheres);
	sharedData.nextNorefs = nobj;
	sharedData.nextNum = numSpheres;
	sharedData.renderMisc = renderMisc;
	pthread_mutex_unlock(&sharedData.mutex);
}

extern "C" void startRenderer() {
	sd = &sharedData;
	bzero(sd, sizeof(sharedData));
	pthread_mutex_init(&sharedData.mutex, NULL);
	// start the rendering thread
	pthread_create(&sharedData.renderer_tid, NULL, rendererThread, &sharedData);
}

extern "C" void stopRenderer() {
	sharedData.shouldExit = true;
	pthread_join(sharedData.renderer_tid, NULL);
	pthread_mutex_destroy(&sharedData.mutex);
}


