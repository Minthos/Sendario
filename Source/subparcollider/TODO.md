## Near term TODO:

* basic camera control with gamepad
* quaternions to represent orientation, some refactoring of the physics engine
* spaceship controls and thrust, attitude control
* fix the rendering bug when camera is near/inside an object
* reactivate atmospheric drag but make it follow the celestial and not apply to the celestial
* visualize future trajectories with lines in colors of the rainbow, to about 1 orbital period

## Recently done:

* improved gravity (brute force solution is not scalable but good enough for now)
* create a skybox with the starfield.py shader
* make the sun really bright
* give planets and moons different colors
* implemented Verlet integration. It doesn't solve orbital stability completely but it improves it.
* fix remaining bug(s) the light calculation
* set the sun's position as origin of light for each object to render
* logarithmic depth in fragment shader
* transform objects to be centered on the camera before rendering
* "eliminated" physics bugs (partly by commenting out the drag and magnus force)
* installed valgrind from source because fuck ubuntu (it was easy and it works, 10/10 with rice)


## Medium term TODO:

* Make a basic space exploration prototype
 - A solar system with the sun, some planets, a circular "spaceship" with a gaslight drive
* w-space


## Even further out:

* spaceship design/construction
* ground vehicle/robot/factory design/construction
* simplified physics of an "MVP" suite of parts for designing things
* mining, refining and logistics
 - mining can be in several types: rich deposits in hard rock, sparse deposits in loose regolith, asteroids
 - logistics shouldn't be too tedious but scaling up should be time-consuming. we don't want players to get too big too fast.


## Low priority:

* implement tidal effects on celestial spin and orbit


## Ideas:

sparse octree/hctree with mass distribution of celestials for "global gravity" approximation in asteroid belts and such - linear time to construct the tree, then to evaluate the gravity in a specific location query the tree at resolution inversely proportional to distance

populate every visited galaxy with enough stars to render a skybox for the visited star systems and the universe with enough galaxies and nebulae to render a skybox for every visited galaxy. combinie the two skyboxes to show a realistic view of the universe from anywhere players travel.

should be feasible to have a hctree of each star system, another of each galaxy, and another of the enture universe. mass (gravity) and EM spectrum emissivity for each node in the tree. one tree should fit on an SSD.

of course all of this will be procedurally generated but we still have to cache it so we don't generate the same data over and over again

