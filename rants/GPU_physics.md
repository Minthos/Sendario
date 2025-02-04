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
6. calculate how much force that would require, somehow* balance the force of the collision among the affected triangles per colliding pair of objects
7. give up and implement XPBD in gpu shader
8. that's actually a good idea anyway as a learning experience, can salvage ideas for my own implementation
9. calculate how much volume is displaced by each triangle's movement
10. send shockwaves created by the displaced volume?
11. recalculate moment of inertia for the whole object

*) matthias müller-fischer recommends using multiple sub-steps and solving rigid constraints locally, dunno how that translates to what I'm doing



classic benchmark: pyramid of 650 stacked cubes resting on a flat surface and being affected by gravity, measure the position of the top cube over time and plot the horizontal and vertical deviation on a chart. ideally they should wobble a bit vertically but not horizontally and settle on a stable deviation determined by the softness of the cubes relative to the pull of gravity.




