


Hydrological simulation

generate triangles for the planet down to idk, 8 initial faces subdivided to around 4k-64k ish each?
(that's 8 * 4^6 to 8 * 4^8) for a total of 32768 to 524288 triangles.

Highest level terrain mesh chunk at 2 subdivisions below the layer of 8. 8 * 4^2 = 128 chunks.


8 root nodes
16 grandchildren each = 128 "extended family chunks (ef)"

64k triangles per root triangle = 4k triangles per ef chunk = 524288 triangles in the ef chunks

the procgen algorithm makes about 100k triangles per second per core so we shouldn't generate this stuff
until we can cache it to disk.




