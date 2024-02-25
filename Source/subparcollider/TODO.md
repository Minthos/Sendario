
# physics stuff


Entities should have a 64-bit physics object that does not directly interact with other physics objects
it's subject to gravity, aerodynamic drag and accumulated acceleration from its components

Components should have a 32-bit physics object that interacts with other 32-bit physics objects
rigid-body physics, joints, springs, deformation, destruction

For now just stick with SphericalCow I guess

# wish list

 - rigid body and soft body physics, skeletal physics system with joints, springs, motors etc
* start designing a terrain system and game logic, ignore rendering
* crafting system with 1st person controls and snap to grid (0.5x0.5x0.5m default)
 - grid is not locked to terrain grid
* bytecode script (struct with enum + payload) for giving objects behavior
* semi-opaque horizontal plane with grid lines position the cursor vertically. moves up and down with keyboard shortcuts or mouse wheel.


# To do
## Near term TODO:

* fix bugs in rigid body collision code
* add autocannon

## Medium term TODO:

* Tech tree, mining, crafting and various equipment
* improve mining, refining and logistics
 - build bases on celestials and in orbit
 - mining can be in several types: rich deposits in hard rock, sparse deposits in loose regolith, asteroids
 - logistics shouldn't be too tedious but scaling up should be time-consuming. we don't want players to get too big too fast.
* merge in universe procgen stuff with resource deposits
* Make a basic space exploration prototype
* deprioritized tasks
 - reactivate atmospheric drag but make it follow the celestial and not apply to the celestial
 - improve trajectory visualization: future trajectories with lines in colors of the rainbow with quadratic timestep to about 1 orbital period
* more interesting celestials with textures and normal maps?
* rendering text
* rendering velocity vectors
* navigation interface should enlarge nearby celestials, ships and structures and show name tags near the most prominent ones of each kind
* spaceship with a shape other than a sphere
 - user interface for editing boxoids
 - it would be nice to set some boxoids to not be rendered in the composite
* rigid-body physics with multiple connected boxoids
 - maybe actually prioritize this because it's more fun than the UI stuff, but first get our existing code to compile and run
 - also, you know, a very basic but functioning editing interface so we can build multi-boxoid spaceships for testing
* Ground vehicle using physics model and 4 wheels (rotation limited to 1 axis): dump truck with a cargo boxoid
 - rigid body physics with linked spherical cows
 - springs with adjustable rebound and compression damping
* Use The Sims as inspration for the ship designing experience
 - import/export json without restarting the client
 - snap to grid that snaps whatever you're doing to the grid
 - x and y shrinks/enlarges the grid size
 - context-aware menu. Anything immediately useful easily accessible and more stuff in subtrees, user configurable with scripting "support" (i.e. we have documented where in the source code you can hardcode your own menu options into the menu).
y: repeat last action, redo
* Use redis to hold server state, json file to hold client configuration and backup of all the composites and random seeds.



## Recently done:

* Multiple boxoids. UI for adding a boxoid adjacent to another boxoid.
* Boxoids each get a spherical cow. The Entity class can represent a single boxoid or a single composite.
- Collision detection algo should first consider the composite's bounding sphere and if that collides
  it can proceed to check which boxoid collides first.
* visual 3d grid (or maybe just a single planar surface) to see the scale of our spaceship while designing it
 - I'll make this a composite
 - 3d grid of boxes/spheres axis-aligned, optional lines connecting them
* position camera based on ship's orientation instead of ship's velocity
* rotate the ship's self-thrust vector by the ship's orientation
* w-space
* renderer that renders objects with orientation
* bounding boxes from hctree to compositecod
 - codable boxoids, composites, spheres
 - moving the data from swift to cpp
 - reading the data from json into swift
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


## Even further out:

* ground vehicle/robot/factory design/construction
* simplified physics of an "MVP" suite of parts for designing things


## Low priority:

* switch to Unreal Engine or Godot
* render exhaust plumes
* boxid support in the physics engine
* more creative use of boxoids in the physics engine for things like explosions, magnetic fields, gas clouds, exhaust plumes
* some refactoring of the physics engine
* implement tidal effects on celestial spin and orbit
* it would be fun to model centre of mass shifting as propellant moves around in the tank. workshop should at least
  show dry and wet centres of mass. maybe we just assume tanks have built-in pistons or bladders to avoid slosh and cavitation.


## Ideas:

sparse octree/hctree with mass distribution of celestials for "global gravity" approximation in asteroid belts and such - linear time to construct the tree, then to evaluate the gravity in a specific location query the tree at resolution inversely proportional to distance

populate every visited galaxy with enough stars to render a skybox for the visited star systems and the universe with enough galaxies and nebulae to render a skybox for every visited galaxy. combinie the two skyboxes to show a realistic view of the universe from anywhere players travel.

should be feasible to have a hctree of each star system, another of each galaxy, and another of the enture universe. mass (gravity) and EM spectrum emissivity for each node in the tree. one tree should fit on an SSD.

of course all of this will be procedurally generated but we still have to cache it so we don't generate the same data over and over again

## New shit

Spatial tree around a star: arranged by spherical coordinates with the star as centre. This way we make it easier to conduct intersection tests between objects and the star. The only inconvenience this causes us is that we must compute some spherical-cartesian conversions per object per frame. And we get to do the fun part which is to make a spatial tree and use it for ray casting.


## PVP mechanics

Smoke clouds will be an essential part of large fleet combat, they need to be first-class citizens of the physics engine
They block beam weapons, which are the only weapons that can hit anything at any distance with no chance of evading or active countermeasures.
They will have to be cleared by pressure waves from things like nuclear explosions, they will need temperature, pressure, velocity.

Healing in combat in other games? Hah! In this game stuff is supposed to get blown up. You can repair if you survive the battle.


## Planets

Having plenty of boring, sterile planets with few resources is a feature. They are good hiding spots for players who want to avoid others. "Boring" doesn't have to be taken to an extreme, they can have interesting geological features and lots of variety in their chemical composition and aesthetics, but there shouldn't be any npcs, quests, lifeforms, exotic resources or anything like that on them. Just rock, meteor craters, optionally lakes and oceans.

Habitable planets and moons are where the content is mainly supposed to be. I should make sure the procgen algorithm is really good. Having beautiful, varied, interesting planets to explore will add incredible value to the game. I don't want to just mimic reality, I want to have an alternative reality that's simplified but still offers great variety. For example the matter distribution between elements in a star system or on a planet. It can deviate from realism but it should still produce a wide variety of visually interesting results and be somewhat inspired by the laws of physics.

## Deformable terrain

YES! I want players to be able to dig caves underground, flatten terrain, build roads, tunnels, bridges and such. Allowing the players to alter the environment adds so much "free" gameplay and emotional attachment to the game world.


