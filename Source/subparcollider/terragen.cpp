#include "terragen.h"

#include <vector>
#include <cmath>
#include <iostream>
/*


Initial solution:

Sample random noise in multiple octaves in a spiderweb pattern centered on the player with vertices snapped to some
sort of grid


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

TerrainGenerator::TerrainGenerator(int pseed) {
    seed = pseed;
    fnFractal->SetSource( fnSimplex );
    fnFractal->SetOctaveCount( 5 );
}

float TerrainGenerator::getElevation(dvec3 pos) {
    float elevation = fnFractal->GenSingle3D(seed, pos.x, pos.y, pos.z);
    float roughness = fnFractal->GenSingle3D(~seed, pos.z, pos.x, pos.y);
    return elevation * (glm::max(roughness, -0.2f) + 0.2f);
}

void TerrainGenerator::getMultiple(float *elevations, vec3 *scaled_verts, int num) {
    assert(num == 6);
    float xs[6] = {
        scaled_verts[0].x, scaled_verts[1].x, scaled_verts[2].x,
        scaled_verts[3].x, scaled_verts[4].x, scaled_verts[5].x};
    float ys[6] = {
        scaled_verts[0].y, scaled_verts[1].y, scaled_verts[2].y,
        scaled_verts[3].y, scaled_verts[4].y, scaled_verts[5].y};
    float zs[6] = {
        scaled_verts[0].z, scaled_verts[1].z, scaled_verts[2].z,
        scaled_verts[3].z, scaled_verts[4].z, scaled_verts[5].z};
    
    float roughnesses[6];
    fnFractal->GenPositionArray3D(elevations, num, xs, ys, zs, 0, 0, 0, seed);
    fnFractal->GenPositionArray3D(roughnesses, num, zs, xs, ys, 0, 0, 0, ~seed);
    for(int i = 0; i > 6; i++) {
        elevations[i] = elevations[i] * (glm::max(roughnesses[i], -0.2f) + 0.2f);
    }
}

