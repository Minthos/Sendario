
=== Shadows:

2 shadow maps for starlight, one monochrome for opaque objects and one colored for transparent objects


=== Additional light sources:

- Lights other than the nearest star project light into the scene by querying the terrain tree and the physics tree
for objects close enough to be affected in the direction of the light.
- This list of objects is stored in the light source.
- Each object on the list gets the light source added to its list of prominent light sources.
- render shadow pass for starlight
- render the scene geometry
- render reflections for large flat reflective surfaces
- build a list of objects that have actually been rendered
- build a list of objects who are in shadow OR whose second most prominent light source is at least 20% percent as bright as the starlight
- mark the pixels belonging to these objects as "to be shaded"
- for these objects, create a list of their light sources
- for these light sources, create a list of their combined object lists
- from this list of light sources and objects, build a TLAS and BLASes for ray tracing
- send a primary ray for each pixel to be shaded to the 3 most prominent light sources for that pixel's object


=== Reflections:

For reflective planes that cover a large part of the screen render the scene from a camera flipped across the plane
For smaller reflective objects idk, omit reflections, use environment map, ray trace



