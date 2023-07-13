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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C" {
	#include "renderer.h"
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
layout (location = 3) in int flags;

uniform mat4 rotation;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 fragNormal;
out vec4 vertexPosClip;
out vec3 fragLight;
flat out int fragFlags;

void main()
{
	fragPos = vec3(model * rotation * vec4(position, 1.0)); // transform vertex from object space to world space
	fragNormal = vec3(rotation * vec4(normal, 1.0));
	fragLight = light;
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
	for (int i = 0; i < MAX_LIGHTS; ++i) {
		vec3 lightVector = lightPositions[i] - fragPos;
		float squaredDistance = dot(lightVector, lightVector);
		vec3 lightDirection = lightVector / sqrt(squaredDistance);
		float attenuation = 1.0 / squaredDistance;
		float lambertian = max(dot(fragNormal, lightDirection), 0.0);
		accumulatedLight += attenuation * materialDiffuse * lightColors[i] * lambertian;
	}
	vec3 radiance = materialEmissive * 0.079577 + accumulatedLight;
	float darkness = 3.0 / (3.0 + exposure + radiance.r + radiance.g + radiance.b);
	radiance = radiance * darkness;
	fragColor = vec4(pow(radiance, vec3(1.0 / gamma)), 0.5) + vec4(fragNormal, 0.25);
	const float constantForDepth = 1.0;
	const float farDistance = 3e18;
	const float offsetForDepth = 1.0;
	gl_FragDepth = (log(constantForDepth * vertexPosClip.z + offsetForDepth) / log(constantForDepth * farDistance + offsetForDepth));
	//fragColor = vec4(fragNormal, 0.5);
}

)glsl";

typedef struct {
	Sphere *spheres;
	int numSpheres;
	Sphere *nextBatch;
	int nextNum;
	render_misc renderMisc;
	pthread_t renderer_tid;
	pthread_mutex_t mutex;
	bool shouldExit;
	GLFWwindow *window;
} SharedData;

SharedData sharedData;

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
Boxoid exampleBoxoids[2] =
{
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
	4, 0x03},
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
	4, 0x0}
};


glm::vec3 vlerp(glm::vec3 a, glm::vec3 b, float f) {
	return (a * f) + (b * (1.0f - f));
}

GLuint VERT_ORIGINAL = 1;
GLuint VERT_SHIFTED = 2;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 light;
	GLuint flags;

	Vertex() {}
	Vertex(glm::vec3 pposition, glm::vec3 pnormal, glm::vec3 plight, GLuint pflags) {
		position=pposition;
		normal=pnormal;
		light=plight;
		flags=pflags;
	}
};

struct Edge {
	GLuint verts[3];
	Edge* subdivisions[2];
	
	Edge() {}
	Edge(GLuint vert0, GLuint vert1) {
		verts[0] = vert0;
		verts[1] = vert1;
		verts[2] = -1;
		subdivisions[0] = nullptr;
		subdivisions[1] = nullptr;
	}
};

struct Triangle {
	GLuint verts[3];
	Edge *edges[3];
	GLuint faceIndex;

	Triangle() {}
	Triangle(GLuint pverts[3], Edge *pedges[3], GLuint pfaceIndex) {
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
	}
};

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

Vertex vertex_interpolate(Vertex* a, Vertex* b, float c) {
	return Vertex(vlerp(a->position, b->position, c), glm::normalize(vlerp(a->normal, b->normal, c)), vlerp(a->light, b->light, c), a->flags & b->flags);
}

// Loop subdivision that doesn't create duplicate edges/vertices, runs in linear time
Mesh tessellateMesh(Mesh* original, int iteration, Boxoid* box) {
	GLuint numVerts = original->numEdges + original->numVerts;
	GLuint numTris = original->numTris * 4;
	GLuint numEdges = original->numEdges * 2 + original->numTris * 3;
	GLuint numIndices = numTris * 3;
	Vertex* verts = malloc(numVerts * sizeof(Vertex));
	Triangle* tris = malloc(numTris * sizeof(Triangle));
	Edge* edges = malloc(numEdges * sizeof(Edge));
	GLuint* indices = malloc(numIndices * sizeof(GLuint));
	GLuint edgeIndex = 0;
	GLuint vertIndex = 0;
	GLuint triIndex = 0;
	GLuint indexIndex = 0;
	memcpy(verts, original->verts, original->numVerts * sizeof(Vertex));
	vertIndex += original->numVerts;

	// for each edge create a vertex and 2 new edges
	for(int i = 0; i < original->numEdges; i++) {
		Edge* edge = &(original->edges[i]);
		verts[vertIndex] = vertex_interpolate(&verts[edge->verts[0]], &verts[edge->verts[1]], 0.5f);
		verts[vertIndex].flags = 0;
		edge->verts[2] = vertIndex;
		if(iteration == 0) { // the light value isn't being used for light yet so we'll use it for this instead
			verts[vertIndex].light = glm::vec3(1.0f, glm::distance(verts[edge->verts[0]].position, verts[edge->verts[1]].position), 0.0f);
		}
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
			// this shit. The result looks fine after 1 level of tessellation but after 2 it's messed up.
			// I bet the problem is somewhere around here..
			GLuint vertTemp[3] = {tri->verts[j], newVertices[j], newVertices[(j + 2) % 3]};
			Edge *edgeTemp[3] = {edgeEdges[(j * 2) % 6], centreEdges[(j + 2) % 3], edgeEdges[(j * 2 + 5) % 6]};
			tris[triIndex] = Triangle(vertTemp, edgeTemp, tri->faceIndex);
			newTris[j] = &tris[triIndex];
			triIndex++;
		}
		tris[triIndex] = Triangle(newVertices, centreEdges, tri->faceIndex);
		newTris[3] = &tris[triIndex];
		triIndex++;
		for(int j = 0; j < 4; j++) {
			indices[indexIndex++] = newTris[j]->verts[0];
			indices[indexIndex++] = newTris[j]->verts[1];
			indices[indexIndex++] = newTris[j]->verts[2];
			//printf("indices: %d %d %d\n", indices[i * 9 + j * 3], indices[i * 9 + j * 3 + 1], indices[i * 9 + j * 3 + 2]);
		}
		for(int j = 0; j < 3; j++) {
			// should discriminate between these two but we'll just add them together for now
			float curvature = box->curvature[tri->faceIndex * 2] + box->curvature[tri->faceIndex * 2 + 1];
			// light is a misnomer. this is heavy. it's also repurposing the "light" variable to store temporary values for the curvature calculation, which is the whole reason we are tessellating this mesh in the first place
			glm::vec3 light = verts[newVertices[j]].light;
			float magnitude = 0.025f * curvature * light.y * sqrt(1.0f - (1.0f - light.x) * (1.0f - light.x));
			glm::vec3 offset = original->faceNormals[tri->faceIndex] * magnitude;
			verts[newVertices[j]].position += offset;
			verts[newVertices[j]].flags |= VERT_SHIFTED;
			glm::vec3 p[3] = {verts[newVertices[j]].position,
				verts[newVertices[(j + 1) % 3]].position,
				verts[newVertices[(j + 2) % 3]].position};
			verts[newVertices[j]].normal = glm::normalize(glm::cross(p[0] - p[1], p[0] - p[2]));
		}
	}
	printf("1 mesh subdivided. %d verts, %d tris, %d edges, %d indices\n", numVerts, numTris, numEdges, numIndices);
	return Mesh(original->centre, original->faceNormals, original->faceCentres, verts, numVerts, tris, numTris, edges, numEdges, indices, numIndices);
}

Mesh boxoidToMesh(Boxoid box) {
	Vertex *verts = malloc(8 * sizeof(Vertex));
	Triangle *tris = malloc(12 * sizeof(Triangle));
	int t = 0;
	int e = 0;
	int indexIndex = 0;
	Edge *edges = malloc(36 * sizeof(Edge));
	GLuint *indices = malloc(36 * sizeof(GLuint));
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
			glm::vec3(0.0f),
			VERT_ORIGINAL);
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
			tris[t++] = Triangle(tmpVerts, tmpEdges, i / 2);
		}
		if((i % 2) == 0){
			faceCentres[i / 2] = (corners[sphereIndices[i * 3]] + corners[sphereIndices[i * 3 + 1]] +
								  corners[sphereIndices[i * 3 + 2]] + corners[sphereIndices[i * 3 + 5]]) * 0.25f;
			faceNormals[i / 2] = glm::normalize(faceCentres[i / 2] - centre);
		}
	}
	// remove duplicate edges
	Edge* realEdges = malloc(18 * sizeof(Edge));
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


int uploadMeshes(const Mesh* meshes, int numMeshes, GLuint meshVAO, GLuint meshVBO, GLuint meshEBO)
{
	glBindVertexArray(meshVAO);
	GLuint totalNumVerts = 0;
	GLuint totalNumIndices = 0;
	for (size_t i = 0; i < numMeshes; i++) {
		totalNumVerts += meshes[i].numVerts;
		totalNumIndices += meshes[i].numIndices;
	}
	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalNumIndices * sizeof(GLuint), adjustedIndices, GL_STATIC_DRAW);
	free(adjustedIndices);
	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, light));
	glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE, stride, (void*)offsetof(Vertex, flags));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	return totalNumIndices;
}

void renderMeshes(int numIndices, GLuint meshVAO, GLuint meshVBO, GLuint meshEBO)
{
	glBindVertexArray(meshVAO);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, light));
	glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE, stride, (void*)offsetof(Vertex, flags));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
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
	GLuint boxoidVAO, boxoidVBO, boxoidEBO;
	GLuint sphereVAO, sphereVBO, sphereEBO;
	glGenVertexArrays(1, &boxoidVAO);
	glGenBuffers(1, &boxoidVBO);
	glGenBuffers(1, &boxoidEBO);
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
		
		// Swap pointers with the thread that copies the data
		// This lets the rendering thread and physics simulation both run at full tilt
		// without having to wait for each other
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
		if(sharedData.numSpheres >= 5) {
			// uniforms
			glUseProgram(boxoidProgram);
			glm::vec3 center = glm::vec3(sharedData.spheres[5].position[0], sharedData.spheres[5].position[1], sharedData.spheres[5].position[2]);
			model = glm::translate(glm::mat4(1.0f), center);
			glUniformMatrix4fv(boxoidModelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(boxoidCenterLoc, center[0], center[1], center[2]);
			// separate rotation from translation so we can rotate normals in the vertex shader
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 3.14f * sin(time / 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			glUniformMatrix4fv(boxoidRotationLoc, 1, GL_FALSE, glm::value_ptr(rotation));
			glm::vec3 diffuseComponent = vectorize(sharedData.renderMisc.materials[exampleBoxoids[0].material_idx].diffuse);
			glm::vec3 emissiveComponent = vectorize(sharedData.renderMisc.materials[exampleBoxoids[0].material_idx].emissive);
			float vibe = 3.0;
			diffuseComponent = glm::vec3(std::pow(diffuseComponent.r, vibe), std::pow(diffuseComponent.g, vibe), std::pow(diffuseComponent.b, vibe));
			glUniform3fv(boxoidDiffuseLoc, 1, glm::value_ptr(diffuseComponent));
			glUniform3fv(boxoidEmissiveLoc, 1, glm::value_ptr(emissiveComponent));
		  
			static int numIndices = 0;
			// can use a better algorithm to dynamically update meshes when the geometry changes and in response to LOD
			// considerations
			Mesh meshes[7];
			if(numIndices == 0) {
				meshes[0] = boxoidToMesh(exampleBoxoids[0]);
				meshes[1] = boxoidToMesh(exampleBoxoids[1]);
				meshes[2] = tessellateMesh(&meshes[0], 0, &exampleBoxoids[0]);
				meshes[3] = tessellateMesh(&meshes[1], 0, &exampleBoxoids[1]);
				meshes[4] = tessellateMesh(&meshes[2], 1, &exampleBoxoids[0]);
				meshes[5] = tessellateMesh(&meshes[3], 1, &exampleBoxoids[1]);
				meshes[6] = tessellateMesh(&meshes[4], 2, &exampleBoxoids[0]);
			}
			numIndices = uploadMeshes(&meshes[sharedData.renderMisc.buttonPresses % 7], 1, boxoidVAO, boxoidVBO, boxoidEBO);
			renderMeshes(numIndices, boxoidVAO, boxoidVBO, boxoidEBO);
		}
		else {
			printf("numSpheres: %d\n", sharedData.numSpheres);
		}
		glBindVertexArray(0);
		glfwSwapBuffers(sharedData.window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &boxoidVAO);
	glDeleteBuffers(1, &boxoidVBO);
	glDeleteBuffers(1, &boxoidEBO);
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);
	glDeleteProgram(sphereProgram);
	glfwTerminate();
	return 0;
}

// thread safe entry point copies all the data to keep things dumb.
extern "C" void render(Sphere* spheres, size_t sphereCount, render_misc renderMisc) {
	pthread_mutex_lock(&sharedData.mutex);
	if(sharedData.nextBatch != NULL) {
		free(sharedData.nextBatch);
	}
	sharedData.nextBatch = (Sphere*)malloc(sizeof(Sphere) * sphereCount);
	memcpy(sharedData.nextBatch, spheres, sizeof(Sphere) * sphereCount);
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


