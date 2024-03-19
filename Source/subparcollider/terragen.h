#pragma once
#include "FastNoise2/include/FastNoise/FastNoise.h"
#include <glm/glm.hpp>

using glm::dvec3;

void noisetest();
class TerrainGenerator {
    int seed;
    double radius;
    FastNoise::SmartNode<FastNoise::Simplex> fnSimplex = FastNoise::New<FastNoise::Simplex>();
    FastNoise::SmartNode<FastNoise::FractalFBm> fnFractal = FastNoise::New<FastNoise::FractalFBm>();

public:
    TerrainGenerator(int pseed);
    float getElevation(dvec3 pos);
};



