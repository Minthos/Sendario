
Near term TODO:

* gravity, wtf?? - may need to simulate tidal interactions (simplified obviously) for orbits to circularize instead of getting more eccentric over time
* improve logic for celestial bodies' orbits
* give planets and moons different colors
* do something about atmospheric drag


Recently done:

* fix remaining bug(s) the light calculation
* set the sun's position as origin of light for each object to render
* make the sun really bright
* logarithmic depth in fragment shader
* transform objects to be centered on the camera before rendering
* "eliminated" physics bugs (partly by commenting out the drag and magnus force)
* installed valgrind from source because fuck ubuntu (it was easy and it works, 10/10 with rice)


Long-range plans:

Make a basic space exploration prototype

A solar system with the sun, some planets, a circular "spaceship" with a gaslight drive

I have to implement:
spherical + cartesian coordinates - done!
gravity between celestials
w-space
a spaceship
controls

Even further out:

* spaceship design/construction
* ground vehicle/robot/factory design/construction
* simplified physics of an "MVP" suite of parts for designing things
* mining, refining and logistics
 - mining can be in several types: rich deposits in hard rock, sparse deposits in loose regolith, asteroids
 - logistics shouldn't be too tedious but scaling up should be time-consuming. we don't want players to get too big too fast.


