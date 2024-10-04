
Incompatible ideas: massively multiplayer, rts with unlimited number of units per player, detailed physics simulation of the internal workings of vehicles and components, free to play

performance optimizations increase the total amount of cool shit we can do, but there must still be some limits

One fun game mode would be to have one unit per player, physics simulation of player-crafted components (with as much as possible pre-computed so it doesn't all have to be computed real-time during battles), pay to play, fleet battles with thousands of players.

No FTL travel initially. Advanced-ish fusion technology can make the travel time between planets in a star system ~hours. ~minutes between moons of a planet. Atmospheric flight is slower due to aerodynamics but the atmosphere provides unlimited propellant and coolant.

Make new planets available on a regular schedule depending on how crowded planets get. The planets should be within lightyears of each other. That gives me the option to introduce FTL travel to the tech tree a few years into the game.



----

Difficulty modes

I want the game to have hardcore difficulty. Permadeath, character stays in the game world when the player logs out,
death from hunger, thirst, asphyxiation, heat, cold, radiation, poisoning etc. Slow healing from injuries.
Basically become a robot or die trying.

This will alienate a lot of players so I should provide an easy difficulty mode "sunshine and roses" without permadeath,
without biological needs, with "normal" logout and respawn mechanics

----

Trade, resources and technology

Each tier of technology should require combining multiple resources
Those resources should be further apart and harder to find for the higher tiers of tech

abundant on habitable celestials: air, water, dirt, sand, wood, stone, salt, plants, animals
common metals and chemicals slightly rarer: iron, nickel, copper, coal, oil, nitrates, limestone
rare metals: uranium, thorium, tungsten, silver, aluminium, lithium, titanium, cobalt, gallium, neodymium, niobium
even rarer metals: gold, platinum
rare crystals: heisenbergite, diamond, sapphire
rare gases: helium-3, tritium, deuterium

I shouldn't require all of those to advance a tech level but several of them at each level
some can be substituted for each other, for example [uranium, thorium] as nuclear fuel and [coal, oil] as fossil fuel

air + water -> nitrates
[plants, animals, nitrates] -> fertilizer
fertilizer -> nitrates
nitrates + [coal, oil] -> explosives
nitric acid + sulfuric acid + cellulose -> guncotton
limestone + sand + water -> concrete
[copper, aluminium] + plastic -> electric wires
[cobalt, neodymium] + iron -> magnets
electric wires + magnets -> electric motors
[iron, aluminium] + electric wires -> internal combustion engines
sand -> solar panels


----

Transparent crystals

inspired by the crystals in isqswjwki55a1.png it would be cool to have crystals that are transparent.

however, if they intersect the terrain surface they will reveal the surface and not properly look like they are
embedded in it. I need a (ray tracing? some kind of z-buffer hack?) shader that can pass through terrain
intersections inside the object and render the intersected terrain's material on the backface of the object instead.

I should have some planets/moons rich in these colorful crystals and make them useful to the players. Maybe bonuses
to lasers and armor vs lasers, maybe some of them are heisenbergite

----


Shadow rendering

start at the terrain node closest to the star on the spheroid
flood-fill the terrain and store two values per node: how tall obstruction the node causes and the elevation
at which the node is not obstructed


----


For each occupied and visible node of the physics tree, do a tree traversal towards the star and store a pointer to all
objects that intersect (doesn't need to be accurate)

render all the objects to a texture with a special shader that only stores the alpha channel and
z value, from the star's point of view, at a resolution that depends on how far the node is from
the camera. not rendered = not illuminated.

this is now the shadow map for that node


----


Point defense weapons

Should prioritize:

#1 incoming nuclear warheards
#2 incoming guided missiles approaching high-value targets
#3 incoming unguided projectiles predicted to hit high-value targets
#4 remaining incoming stuff

priority should be weighted depending on the chance of hitting each incoming thing and the time remaining to defeat it




----


Procgen

Generate the basic heightmap of the planet, refine it on demand
no, don't generate a heightmap. instead generate only vertices for the local minima and maxima
as little data as possible at first, add more detail only when needed
Pick a sea level
Rainfall comes from the sea, preferentially deposits on mountains
Hot weather gives more rain, cold weather retains the rain as snow and ice
Fallen rain follows valleys and becomes rivers
Wet areas produce vegetation



----

Galaxy and star generation

I should download the GAIA GDR3 dataset and convert it to a binary format. PostGIS may be a good idea for this data.

Sending the entire dataset to each client is probably overkill. Instead maybe some 3d textures
representing the Milky Way and nearby galaxies and progressively lower detail 3d textures of the rest of the universe.

Then the server can generate a point cloud representing stars within reasonable travel distance
of the player to be rendered individually and the client can generate a skybox from the 3d textures
for anything outside that range.


