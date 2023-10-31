
A ship should be treated as a single physics object, but it can have multiple hitboxes to reduce the number of false positives in early collision detection and narrow down the location of impacts, kind of like the tlas/blas system in ray tracing.

When an impact is detected a stress analysis should be done to determine where the ship deforms as a result. It's not necessarily in the location that's hit. Especially if the hit point is a wheel or other movable part.

"Boxoid" is not the best geometric object. I like the idea of parametric geometry but I want something slightly more powerful.

Beam:
 rod (different cross section profiles)
 pipe (square/round, single/bundle)

Surface:
 flat (multiple vertices)
 bezier (multiple vertices)

Round:
 cylinder (straight/conical/bezier)
 sphere (perfect/oblate)
 dome (perfect/oblate)

Joint:
 rigid (weld, etc)
 rotating axis-constrained (limited or unlimited range of movement)
 rotating unconstrained (limited or unlimited range of movement)
 sliding (limited range of movement)
 rail (configurable angles of allowed departure)
 tethered (limited range of movement)

Parametric objects should have material properties.

compression (hardness, strength, damping, rebound damping)
elongation (hardness, strength, damping, rebound damping)
bending (hardness, strength, damping, rebound damping)

levels of deformity:
-- not damaged
0: maintain shape: force < [hardness, strength]
1: return to original shape: hardness < force < strength
-- damaged
2: permanently bend: force > strength > hardness
3: snap/shatter: force > hardness > strength

deformation progresses incrementally, force is diminished for each step
objects must be able to be in a state of tension from one physics tick to the next

a relatively simple proof of concept would be a spaceship made of several components getting hit by kinetic impacts from random directions and random mass/velocity and colliding with terrain at different velocities and gravities




The game's data structure should be binary serialization friendly: no pointers, only array indices to link objects together.

C and C++ play nice with this way of thinking.

Using swift for the Codable protocol was a mistake. Translating between swift and c++ is as much trouble as manually serializing/deserializing. Using SDL in swift is painful. 




