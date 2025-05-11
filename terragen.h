#pragma once
#include "FastNoise2/include/FastNoise/FastNoise.h"
#include <glm/glm.hpp>

// from SHA-Intrinsics/sha256-x86.c (AVX SIMD version)
void sha256_process_x86(uint32_t state[8], const uint8_t data[], uint32_t length);
// from SHA-Intrinsics/sha256.c (fallback for compatibility)
void sha256_process(uint32_t state[8], const uint8_t data[], uint32_t length);

using glm::dvec3;
using glm::vec3;

uint64_t sha256_hash(uint64_t seed_a, uint64_t seed_b);

// The sha256 generator produces very high quality pseudorandom numbers that are difficult to predict based
// on previous output
struct Prng_sha256 {
    uint32_t state[8];
    uint64_t message[8];
    uint64_t index;

    void init(uint64_t seed_a, uint64_t seed_b);
    uint64_t get();
    double uniform();
};

// The xoshiro generator is very fast but trivially predictable. That's usually an acceptable tradeoff.
struct Prng_xoshiro {
    uint64_t s[4];
    
    void init(uint64_t seed_a, uint64_t seed_b);
    uint64_t get();
    double uniform();
};

class TerrainGenerator {
public:
    int seed;
    double radius;
    float roughness;
    FastNoise::SmartNode<FastNoise::Simplex> fnSimplex;
    FastNoise::SmartNode<FastNoise::FractalFBm> fnFractal;

    TerrainGenerator(int pseed, float proughness);
    float getElevation(dvec3 pos);
    void getMultiple(float *elevations, float *roughnesses, vec3 *positions, int num, float typeslider);
};



