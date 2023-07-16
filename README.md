# Takeoff Sendario

Project status: Very far from ready

The interesting stuff is actually in the Source/subparcollider folder.
The Unreal Engine stuff is not being developed.

Everything is subject to change without notice.

Licence to be determined (open source).

---
### the *ahem* "physics" "engine"
The so-called physics engine is a lo-fi approximation inspired by real world physics. Most of the features are missing
and will likely stay missing.

### rendering
The renderer is made with c++, opengl, glut and glm. It's meant to be low on features so I can focus on the gameplay. Of course it offers a tempting outlet for my urge to tinker.

The most interesting feature is the ability to render "boxoids" which are paremeterized geometric primitives that can have a range of useful and interesting shapes. I intend to let the user assemble boxoids into more complex shapes like spaceships, factories, vehicles, space stations and so on. The plan was to do the boxoid geometry transformation in a shader but I ended up doing it withtessellation because I'm too dumb to figure out how to do it in a shader. It looks a bit ugly when the detail level goes up so for now staying at 2 to 3 levels of tessellation looks best. Good for the cpu too to keep the detail level low. There are some obvious performance enhancements I will do to the renderer later when I have more of the gameplay implemented.

It can also render spheres as a special case, though currently without texturing. It's used to render planets, moons and stars. This is done completely in the fragment shader with no tessellation.

There are no shadows, the lighting calculation is extremely simple lambertian but with physically-based values for light intensity. Currently only one light source is supported but I plan to upgrade the lighting at some point. I have some ideas I want to try.

### controls
Currently the only input supported is gamepad. At some point mouse and keyboard support should also be addded but it's low priority for me personally. Pull requests welcome.

### windows support
Not implemented. Pull requests welcome.

### ios/android/mac support
Not implemented. Pull requests welcome.

## Gameplay

### State of gameplay implementation

First I have to build a ui for designing a spaceship..

Some of the gameplay is handled by the physics engine but there's a lot of work to be done.

### intended gameplay

Be an ai.
Start on a planet with established industry.
Your mission is to have presence on as many celestial bodies as you can.
Start by designing a spaceship.
Launch it and try not to die.
Don't worry, you can make more copies of yourself. Try again until you succeed.
Land on a moon or something.
Look for metal deposits.
Your ship has a fusion reactor. Use it as power source.
Mine resources, build infrastructure, produce fuel and equipment.
Let's have some rocks with hostile fauna, environmental hazards and such to make it interesting, stellaris-style.
When you have built a fusion reactor for your base you can go to the next rock to colonize.
You can manage the base remotely and use it to build more stuff including spaceships.
There will be other players out there too but everyone gets to colonize an uninhabited exoplanet for their first base.

First person gameplay for controlling the spaceship, mining resources and so on.
Remotely controlling bases needs a more strategy game like interface.
Designing hardware should also be a significant part of the game.
PVP combat, trade and politics should be as emergent as possible.
PVE can be designed for singleplayer, small and large groups.


