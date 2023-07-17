#include <pthread.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h> 

/* New boxoid rendering pipeline:

3 buffers
vertices
indices
uniforms 

1. Renderer initialization. Create buffers for cuboid objects.
2. main.swift calls addRenderObjects([Complex]) -> [Int] and gets back the indices of the objects,
deleteRenderObjects[Int] to delete the objects.
3. addRenderObjects tessellates the boxoids. Should probably cache different detail levels of
the mesh and choose between them based on size/distance to maintain uniform detail level at all distances
4. Sort the boxoids each frame before copying to vertex buffer?

*/

// warning! this is also defined in the shader source, so changing it here requires changing it there as well.
// you should also change the size of the lights array in the swift source to match it.
#define MAX_LIGHTS 1

typedef enum {
    COMPOSITE,
    SPHERE,
    BOXOID,
    RIBBON
} shape_type;

typedef struct {
    float corners[3 * 8];
    float curvature[12]; // the curvature of each face in two axes per face. setting all 12 curvatures to 1 gives a spheroid shape.
// all to 0 gives a box shape. negative values gives a kiki shape. this way we can produce cylinders, capsules, cones and weird custom shapes.
    int material_idx; // placeholder
    unsigned int missing_faces; // bit field specifying which faces to omit from rendering. could have been unsigned char, but padding is nice.
} Boxoid;

typedef struct {
    float position[3];
    float radius;
    int material_idx;
} Sphere;

typedef struct {
    size_t num_points;
    int material_idx;
    float point[4 * 0]; // (x, y, z, width) * num_points
} Ribbon;

typedef struct {
    float orientation[4]; // w,x,y,z quaternion specifying the object's rotation from object space to world space
    float position[3]; // x,y,z coordinates in world space
    float scale; // default is 1
    size_t nb; // how many boxoid shapes the object is composed of
    Boxoid* b;
} Composite;

typedef struct {
    shape_type type;
    union {
	Boxoid boxoid;
	Sphere sphere;
	Ribbon ribbon;
	Composite composite;
    };
} shape_wrapper;

typedef struct {
	size_t id;
	shape_type type;
} Objref;

typedef struct {
    float position[3];
    float color[3];
} light_source;

typedef struct {
    float diffuse[3];
    float emissive[3];
} material;

typedef struct {
    float camForward[3];
    float camUp[3];
    float camPosition[3];
    float fov;
    light_source lights[MAX_LIGHTS];
    material* materials;
	int32_t buttonPresses;
} render_misc;

void startRenderer();
Objref submitObject(shape_wrapper* shape);
void updateObject(Objref obj, shape_wrapper* shape);// pass nullptr to delete
void render(Objref* oref, size_t nobj, Sphere* spheres, size_t numSpheres, render_misc renderMisc);
void stopRenderer();

bool isGameController(int joystick_index);
const char* getStringForButton(Uint8 button);

