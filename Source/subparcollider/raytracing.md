2160 * 3840 * 120 = ~1 G pixels/sec
8.3 M pixels

if the number of composites is lower than the number of pixels it's faster to do ambient lighting on a per-composite level than per-pixel

not all-to-all but frustum casting, maybe 4 frustums per face?


The TLAS' main purpose is to tell which locations are empty and which intersect an object's bounding volume.
Ray traversal means intersection testing every node of the tree along the ray's path
How many levels we have to go up to reach a neighbor node is determined by the difference in coordinates between the current node and the neighbor node

64 bit occupancy field
32 bit union of index to children if occupancy is nonzero or intersections if occupancy is zero
32 bit index to aggregate emissiveness and opaqueness struct? (can represent solid objects and gas clouds)

when building the tree we can give each node 64 children but in most cases the number of actual children is very small so to save memory and memory bandwidth we can compress the data structure before copying it to the gpu

leaf nodes can have up to 64 intersections but the actual number should be less. distance from camera should determine hctree resolution at the lowest level. if it ends up being >64, give up I guess. throw out the least visible ones.


"hctree"
grid aligned
16 byte nodes
more computation, many leaves point to same object
ray packets can amortize per-node computation cost of traversal


"ray tracing for games"
32 byte nodes
bvh with aabb and two children nodes: children can overlap, aabb can fit to data, don't have to compute aabb for each intersection test


FIRST make something simple
a simple bvh/octree implementation in C++/glsl
then experiment with different algorithms, TLAS/BLAS, hctree/bvh
finally experiment with ambient light and volumetric rendering, frustum queries, ray packets..



-----

Traversing the TLAS by advancing a plane that covers the entire cross section of the frustum?
When a bounding box intersects the frustum, "rasterize" the bounding box to find which rays intersect it, then send those rays into the box's BLAS

OR

use ray packets of 16x16 or 8x8 rays, now we have to do more TLAS traversal but we can just throw all the rays first at the bounding box and then into the BLAS if they intersect the BB.

Transforming a ray packet by an object's rotation matrix doesn't require transforming all the rays, just transform the corner rays and interpolate.


-----
preparation
Copy incoming and outgoing light values from the previous frame's buffer parallel to the TLAS and cut all the values in half

First iteration (light emitters)
Cast a ray packet from every light source to every node in the TLAS, basically flood fill the TLAS
for each hit, add the ray's vector and the light's color to a buffer indexed by the node's index * num_lights

intermission (conversion pass)
For each node in the TLAS we can now calculate reflected light based on incoming light and the geometry and add it to outgoing light

second iteration (indirect light)
For each node in the TLAS, splat the outgoing light on nodes close enough to receive a meaningful dose

optionally repeat intermission and second iteration a few times to improve indirect lighting

-----

Ambient occlusion

ambient occlusion can be computed using only the BLAS and the incoming light from the cube map
but that means it can be precomputed

store it per vertex as 2x vec3?
store it as a lightmap?



