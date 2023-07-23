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
* Vehicle Assembly Building

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

"Efficiency" unless otherwise stated means energy efficiency. How much of the input energy (electric + consumables)
becomes useful output. The rest becomes heat that the ship's cooling system has to get rid of.

### Tech level 0 enables:
- RCS thrusters
- Arcjet rocket engine 5 hp/t (300:1 twr @ 4 km/s ve, max 10 km/s ve, electric, twr +100 * lv)
- Propellant tank 50 hp/t hp/t (mass * 0.8^lv)
- Hull 100 hp/t (mass * 0.7^lv)
- Cargo hold 100 hp/t (mass * 0.7^lv)
- Heat sink 20 hp/t (mass * 0.5^lv)
- Hardware factory 50 hp/t builds 0.1 t/t per hour
- Mineral refinery 50 hp/t refines 2 t/t per hr
- Gas refinery 50 hp/t refined 2 t/t per hr
- VAB 50 hp/t

### Hardware factory enables:
- Utility drones level 0 20 hp/t refines 1 t/t per hr
- Motors, actuators, +++
- Sensors 20 hp/t
- Conveyor belt

### Mineral refinery enables:
- Coilgun 50 hp/t (5 km/s) 50% efficiency. simple and cost-effective way to damage a target
- Armour plate 200 hp/t (mass * 0.6^lv)
- Railroad track 100 hp/t
- Gas pipe 100 hp/t

### Hardware factory + mineral refinery enables:
- Solar panel 20 hp/t (20% efficiency + 20% * lv)
- Trains 100 hp/t

### Gas refinery enables:
- Dumbfire rockets 5 hp/t (configurable engine, propellant and warhead)
- Chemical explosives 5 hp/t volatile
- Smoke grenades 20 hp/t nonvolatile (hinders sensors, stops directed energy weapons) duration * 2/lv (researchable
	upgrades change the emissive/diffuse)
- Propellant 20 hp/t nonvolatile (+10% density/lv, upgrades boost thrust and isp)

### Hardware factory + gas refinery enables:
- Life support (not used for anything since the game has no lifeforms)

### Hardware factory + both refineries enables:
- Chip fab (5 hp/t) builds 1 PFLOPS * t * h
- Battery (20 hp/t)
- Autocannon 50 hp/t (3 km/s) 80% efficiency (configurable warhead: kinetic, flak, explosive, smoke, nuclear, fusion, antimatter)
	Autocannons are good for saturating an area with danger but are defeated by maneuverability and speed.
	Effective point defense against unguided missiles.
- Chemical rocket engine 5 hp/t (450:1 twr, 3.7 km/s ve, self-powered)

### Chip fab + gas refinery enables:
- Stealth coating

### Chip fab + hardware factory enables:
- Utility drone tech level 1 30 hp/t
- Homing missiles 5 hp/t configurable engine, propellant and warhead
- Proximity mines 5 hp/t stealth coated, configurable warhead, propulsion optional

### Tech level 1 enables:
- Everything can be upgraded to level 1 if their prerequisites and ingredients are at level 1.

### Mineral refinery lv 1:
- coilgun -> railgun (10 km/s) 60% efficiency

### Hardware factory level 1 enables:
- Defense drones (short range, fast and cheap)
- Attack drones (longer range, spacecraft, aircraft, ground vehicles, mechs, boats, submarines)
- Sensor drones

### Hardware factory level 1 + both refineries level 1 enables:
- Chip fab level 1 builds 10 PFLOPS * t * h
- Nuclear reactor 50 hp/t (10 MW/t thermal, lv 1 @ 50% efficiency (+ 10%/lv) = 5 MW/t electric and a lot of radiators)
- Nuclear warheads 10 hp/t 20kg and up, nonvolatile, very powerful. one is often enough to destroy a ship. large AOE.
- Lasers 20 hp/t 1 GW/t input (lv1 5% efficiency, lv2 10% efficiency, lv3 20% efficiency) essential defense against nuclear missiles.
- Ion drive 10 hp/t (0.4:1 twr @ 30 km/s ve, max 50 km/s ve, electric. +0.2 max twr and +20km/s max ve per level)
- Nuclear rocket engine 20 hp/t (40:1 twr @ 10 km/s ve, max 20 km/s ve, self-powered)
- Autocannon -> Advanced Autocannon (5 km/s) 90% efficiency (configurable warhead)

### Chip fab level 1 + hardware factory level 1 enables:
- Utility drone tech level 2 60 hp/t

### Tech level 2 enables:
- Everything can be upgraded to level 2 if their prerequisites and ingredients are at level 2.
- Gaslight drive 30 hp/t (electric, allows hyperspace travel)

### Hardware factory level 2 enables:
- Gamma beam cannon. 200 hp/t Detonates a tiny nuclear bomb inside a reflector that focuses the radiation from the blast
in the general direction of a target. This has obvious downsides but can cause a lot of damage especially to lightly
armored equipment and biological creatures. Large AOE.

### Mineral refinery lv 2:
- Railgun -> Advanced Railgun (20 km/s) 70% efficiency

### Hardware factory level 2 + both refineries level 2 enables:
- Advanced Autocannon -> Ultra Autocannon (8 km/s) 95% efficiency (configurable warhead)
- Chip fab lv 2 builds 100 PFLOPS * t * h

### Hardware factory level 2 + both refineries level 2 + any reactor level 2 enables:
- Fusion reactor 20 hp/t (25 MW/t thermal, 10% waste heat = 22.5 MW/t electric, 40 MW/t and 7% waste heat at lv3)
- Fusion warheads 10 hp/t 100 kg and up, nonvolatile, extremely powerful. Large AOE.
- Fusion rocket engine 10 hp/t (15:1 twr @ 20 km/s ve, max 100 km/s ve, hybrid electric/self-powered)

### Chip fab level 2 + hardware factory level 2 enables:
- Utility drone tech level 3 100 hp/t

### Tech level 3 enables:
- Everything can be upgraded to level 3 if their prerequisites and ingredients are at level 3.

### Mineral refinery level 3 enables:
- Space elevator 500 hp/t
- railgun -> plasma gun (30 km/s) 80% efficiency

### Hardware factory level 3 + both refineries level 3 enables:
- Chip fab lv 3 builds 1 EFLOPS * t * h

### Hardware factory level 3 + both refineries level 3 + chip fab level 3 + any reactor level 3 enables:
- Antimatter beam 20 hp/t 1 GW/t input, 99% thermal efficiency, volatile
- Antimatter warheads 10 hp/t small, extremely powerful, volatile. Large AOE.
- Antimatter reactor 5 hp/t (250 GW/t power, 2% waste heat) volatile
Antmatter is extremely expensive, usually not a cost effective way to fight unless the battle is very high stakes.
It also releases all its energy when the containment fails, unlike fusion and nuclear.

---

Fusion engine: 

Uses strong magnetic fields to compress an ion plasma containing fusion fuel until it fuses. Instead of using the
energy to produce electricity it uses it to accelerate the propellant.
The fusion fuel is hydrogen and helium isotopes. The propellant can be water, hydrocarbons, noble gases etc.

The fusion reaction is thermally limited, offering a very high isp low thrust "cruise" mode which uses minmal coolant
but relies almosts entirely on radiative cooling and a high thrust medium isp "boost" mode or "beast" mode that
uses more coolant to produce extra thrust when demanded.

Nuclear engine:

Runs propellant through a solid reactor core to heat the propellant and cool the reactor.
Simple design allows very high propellant flow rate and advances in material science has enabled exhaust velocity as
high as 25 km/s at low thrust and 10 km/s at max thrust. Propellant can be water, hydrocarbons, noble gases etc. The
fuel is solid uranium and stays inside the reactor. A single unit of fuel can last for years of operation.

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
Applying an upgrade to an item of a different tech level than the upgrade was researched for produces a -20%/abs(delta lv)
reduction in bonuses and +20%/abs(delta lv) increase in maluses.

Track the origin of every boxoid and every composite. When an item is part of equipment used to earn xp, the user, the builder, and the inventor of that item (can be the same or different players) each get some xp. The xp is tied to the (item design, player) pair. Players can use this xp as input material in r&d projects for that item, with each project having a chance to discover an upgrade that can be applied to the item. This means that players who design, manufacture and use a few popular items get to enjoy more available upgrades for those items than players using experimental, exotic and bespoke designs. I think the user should get most of the xp, maybe 80%, with 10% going to the builder and 10% to the inventor. Disallow transfer/sharing of upgrade recipes between players, but allow trading of the finished item with the upgrade applied. Designs for boxoids and composites will be automatically shared with other players because they are used for rendering. May as well build a system around copying and modifying each others' designs.

Research projects:
1. An intern experiments on it
 - cost: 10 xp, 1 PFLOPS * h, $10.000 r&d money, 1 of the item
 - Tier: 50% failure, 45% lv0, 4% lv1, 1% lv2
2. A team of engineers looks at the telemetry and addresses the most glaring issues
 - cost: 50 xp, 20 PFLOPS * h, $100.000 r&d money
 - Tier: 20% failure, 40% lv0, 35% lv1, 4% lv2, 1% lv3
3. The engineers get free reins
 - cost: 200 xp, 500 PFLOPS * h, $2M r&d money
 - Tier: 40% failure, 27% lv1, 30% lv2, 3% lv3
4. Reduce technical debt
 - cost: 1000 xp, 10 EFLOPS * h, $20M r&d money
 - Tier: 30% failure, 50% lv2, 20% lv3

upgrade tiers:
lv0: up to 2 bonuses, up to 1 malus, 1x strength
lv1: up to 3 bonuses, up to 2 maluses, 2x strength
lv2: up to 4 bonuses, up to 3 maluses, 3x strength
lv3: up to 5 bonuses, up to 4 maluses, 4x strength

All items can have the following upgrades:
- telemetry harness: boosts xp gain from using this item
- weight reduction
- hp increase
- cost reduction
- build time reduction

Upgrades that can be unlocked cover the major performance stats of an item, depending on item type:
- All items that generate heat can get efficiency bonus
- All weapons can get a damage bonus and rate of fire bonus.
- Weapons with sub-relativistic projectile velocity can get projectile velocity bonus
- Thrusters can get thrust and ve bonus
- Reactors can get output bonus
- Factories/refineries can get manufacturing speed bonus, material efficiency bonus, energy use bonus
- Utiliity drones can get productivity bonus, cargo capacity bonus, speed and range bonus



