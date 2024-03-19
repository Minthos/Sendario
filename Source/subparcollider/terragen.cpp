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



struct polar_vert {
    double lat;
    double lon;
    double elevation;
};

void noisetest() {
    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();

    fnFractal->SetSource( fnSimplex );
    fnFractal->SetOctaveCount( 5 );
    
    std::vector<float> noiseOutput(16 * 16 * 16);

    // Generate a 16 x 16 x 16 area of noise
    fnFractal->GenUniformGrid3D(noiseOutput.data(), 0, 0, 0, 16, 16, 16, 0.2f, 1337);  
    int index = 0;

    for (int z = 0; z < 16; z++)
    {
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                // do something with data (x, y, z, noiseOutput[index++]);          
            }       
        }
        double value = fnFractal->GenSingle3D(1337, 0.0f, 0.0f, (float)z);
        std::cout << value << "\n";
        //std::cout << noiseOutput[z] << "\n";
    }
}

TerrainGenerator::TerrainGenerator(int pseed, double pradius) {
    seed = pseed;
    radius = pradius;
    fnFractal->SetSource( fnSimplex );
    fnFractal->SetOctaveCount( 5 );
}

double TerrainGenerator::getElevation(dvec3 pos) {
    double length = glm::length(pos);
    if(length <= 0.0){
        return 0.0;
    }
    dvec3 onSphere = pos * (radius / length);
    return fnFractal->GenSingle3D(seed, onSphere.x, onSphere.y, onSphere.z);
}




