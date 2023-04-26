#include <pthread.h>

typedef struct {
    float x, y, z;
    float radius;
    void* material;
} sphere;

typedef struct {
    float camDirection[3];
    float fov;
} render_misc;

void startRenderer();
void render(sphere* spheres, size_t sphereCount, render_misc renderMisc);
void stopRenderer();

