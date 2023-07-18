## First feature: ship builder interface

Design ship sections (shape, material, thickness, purpose, initial tech level, other variables)

Each option has a materials cost tree and equipment requirement for the whole process (ore/mineral refinery, gas refinery, chip fab, hardware factory, utility drones), time estimate and energy use estimate.

Gather materials (mineral/gas deposits, salvage, trade), upgrade ship/base with new sections, upgrade existing sections.

### Sections:

* Weapon turret (business end curvature determines aiming angle, aiming angle restricts barrel length. Barrel must be contained inside the shape of the section. Lower aiming angle allows for lighter servos and more space efficient layout.
* Main thruster (rcs comes for free)
* Reactor
* Heat sink
* Battery
* Fuel tank
* Cargo hold
* Armor/bulkhead
* Chip fab
* Hardware factory
* Mineral refinery
* Gas refinery

Your ship starts with a fusion reactor, rcs thrusters, and 2 utility drones. It's obviously not enough for a journey into space. You have to add a fuel tank and an engine at least. Fortunately you get to design your own starting ship and build it for free within reasonable limits. Good luck!

## Tech tree

Each item requires ingredients produced by the things that are required to enable it at the same level as the item to build.
For example a level 0 chip fab requires level 0 refined minerals, level 0 hardware components and level 0 refined gases. Upgrading it to level 1 requires the level 1 versions of those ingredients.

In addition to building the ingredients needed to craft something the same sections also produce the consumables used by the thing, if it uses consumables. Exceptions are the consumables listed in the tech tree.

Utility drones assemble parts made by the various factories into finished sections, move everything that needs to be moved, mine gas and minerals from deposits and repair your stuff.

### Tech level 0 enables:
RCS thrusters
Arcjet rocket engine (high thrust low isp, electric)
Propellant tank
Hull
Cargo hold
Heat sink
Hardware factory
Mineral refinery
Gas refinery

### Hardware factory enables:
Utility drones level 0
Sensors
Conveyor belt

### Mineral refinery enables:
Coilgun
Armour plate (put on anything to increase hit points)
Railroad track
Gas pipe

### Hardware factory + mineral refinery enables:
Solar panel
Trains

### Gas refinery enables:
Dumbfire rockets
Chemical explosive warheads
Fuel
Propellant

### Hardware factory + gas refinery enables:
Life support

### Hardware factory + both refineries enables:
Chip fab
Battery
Autocannon
Chemical rocket engine (high thrust, very low isp, self-powered)

### Chip fab + gas refinery enables:
Stealth coating

### Chip fab + hardware factory enables:
Utility drone tech level 1
Homing missiles
Proximity mines

### Tech level 1 enables:
Everything can be upgraded to level 1 if their prerequisites are at level 1.

### Hardware factory level 1 enables:
Defense drones
Attack drones (spacecraft, aircraft, tanks, mechs, boats, submarines)
Sensor drones

### Hardware factory level 1 + both refineries level 1 enables:
Chip fab level 1
Nuclear reactor (medium power low thermal efficiency)
Nuclear warheads
Lasers
Ion drive (low thrust high isp, electric)
Nuclear rocket engine (high thrust medium isp, self-powered)

### Chip fab level 1 + hardware factory 1 enables:
Utility drone tech level 2

### Tech level 2 enables:
Everything can be upgraded to level 2 if their prerequisites are at level 2.
Gaslight drive (electric, allows hyperspace travel)

### Hardware factory level 2 + both refineries level 2 + any reactor level 2 enables:
Fusion reactor (high power high thermal efficiency)
Fusion warheads
Fusion rocket engine (medium thrust high isp, hybrid electric/self-powered)

### Chip fab level 2 + hardware factory 2 enables:
Utility drone tech level 3

### Tech level 3 yada yada

### Mineral refinery level 3 enables:
Space elevator

### Hardware factory level 3 + both refineries level 3 + chip fab level 3 + any reactor level 3 enables:
Antimatter beam
Antimatter warheads
Antimatter battery (very high burst power, expensive fuel, high thermal efficiency)

---

I guess utility drone tech level 2 with gaslight drive is where the game opens up and tech progress should slow down to a crawl. Fusion is the major milestone after that, until then it's only upgrading stuff and building/fighting. The end of the tech tree is antimatter. I guess I don't really want players to reach it in the persistent world, though if antimatter is obscenely expensive it may not be completely game-breaking to have it exist. Will have to tweak the game balance.

Before that it would be cool to have a singleplayer/coop mechwarrior-like campaign almost, fight some battles over control of a moon, either tech up and run away or tech up and win. It could devolve into a total annihilaton-inspired experience where the "commander" (your spaceship) arrives on a celestial body and has to bootstrap a base and fight a war with not much story behind it. Should get AI to come up with some story ideas.

Your ship can be a mobile base, big and heavy and fielding its own army of drones - or it can be fast and nimble and geared for combat and exporation while the army projects power from a fixed base somewhere.

It would be interesting to try game modes where players can fill different roles, from piloting various machines in multiplayer combat to directing strategy to exploring and questing.

I don't want to add too much complexity but it should be feasible to implement different game types by varying the resource amounts and time needed to build various things and the distance between player starting locations.

In a fast-research game with starting locations on different planets the game gets interesting when the gaslight drive is unlocked and until then it's about building up industry (econ + tech).

In games with players starting on the same planet or moon it may be good to have few resources in the starting area so they are incentivized to fight over the best deposits.

This tech tree is farly simplistic and linear. It will be interesting to see what kind of meta evolves regarding ship designs and strategies when everyone has basically the same tech tree progression.

In an MMO persistent world I would like to have upgrades that players can research on specific ship designs or specific sections to tweak the stats. A system where players can spend compute and R&D effort to get a random bonus after a random time. Applying multiple upgrades to the same item gives diminishing returns. Maybe use AI to generate a description of the upgrades based on the stats of the upgrade and the item/section's description and calculate overlap by having AI rate how much the descriptions sound like they should overlap. More similar = more stacking penalty.




