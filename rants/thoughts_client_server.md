
Client transmits unit commands to the server
Client predicts physics for all units it's aware of
Server sends authoritative state updates to the client
Client retroactively applies updates and replays the prediction up to present time
Client does physics simulations for all units it's aware of in a single process
The client needs two physics states: One which is the physics state at the time of the latest update from the server,
and one which is the predicted physics state presented to the player.


Server does physics calculations for all units for all players
A process has a thread pool, threads pull collision trees out of a bag to work on
When load gets high we'll need multiple processes. Can do one process per star system or even one process per celestial.
Initially just a single process.



