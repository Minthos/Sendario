#include <pthread.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h> 

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
    float roundness[6];
    int material_idx;
    unsigned int missing_faces; // could have been unsigned char, but padding is nice
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
    float orientation[4];
    float position[3];
    float scale;
    size_t num_children;
    Boxoid children[0];
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
} render_misc;

void startRenderer();
void render(Sphere* spheres, size_t sphereCount, render_misc renderMisc);
void stopRenderer();

bool isGameController(int joystick_index);
const char* getStringForButton(Uint8 button);

