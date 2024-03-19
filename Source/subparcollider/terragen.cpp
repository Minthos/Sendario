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

double TerrainGenerator::getElevation(dvec3 pos) {
    return fnFractal->GenSingle3D(seed, pos.x, pos.y, pos.z);
}


