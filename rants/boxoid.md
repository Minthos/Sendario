

= Boxoid rendering

"Boxoid" is my name for a parametric geometric shape that's specified by 8 corners (x, y, z), 12 roundness values (one for each edge) and a bit field specifying which faces to not bother rendering

```C
typedef struct {
    float corners[3 * 8]; // coordinates in object space
    float bulge[12]; // modify the roundness of each edge. if two edges on opposite sides of a face are rounded, the face inherits the roundness of the edge value that's closest to 0, along the axis defined by those edges. setting all 12 edges to 1 gives a spheroid shape. all to 0 gives a box shape. negative values gives a kiki shape. this way we can produce cylinders, capsules, cones and weird custom shapes.
    int material_idx; // placeholder
    unsigned int omit; // bit field specifying which faces to omit from rendering. could have been unsigned char, but padding is nice.
} Boxoid;

typedef struct {
    float orientation[4]; // w,x,y,z quaternion specifying the object's rotation from object space to world space
    float position[3]; // x,y,z coordinates in world space
    float scale; // default is 1
    size_t num_children; // how many boxoid shapes the object is composed of
    Boxoid children[0];
} Composite;
```

== So howtf do we render them?

 
Vertex shader:

// just the corners
send world space coordinates to fragment shader
send modified (taking bulge into account) world space coordinates to fragment shader
send clip space coordinates to fragment shader

// precomputed values to save work for the fragment shader
curvature of the adjacent faces


Fragment shader: 
figure out if the modified corner value causes the eye ray to miss the object
figure out if the modified corner value causes the eye ray to hit another face
figure out the surface normal at the intersection point


Uniforms:




