## First feature: ship builder interface

Design ship sections (shape, material, thickness, purpose, initial tech level, other variables)

Each option has a materials cost tree and equipment requirement for the whole process (ore/mineral refinery, gas refinery, chip fab, hardware factory, utility drones), time estimate and energy use estimate.

Gather materials (mineral/gas deposits, salvage, trade), upgrade ship/base with new sections, upgrade existing sections.

### Sections:

* Weapon turret (business end curvature determines aiming angle, aiming angle restricts barrel length. Barrel must be contained inside the shape of the section. Lower aiming angle allows for lighter servos and more space efficient layout.
* Main thruster (rcs comes for free)
* Reactor
* FTL drive
* Heat sink
* Battery
* Fuel tank/cargo hold/armor/bulkhead
* Chip fab
* Hardware factory
* Mineral refinery
* Gas refinery

Your ship starts with a fusion reactor, ftl drive, rcs thrusters, and 2 utility drones. It's obviously not enough for a journey into space. You have to add a fuel tank and a rocket engine at least. Fortunately you get to design your own starting ship and build it for free within reasonable limits. Good luck!

example von neumann probe that cools the reactor and ftl drive with main thruster propellant:
```
                          ------------------------------
     / armour | propellant                              tank | battery | chip fab | cargo hold | mineral refinery \
 gun | armour | propellant                              tank + ftl drive - heat sink - reactor + fuel         tank + main thruster
     \ armour | propellant                              tank | hardware factory | cargo   hold | gas     refinery /
                          ------------------------------
```

## Tech tree

Each item requires ingredients produced by the things that are required to enable it at the same level as the item to build.
For example a level 0 chip fab requires level 0 refined minerals, level 0 hardware components and level 0 refined gases. Upgrading it to level 1 requires the level 1 versions of those ingredients.

In addition to building the ingredients needed to craft something the same sections also produce the consumables used by the thing, if it uses consumables. Exceptions are the consumables listed in the tech tree.

Utility drones assemble parts made by the various factories into finished sections, move everything that needs to be moved, mine gas and minerals from deposits and repair your stuff.

### Tech level 0 enables:
- RCS thrusters
- Arcjet rocket engine (high thrust low isp, electric)
- Propellant tank
- Hull
- Cargo hold
- Heat sink
- Hardware factory
- Mineral refinery
- Gas refinery

### Hardware factory enables:
- Utility drones level 0
- Motors, actuators, +++
- Sensors
- Conveyor belt

### Mineral refinery enables:
- Coilgun (5 km/s)
- Armour plate (put on anything to increase hit points)
- Railroad track
- Gas pipe

### Hardware factory + mineral refinery enables:
- Solar panel
- Trains

### Gas refinery enables:
- Dumbfire rockets
- Chemical explosives
- Fuel
- Propellant

### Hardware factory + gas refinery enables:
- Life support (not used for anything since the game has no lifeforms)

### Hardware factory + both refineries enables:
- Chip fab
- Battery
- Autocannon (3 km/s)
- Chemical rocket engine (high thrust, very low isp, self-powered)

### Chip fab + gas refinery enables:
- Stealth coating

### Chip fab + hardware factory enables:
- Utility drone tech level 1
- Homing missiles
- Proximity mines

### Tech level 1 enables:
- Everything can be upgraded to level 1 if their prerequisites and ingredients are at level 1.

### Mineral refinery lv 1:
- coilgun -> railgun (10 km/s)

### Hardware factory level 1 enables:
- Defense drones (short range, fast and cheap)
- Attack drones (longer range, spacecraft, aircraft, ground vehicles, mechs, boats, submarines)
- Sensor drones

### Hardware factory level 1 + both refineries level 1 enables:
- Chip fab level 1
- Nuclear reactor (medium power low thermal efficiency)
- Nuclear warheads
- Lasers
- Ion drive (low thrust high isp, electric)
- Nuclear rocket engine (high thrust medium isp, self-powered)
- Autocannon -> Advanced Autocannon (5 km/s)

### Chip fab level 1 + hardware factory level 1 enables:
- Utility drone tech level 2

### Tech level 2 enables:
- Everything can be upgraded to level 2 if their prerequisites and ingredients are at level 2.
- Gaslight drive (electric, allows hyperspace travel)

### Mineral refinery lv 2:
- Railgun lv 2 (20 km/s)

### Hardware factory level 2 + both refineries level 2 enables:
- Advanced Autocannon -> Ultra Autocannon (8 km/s)
- Chip fab lv 2

### Hardware factory level 2 + both refineries level 2 + any reactor level 2 enables:
- Fusion reactor (high power high thermal efficiency)
- Fusion warheads
- Fusion rocket engine (medium thrust high isp, hybrid electric/self-powered)

### Chip fab level 2 + hardware factory level 2 enables:
- Utility drone tech level 3

### Tech level 3 enables:
- Everything can be upgraded to level 3 if their prerequisites and ingredients are at level 3.

### Mineral refinery level 3 enables:
- Space elevator
- railgun -> plasma gun (30 km/s)

### Hardware factory level 3 + both refineries level 3 + chip fab level 3 + any reactor level 3 enables:
- Antimatter beam
- Antimatter warheads
- Antimatter reactor (very high burst power, expensive fuel, high thermal efficiency)

---

The game should be fun. Let's give the starting ship access to a single small fusion rocket engine but no weaponry beyond level 0.
This will give them freedom to travel far but if they want to cause damage they have to play the game.

Pacing
The first tech level should be revealed through a story-driven campaign or should be relatively quick up to combat drones.
10 minutes build time for each of the first buildings + 20 minutes of resource gathering?
Nuclear tech level should be reachable within ~1 day, so that means about 12 hour upgrade time on each of the refineries.
Maybe a week to get to lv.2 and another week to build a gaslight drive?

so
lv 0: 1 minute drone, 5 minutes ship part or hw factory, 10 minutes refinery, 30 minutes chip fab
lv 1: 1 hour drone, 6 hours ship part or hw factory, 12 hrs refinery, 2 days chip fab
lv 2: 3 days drone, 1 week ship part or hw factory, 2 weeks refinery, 1 month chip fab
lv 3: 1 month drone, 2 months ship part, 6 months refinery, 2 years chip fab

If material input costs and difficulty of producing/sourcing ingredients scale somewhat in line with construction costs
but power level differences are less extreme we should see most of pvp action happen using cheaper parts (lv 0 to 2).


---
It would be cool to have a singleplayer/coop mechwarrior-like campaign almost, fight some battles over control of a moon or asteroid, either run away or tech up and win. In fast game modes it could be a total annihilaton-inspired experience where the "commander" (your spaceship) arrives on a celestial body and has to bootstrap a base and fight a war with not much story behind it. Should get AI to come up with some story ideas.

Your ship can be a mobile base, big and heavy and fielding its own army of drones - or it can be fast and nimble and geared for combat and exporation while the army projects power from a fixed base somewhere.

It would be interesting to try game modes where players can fill different roles, from piloting various machines in multiplayer combat to directing strategy to exploring and questing.

I don't want to add too much complexity but it should be feasible to implement different game types by varying the resource amounts and time needed to build various things and the distance between player starting locations.

In a fast-research game with starting locations on different planets the game gets interesting when the gaslight drive is unlocked and until then it's about building up industry (econ + tech).

In games with players starting on the same planet or moon it may be good to have few resources in the starting area so they are incentivized to fight over the best deposits.

This tech tree is farly simplistic and linear. It will be interesting to see what kind of meta evolves regarding ship designs and strategies.



## 2nd feature
In an MMO persistent world I would like to have upgrades that players can research on specific ship designs or specific sections to tweak the stats. A system where players can experience points, R&D effort and compute to get a random bonus after a random time. Applying multiple upgrades to the same item gives diminishing returns. Maybe use AI to generate a description of the upgrades based on the stats of the upgrade and the item/section's description and calculate overlap by having AI rate how much the descriptions sound like they should overlap. More similar = more stacking penalty.

Track the origin of every boxoid and every composite. When an item is part of equipment used to earn xp, the user, the builder, and the inventor of that item (can be the same or different players) each get some xp. The xp is tied to the (item design, player) pair. Players can use this xp as input material in r&d projects for that item, with each project having a chance to discover an upgrade that can be applied to the item. This means that players who design, manufacture and use a few popular items get to enjoy more available upgrades for those items than players using experimental, exotic and bespoke designs. I think the user should get most of the xp, maybe 80%, with 10% going to the builder and 10% to the inventor. Disallow transfer/sharing of upgrade recipes between players, but allow trading of the finished item with the upgrade applied. Designs for boxoids and composites will be automatically shared with other players because they are used for rendering. May as well build a system around copying and modifying each others' designs.



