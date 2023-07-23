## Near term TODO:


* Make much of the interface a website hosted with redis, flask and tornado.
* Use redis to hold server state, json file to hold client configuration and backup of all the composites and random seeds.
* Players who spawn into the game can browse galaxies and star systems to spawn in via a website. If they just start
the client it will pick a random unvisited starting location within 50 LY of Earth.
* Use The Sims as inspration for the ship designing experience


// editing interface shuld have a visual axis aligned grid and a button to snap to grid that snaps whatever you're doing to the grid
// x and y shrinks/enlarges the grid size

// navigation interface should enlarge nearby celestials, ships and structures and show name tags near the most
// prominent ones of each kind

* spaceship with a shape other than a sphere
 - user interface for editing boxoids
 - actually it would be nice to set some boxoids to not be rendered in the composite..
 - reading the data from json into swift

* visual 3d grid (or maybe just a single planar surface) to see the scale of our spaceship while designing it
 - I'll make this a composite
 - would be good if I had textures for this or at least a new shader
 - can probably abuse the boxoid shader somewhat
 - 3 planes that cross at origo? each plane has thin lines that grow thicker the further from origo they are? draw coordinates near round numbers
* renderer that renders objects with orientation
* position camera based on ship's orientation instead of ship's velocity
* rotate the ship's self-thrust vector by the ship's orientation

## Recently done:

* bounding boxes from hctree to compositecod
 - codable boxoids, composites, spheres
 - moving the data from swift to cpp
 - rendering somewhat efficiently
 - boxoid rendering <-- whooo boy that took me down some rabbit holes..
 - cleaning up and improving the rendering code, more generality and less hardcoded stuff
* spaceship attitude control
* fix the rendering bug when camera is near/inside an object
* basic camera control with gamepad
* spaceship controls and thrust
* quaternions to represent orientation
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

## High priority right now

We can start coding an interface for building a spaceship using boxoids. Will have to get some json going. Swift is appropriate for that.
Use thumbsticks and throttle to elongate and adjust curvature in any direction.
Press cam button to switch between moving the camera and editing the shape.
3d crosshairs should intersect at the camera's target, aligned to world space or object space axis depending on context.

lt and rt buttons to cycle between shapes
a: save, edit, new, accept
x: cancel, undo
b: context-aware menu. Anything immediately useful easily accessible and more stuff in subtrees, user configurable with scripting "support" (i.e. we have documented where in the source code you can hardcode your own menu options into the menu).
y: repeat last action, redo


## Medium term TODO:

* Tech tree, mining, crafting and various equipment
* improve mining, refining and logistics
 - mining can be in several types: rich deposits in hard rock, sparse deposits in loose regolith, asteroids
 - logistics shouldn't be too tedious but scaling up should be time-consuming. we don't want players to get too big too fast.
* merge in universe procgen stuff with resource deposits
* Make a basic space exploration prototype
 - A solar system with the sun, some planets, a spaceship with a gaslight drive
* w-space
* deprioritized tasks
 - the shader
 - reactivate atmospheric drag but make it follow the celestial and not apply to the celestial
 - improve trajectory visualization: future trajectories with lines in colors of the rainbow with quadratic timestep to about 1 orbital period


## Even further out:

* ground vehicle/robot/factory design/construction
* simplified physics of an "MVP" suite of parts for designing things


## Low priority:

* render exhaust plumes
* boxid support in the physics engine
* more creative use of boxoids in the physics engine for things like explosions, magnetic fields, gas clouds, exhaust plumes
* replace the skybox with "snow" that's aligned to the world coordinate space
* some refactoring of the physics engine
* implement tidal effects on celestial spin and orbit


## Ideas:

sparse octree/hctree with mass distribution of celestials for "global gravity" approximation in asteroid belts and such - linear time to construct the tree, then to evaluate the gravity in a specific location query the tree at resolution inversely proportional to distance

populate every visited galaxy with enough stars to render a skybox for the visited star systems and the universe with enough galaxies and nebulae to render a skybox for every visited galaxy. combinie the two skyboxes to show a realistic view of the universe from anywhere players travel.

should be feasible to have a hctree of each star system, another of each galaxy, and another of the enture universe. mass (gravity) and EM spectrum emissivity for each node in the tree. one tree should fit on an SSD.

of course all of this will be procedurally generated but we still have to cache it so we don't generate the same data over and over again


