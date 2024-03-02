
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



