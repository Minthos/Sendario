# Takeoff Sendario

### Massively multiplayer sci-fi physics sandbox/game

(most of this has not been implemented yet)

Procedurally generated universe
Crafting system should be a physics sandbox inspired by KSP, satisfactory, the sims, RTS games
Resource gathering should be as simple as phyisically moving resources from where they found to the place where they will be used for crafting and chaining crafting steps together to create more advanced stuff
Combat should be as close to actual physics as feasible within a constrained compute and development budget
Focus on player-generated content and allowing for rich and complex player-player interactions
I would ideally implement some scripted content, NPCs, NPC factions, civilian cities, pve combat and so on if I had time.
To save development resources I will initially let players program NPCs to automate tasks and thus open the possibility for crafty players to build bot armies and try to claim significant territory from other players.
No inventory, everything has to be physically moved.
That sounds tediuous so players should program robots to do it for them.

----

The interesting stuff is actually in the Source/subparcollider folder.
The Unreal Engine stuff is not being developed.

Everything is subject to change without notice.

Licence to be determined (open source).


![screenshot](/Takeoff_000.jpg?raw=true "")



### rendering
The renderer is made with c++, opengl, glut and glm. It's meant to be low on features so I can focus on the gameplay. Of course it offers a tempting outlet for my urge to tinker.


### windows support
Not implemented. Pull requests welcome.

### ios/android/mac support
Not implemented. Pull requests welcome.

## Gameplay

### How is it about AI?

First person gameplay for controlling spaceships and vehicles, mining resources and so on. Can also automate NPCs
to do stuff. Maybe I will allow AI to play the game. It would be fun to have a thriving bot development community.
Remotely controlling bases needs a more strategy game like interface.
Designing hardware should also be a significant part of the game.
PVP combat, trade and politics should be as emergent as possible.
PVE content can be designed for solo players, small and large groups.


### what about concerns like griefing and bots ruining the game

I think the universe will be so big that anyone can just disappear to some far away system where no one will find them to build up their strength unchallenged.
I hope that bots can become a useful part of the game and not just a nuisance that destroys the game. By officially allowing bots to play the game I will enable them to fully embrace what they are, both strengths and weaknesses.
Some problems will be discovered with play testing and solved with experimentation.


### monetization idea

Initially the game can be free to play for everyone but if it becomes popular I can make a paid server and a free server, with the paid server getting priority on hosting resources and customer support.

Another idea is to separate players into two tiers: Paying subscribers can build as many fusion reactors as their resources allow them to, free players can have max 1 fusion reactor and it can never be stolen but it can be destroyed by the environment or other players. If it's destroyed they get a new one for free including a new ship. The ship is made from bioplastic so while it can't be farmed for construction materials it can be incinerated for a tiny energy gain. Even though they can't have more than 1 fusion reactor they can have nuclear reactors, solar panels, antimatter reactors and such. Solar is cheap but heavy, suitable for bases, stations and satellites that are near the habitable zone of a star. Nuclear works everywhere and is the cheapest and most reliable alternative to fusion power on spaceships and remote outposts. Antimatter is lightweight and extremely powerful but can only be used for short durations because of the waste heat and because the fuel is very expensive.








