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

typedef struct {
    float x, y, z;
    float radius;
    uint64_t material_idx;
} sphere;

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
void render(sphere* spheres, size_t sphereCount, render_misc renderMisc);
void stopRenderer();

bool isGameController(int joystick_index);
const char* getStringForButton(Uint8 button);
