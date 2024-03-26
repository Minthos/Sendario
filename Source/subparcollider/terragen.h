#pragma once
#include "FastNoise2/include/FastNoise/FastNoise.h"
#include <glm/glm.hpp>

using glm::dvec3;
using glm::vec3;

void noisetest();
class TerrainGenerator {
public:
    int seed;
    double radius;
    float roughness;
    FastNoise::SmartNode<FastNoise::Simplex> fnSimplex;
    FastNoise::SmartNode<FastNoise::FractalFBm> fnFractal;

    TerrainGenerator(int pseed, float proughness);
    float getElevation(dvec3 pos);
    void getMultiple(float *elevations, vec3 *positions, int num);
};



