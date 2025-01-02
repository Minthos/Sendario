
### Terrain chunking

The terrain mesh that gets sent to the gpu needs to be split up so that parts of it can be updated without having
to update the whole mesh.

A chunk should probably be a triangle at the level of the terrain tree that gets us above 1/4 of the target number of triangles per chunk. If a chunk exceeds the target triangle count it gets split into 4.
Ok so knowing that, we can declare the planet's 8 faces as the initial chunks and then subdivide them as soon as they
exceed the target triangle count.
Terrain LOD updates should probably then be generated per chunk, with a uniform LOD for the whole chunk.
The client has to decide which chunk to refine next, when to take a breather, and when to retire old chunks.
Choosing which to refine is probably as simple as sorting them by size divided by distance from the player
Now any idle cpu core can just pick the first chunk from the list and start generating.
The finished chunks can be placed in a queue for uploading to the gpu.


