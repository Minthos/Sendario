
Shadow rendering

For each occupied and visible node of the physics tree, do a tree traversal towards the star and store a pointer to all
objects that intersect (doesn't need to be accurate)

render all the objects to a texture with a special shader that only stores the alpha channel and
z value, from the star's point of view, at a resolution that depends on how far the node is from
the camera. not rendered = not illuminated.

this is now the shadow map for that node


----


Point defense weapons

Should prioritize:

#1 incoming nuclear warheards
#2 incoming guided missiles approaching high-value targets
#3 incoming unguided projectiles predicted to hit high-value targets
#4 remaining incoming stuff

priority should be weighted depending on the chance of hitting each incoming thing and the time remaining to defeat it




----


Procgen

Generate the basic heightmap of the planet, refine it on demand
no, don't generate a heightmap. instead generate only vertices for the local minima and maxima
as little data as possible at first, add more detail only when needed
Pick a sea level
Rainfall comes from the sea, preferentially deposits on mountains
Hot weather gives more rain, cold weather retains the rain as snow and ice
Fallen rain follows valleys and becomes rivers
Wet areas produce vegetation



----

Galaxy and star generation

I should download the GAIA GDR3 dataset and convert it to a binary format. PostGIS may be a good idea for this data.

Sending the entire dataset to each client is probably overkill. Instead maybe some 3d textures
representing the Milky Way and nearby galaxies and progressively lower detail 3d textures of the rest of the universe.

Then the server can generate a point cloud representing stars within reasonable travel distance
of the player to be rendered individually and the client can generate a skybox from the 3d textures
for anything outside that range.


