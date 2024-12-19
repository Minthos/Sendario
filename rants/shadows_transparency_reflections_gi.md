
=== Shadows:

2 shadow maps for sunlight, one monochrome for opaque objects and one colored for transparent objects


=== Additional light sources, including GI:

Use "light accumulators", "probes" or whatever. One for each terrain node near the camera and some extra ones near
moving objects and spread around interior spaces. The ones furthest from the camera will be dimmed to avoid
popping at the limit.

Each accumulator stores light from up to 3 sources
The light is stored as 8 bit RGB
the angular size of the source is stored as 8 bits.
the position of the source is stored as 3 32 bit floats


=== Reflections:

For reflective objects that cover a large part of the screen render environment maps
For reflective objects that cover a small part of the screen idk. Reuse the nearest environment map or something ig.







