I think I should implement the physics engine in a compute shader to avoid having to transfer thousands of transformation matrices between cpu and gpu every frame. Then the game should be able to cope with a much higher number of moving objects and limbs.


static draw for terrain tiles and non-deforming objects (only change the transformation matrices)
dynamic draw for objects that are undergoing deformation

"deformation" can be defined as the vertices of an object moving relative to the object's own coordinate space in more complex ways than what can be done by changing the object's transformation matrix, typically caused by collision but
could also be caused by other TBD phenomena.

physics tick

1. all actors begin their actions (applying forces and changing velocities)
2. all physics objects (passive and active) update their positions according to their velocities (and keep the previous position for use in step 4)
3. broad phase box-box and box-terrain intersections
4. narrow phase triangle-triangle intersections (compute two transformation matrices that will transform one object's triangles into another object's coordinate space before and after the position updates in step 2 were applied
5. push on all colliding triangles as if they are not connected to anything
6. push back based on depth of penetration and material properties like strength and compliance (XPBR \*)
7. triangles that move more than some threshold determined by the material's hardness don't fully return to their original position. that means we have a destructive collision and may need to subdivide the triangle to represent its new shape.
8. remember that corners are infinitely sharp in the simulation and that can cause deviation from expected outcomes
9. calculate how much volume is displaced by each triangle's movement
10. send shockwaves created by the displaced volume?
11. recalculate moment of inertia for the whole object
12. generate a report for each destructive collision, containing the following:
    - timestamp
    - object ids of the two objects
    - mass, position, velocity, orientation and spin of both objects before and after the collision
    - if any pieces broke off: object id, mass, position, velocity, orientation and spin of the new pieces
    - updated object space positions for any vertices that have been permanently dislocated

\*) matthias müller-fischer uses many sub-steps (20 ish to 100 ish) and solving constraints locally instead of many iterations per sub-step


classic benchmark: pyramid of 650 stacked cubes resting on a flat surface and being affected by gravity, measure the position of the top cube over time and plot the horizontal and vertical deviation on a chart. ideally they should wobble a bit vertically but not horizontally and settle on a stable deviation determined by the softness of the cubes relative to the pull of gravity.




