#include <vector>
#include <cmath>
#include <iostream>

#include "FastNoise2/include/FastNoise/FastNoise.h"


/*


Initial solution: Generate random noise in multiple octaves and call it a day, focus on the sphere/cube mapping and
data structure, stuff like that


Improvements to quality:

1. Each planet has a personality. Initially we can just set it to default.

2. Generate a geological history for each planet. Atmosphere reduces meteor impact frequency. Plate tectonics and volcanism deform the surface and can erase impact craters. Erosion ages the surface.

3. Create wet/dry and hot/cold areas, rivers and riverbeds, rockslides, glaciers, forests.

4. Placing individual rocks: Create them randomly and apply some earthquakes.



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

// include libFastNoise.a
// it would be cool to use terragen from planetside.co.uk but I want something fast and lightweight that can
// run on each player's computer more than I want really beautiful and detailed planets




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
        std::cout << noiseOutput[z] << "\n";
    }
}

