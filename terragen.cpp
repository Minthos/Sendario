#include "terragen.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <cstring>
/*


Initial solution:

Sample random noise in multiple octaves in a octahedron->triangles subdivision grid with LOD centered on the player


Voronoi cellular noise = continental plates



Improvements to quality:

1. Each planet has a personality. Initially we can just set it to default.

2. Generate a geological history for each planet. Atmosphere reduces meteor impact frequency. Plate tectonics and volcanism deform the surface and can erase impact craters. Erosion ages the surface.

3. Create wet/dry and hot/cold areas, rivers and riverbeds, rockslides, glaciers, forests.

4. Placing individual rocks: Create them randomly and apply some earthquakes.





https://en.wikipedia.org/wiki/Peatland
Peatlands are found around the globe, although are at their greatest extent at high latitudes in the Northern Hemisphere. Peatlands are estimated to cover around 3% of the globe's surface


*/

/* ranking of planets/moons

habitable - magnetic field, oxygen atmosphere, surface water, usually has life
wet - oxygen-free or toxic atmosphere, surface liquids (titan)
dry - thin atmosphere, no surface liquids, some subsurface liquids (mars)
subsurface ocean - may have atmosphere, frozen surface, thick layer of liquid below the surface, cryovolcanism (europa, triton, ganymede)
frozen - no atmosphere, frozen liquids on or below the surface (pluto, eris, ceres, callisto, +++)
barren - no atmosphere, no surface liquids, few or no subsurface liquids (mercury, the moon)
volcanic - some atmosphere, solid surface with ample volcanic activity (io)
tidally locked - extreme temperature difference between hot and cold side
thick - atmosphere too thick for easy colonization, surface conditions are hard to measure from orbit (venus)
lava - surface of liquid rock/metal
ice giant - crushing atmosphere, no surface (neptune, uranus)
gas giant - crushing atmosphere, no surface (jupiter, saturn)

*/


uint64_t sha256_hash(uint64_t seed_a, uint64_t seed_b) {
    uint64_t message[8] = {0};
    message[0] = seed_a;
    message[1] = seed_b;
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
#ifdef DEBUG
    sha256_process(state, (uint8_t*)message, sizeof(message));
#else
    sha256_process_x86(state, (uint8_t*)message, sizeof(message));
#endif
    return ((uint64_t*)state)[0];
}

void Prng_sha256::init(uint64_t seed_a, uint64_t seed_b) {
    index = 0;
    uint32_t grrstate[] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    memcpy((void*)state, (void*)grrstate, sizeof(grrstate));
    message[0] = seed_a ^ 0x8378df9d2b2cd3ae;
    message[1] = seed_b ^ 0x170719c5c2783be5;
    message[2] = 0xf958c6d70c69e6da;
    message[3] = 0x26f15592ce812bf6;
    message[4] = 0xe315b4238b4549e0;
    message[5] = 0x1f4ab9e07033c8ea;
    message[6] = 0x4d65c992c5759de0;
    message[7] = 0x8d93f95dcaef97bd;
    for(int i = 0; i < 16; i++) {
        get();
    }
}

uint64_t Prng_sha256::get() {
    if(index == 0)
#ifdef DEBUG
        sha256_process(state, (uint8_t*)message, sizeof(message));
#else
        sha256_process_x86(state, (uint8_t*)message, sizeof(message));
#endif
    index = (index + 1) % 4;
    return ((uint64_t*)state)[index];
}

double Prng_sha256::uniform() {
    constexpr uint64_t mask1 = 0x3FF0000000000000ULL;
    constexpr uint64_t mask2 = 0x3FFFFFFFFFFFFFFFULL;
    const uint64_t to_12 = (get() | mask1) & mask2;
    return (*(double*)(&to_12))-1.0;
}

void Prng_xoshiro::init(uint64_t seed_a, uint64_t seed_b) {
	s[0] = seed_a ^ 0x6a09e667bb67ae85;
	s[1] = seed_b ^ 0x3c6ef372a54ff53a;
    s[2] = 0x510e527f9b05688c;
    s[3] = 0x1f83d9ab5be0cd19;
}

static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t Prng_xoshiro::get() {
    const uint64_t result = rotl(s[0] + s[3], 23) + s[0];
    const uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);
    return result;
}

double Prng_xoshiro::uniform() {
    constexpr uint64_t mask1 = 0x3FFULL << 52;
    const uint64_t to_12 = (get() >> 12 | mask1);
    return (*(double*)(&to_12))-1.0;
}

TerrainGenerator::TerrainGenerator(int pseed, float proughness) {
    fnSimplex = FastNoise::New<FastNoise::Simplex>();
    fnFractal = FastNoise::New<FastNoise::FractalFBm>();
    seed = pseed;
    roughness = proughness;
    fnFractal->SetSource( fnSimplex );
    fnFractal->SetOctaveCount( 13 );
}

float TerrainGenerator::getElevation(dvec3 pos) {
    float elevation = fnFractal->GenSingle3D(seed, pos.x, pos.y, pos.z);
    float local_roughness = fnFractal->GenSingle3D(~seed, pos.z, pos.x, pos.y);
    return elevation * (glm::max(local_roughness, -roughness) + roughness);
}

void TerrainGenerator::getMultiple(float *elevations, float *out_roughnesses, vec3 *scaled_verts, int num, float typeslider) {
    assert(num == 12);
    float xs[12] = {  
        scaled_verts[0].x, scaled_verts[1].x, scaled_verts[2].x,
        scaled_verts[3].x, scaled_verts[4].x, scaled_verts[5].x,
        scaled_verts[6].x, scaled_verts[7].x, scaled_verts[8].x,
        scaled_verts[9].x, scaled_verts[10].x, scaled_verts[11].x};
    float ys[12] = {
        scaled_verts[0].y, scaled_verts[1].y, scaled_verts[2].y,
        scaled_verts[3].y, scaled_verts[4].y, scaled_verts[5].y,
        scaled_verts[6].y, scaled_verts[7].y, scaled_verts[8].y,
        scaled_verts[9].y, scaled_verts[10].y, scaled_verts[11].y};
    float zs[12] = {
        scaled_verts[0].z, scaled_verts[1].z, scaled_verts[2].z,
        scaled_verts[3].z, scaled_verts[4].z, scaled_verts[5].z,
        scaled_verts[6].z, scaled_verts[7].z, scaled_verts[8].z,
        scaled_verts[9].z, scaled_verts[10].z, scaled_verts[11].z};

    float roughnesses[12];
    fnFractal->GenPositionArray3D(elevations, 6, xs, ys, zs, 0, 0, 0, seed);
    fnFractal->GenPositionArray3D(&elevations[6], 6, &xs[6], &ys[6], &zs[6], 0, 0, 0, ~seed);
    fnFractal->GenPositionArray3D(roughnesses, 6, zs, xs, ys, 0, 0, 0, seed ^ 0xF0F0F0F0F0F0);
    fnFractal->GenPositionArray3D(&roughnesses[6], 6, &zs[6], &xs[6], &ys[6], 0, 0, 0, ~seed ^ 0xF0F0F0F0F0F0);
    for(int i = 0; i < 12; i++) {
        out_roughnesses[i] = (glm::max(roughnesses[i], -roughness) + roughness) * (1.0 - typeslider) +
            + (glm::max(roughnesses[(i + 6) % 12], -roughness) + roughness) * typeslider;
        float elevationtype1 = elevations[i] * (glm::max(roughnesses[i], -roughness) + roughness);
        float elevationtype2 = elevations[i] * (glm::max(roughnesses[(i + 6) % 12], -roughness) + roughness);
        elevations[i] = elevationtype1 * (1.0 - typeslider) + elevationtype2 * typeslider;
    }
}

