#include <algorithm>
#include <iostream>
#include <vector>

#include <cassert>
#include <cstdint>

#include <glm/glm.hpp>

using glm::dvec3;
using glm::vec3;

struct unionvec3 {
    union {
        float radio;
        float r;
        float red;
        float uv;
        float x;
    };
    union {
        float microwave;
        float g;
        float green;
        float xray;
        float y;
    };
    union {
        float ir;
        float b;
        float blue;
        float gamma;
        float z;
    };
};

struct vec9 {
    unionvec3 lo; // frequencies below the visible spectrum
    unionvec3 rgb;
    unionvec3 hi; // frequencies above the visible spectrum
};

int16_t d2hi(double in) {
    if(in > 0.0) return (int16_t)(in + 0.5);
    if(in < 0.0) return (int16_t)(in - 0.5);
    return 0;
}

struct hvec3 {
    int16_t x;
    int16_t y;
    int16_t z;

    hvec3() { x = 0; y = 0; z = 0; }
    hvec3(int16_t in) { x = in; y = in; z = in; }
    constexpr hvec3(int16_t px, int16_t py, int16_t pz) : x(px), y(py), z(pz) {}
//    constexpr hvec3(int16_t px, int16_t py, int16_t pz) { x = px; y = py; z = pz; }
    hvec3(dvec3 in) {
        x = d2hi(in.x);
        y = d2hi(in.y);
        z = d2hi(in.z);
    }

    static hvec3 max(hvec3 lhs, hvec3 rhs) {
        return hvec3{
                glm::max(lhs.x, rhs.x),
                glm::max(lhs.y, rhs.y),
                glm::max(lhs.z, rhs.z)};
    }

    static hvec3 min(hvec3 lhs, hvec3 rhs) {
        return hvec3{
                glm::min(lhs.x, rhs.x),
                glm::min(lhs.y, rhs.y),
                glm::min(lhs.z, rhs.z)};
    }
};

constexpr hvec3 operator*(const hvec3 lhs, const int16_t rhs) {
    return hvec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

constexpr hvec3 operator/(const hvec3 lhs, const int16_t rhs) {
    assert(rhs != 0);
    return hvec3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
}

constexpr hvec3 operator+(const hvec3 lhs, const int16_t rhs) {
    return hvec3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
}

constexpr hvec3 operator-(const hvec3 lhs, const int16_t rhs) {
    return hvec3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
}

constexpr hvec3 operator+(const hvec3 lhs, const hvec3 rhs) {
    return hvec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

constexpr hvec3 operator-(const hvec3 lhs, const hvec3 rhs) {
    return hvec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}


constexpr dvec3 operator*(const dvec3 lhs, const double rhs) {
    return dvec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

constexpr dvec3 operator/(const dvec3 lhs, const double rhs) {
    assert(rhs != 0.0);
    double factor = 1.0 / rhs;
    return dvec3(lhs.x * factor, lhs.y * factor, lhs.z * factor);
}

constexpr dvec3 operator+(const dvec3 lhs, const double rhs) {
    return dvec3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
}

constexpr dvec3 operator-(const dvec3 lhs, const double rhs) {
    return dvec3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
}

constexpr dvec3 operator+(const dvec3 lhs, const vec3 rhs) {
    return dvec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

constexpr dvec3 operator-(const dvec3 lhs, const vec3 rhs) {
    return dvec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

struct Mesh {
    // put some vertices and triangles here
    //
    // also a texture and shadow map
};

struct CollisionShape;

struct Motor {
    double max_force;
    double min_force; // negative values for motors that can produce power in both directions
    double max_power; // both positive and negative if applicable
};

struct Joint {
    dvec3 pos;
    dvec3 axis;
    Motor motor;
};

struct JointHinge: Joint {
    double target_angle;
    double angle;
    double max_angle;
    double min_angle;
};

struct JointSlide: Joint {
    double target_extension;
    double extension;
    double max_extension;
    double min_extension;
};

struct Joint6dof: Joint {
    dvec3 target_angle;
    dvec3 angle;
};

enum physics_state {
    active,
    sleeping,
    immovable // immovable objects are things like terrain and buildings. They can become active in dire circumstances.
};

struct PhysicsObject {
    CollisionShape *shape; // for objects with no parent this should be the total shape of the object including all descendants.
    // for objects with a parent it should only represent a single shape, not its children.
    PhysicsObject *parent;
    Joint *joint; // joint can only represent the connection to parent. directed acyclic graph is the only valid topology.
    PhysicsObject **children; // both direct and indirect children

    enum physics_state state;
    double mass;
    dvec3 pos; // center of mass, not necessarily the center of the bounding volume
    dvec3 vel;
    dvec3 rot;
    dvec3 spin;
};

struct CollisionShape {
    double radius;
    dvec3 pos; // center of the bounding sphere
    Mesh *convex_hull;
    PhysicsObject *object;
};

struct AABB {
	hvec3 max;
	hvec3 min;
};

struct ctleaf {
    CollisionShape *shape;// 8 bytes
    // axis-aligned bounding box enclosing the bounding sphere of the shape
    hvec3 hi; // 6 bytes (total 14)
    hvec3 lo; // 6 bytes (total 20)
};

AABB calculateBounds(ctleaf* p, uint32_t first, uint32_t last) {
    AABB bounds;
    bounds.min = hvec3(32767);
    bounds.max = hvec3(-32768);
    for (uint32_t i = first; i <= last; ++i) {
        bounds.min = hvec3::min(bounds.min, p[i].lo);
        bounds.max = hvec3::max(bounds.max, p[i].hi);
    }
    return bounds;
}

struct ctnode {
    hvec3 hi; // 6 bytes
    hvec3 lo; // 6 bytes (total 12)
    uint32_t count; // 4 bytes (total 16)
    union{ // 4 bytes (total 20)
        uint32_t left_child;
        uint32_t first_leaf;
    };

    void setBounds(AABB bounds) { hi = bounds.max; lo = bounds.min; }

    void subdivide(ctnode* nodes, uint32_t* poolPtr, ctleaf* primitives, uint32_t first, uint32_t last) {
        if (first == last) {
            this->first_leaf = first;
            this->count = 1;
            return;
        }
        this->left_child = *poolPtr;
        *poolPtr += 2;
        ctnode *left = &nodes[left_child];
        ctnode *right = &nodes[left_child + 1];

        // sort the primitives and split them in a dumb way that is not optimal but better than worst case
        hvec3 size = hi - lo;
        if(size.y > size.x && size.y > size.z) {
            std::sort(primitives + first, primitives + last + 1, [](const ctleaf& a, const ctleaf& b) -> bool {
                return a.lo.x + a.hi.x < b.hi.x + b.lo.x;
            });
        }
        else if (size.y > size.x && size.y > size.z) {
            std::sort(primitives + first, primitives + last + 1, [](const ctleaf& a, const ctleaf& b) -> bool {
                return a.lo.y + a.hi.y < b.hi.y + b.lo.y;
            });
        }
        else {
            std::sort(primitives + first, primitives + last + 1, [](const ctleaf& a, const ctleaf& b) -> bool {
                return a.lo.z + a.hi.z < b.hi.z + b.lo.z;
            });
        }
        uint32_t split = (first + last) >> 1;

        left->setBounds(calculateBounds(primitives, first, split));
        right->setBounds(calculateBounds(primitives, split + 1, last));
        left->subdivide(nodes, poolPtr, primitives, first, split);
        right->subdivide(nodes, poolPtr, primitives, split + 1, last);
        this->count = last - first + 1;
    }
};

ctnode* constructBVH(ctleaf* leaves, int N) {
	AABB globalBounds = calculateBounds(leaves, 0, N);
	ctnode* nodes = (ctnode*) malloc(sizeof(ctnode) * (2 * N));
	ctnode& root = nodes[0];
	root.setBounds(globalBounds);
	root.count = N;
	uint32_t poolPtr = 1;
	root.subdivide(nodes, &poolPtr, leaves, 0, N - 1);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, TLASBuffer);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ctnode) * poolPtr, nodes, GL_STATIC_DRAW);
	return(nodes);
}


#define MAX_MAX_DEPTH 16
struct CollisionTree {
    dvec3 pos;
    ctnode *root = 0;
    std::vector<CollisionShape> shapes;

    CollisionTree(dvec3 origo) {
        pos = origo;
        root = 0;
    }

};

struct Celestial {
    dvec3 pos;
    dvec3 vel;
    dvec3 rot;
    dvec3 spin;
    double mass;
    double radius;
    double surface_temp_min;
    double surface_temp_max;
    vec9 radiance;

    Celestial *nearest_star;
    std::vector<Celestial> orbiting_bodies;
};

struct Zone {
    Zone *neighbors[6]; // +x, -x, +y, -y, +z, -z

    CollisionTree tree;
    Celestial *frame_of_reference;
};


/*

"collision shape" should probably just be simplified to an AABB for each object from the tree's
perspective. Then when testing object collisions we can test the polygon mesh if the AABBs intersect.
An object's AABB should be registered in the collision tree at every node it intersects.
This will make the tree costlier to update but easier to query.
If a vehicle's bounding sphere is far from anything else (i.e. not near the surface of any
celestial and not near other flying objects) we can enter it at the highest level of the tree that
doesn't contain anything else.

Collision shapes should be the building blocks of buildings and vehicles.
Exterior-facing shapes should be flagged as exterior-facing, interiors can be ignored

Box
Cylinder
Sphere
Capsule
Low-polygon convex hull mesh

Each collision shape should have one connection to a physics object. By default the connection
should be to the main object. Exceptions will be when we want parts to move together as a unit,
for example a turret with a gun attached or a spring with a wheel attached.


Collision: bounce, deform, absorb, destroy

Objects at rest should be marked as "sleeping" but when they are not fully at rest but still touching
the ground they should have a force applied to them from the ground equalling the force of gravity
pulling them down. This way they don't try to accelerate into the ground and repeatedly collide with it.

Really the collision code should not only take into account an object's velocity and rotation but
also its force and torque values.


Multiplayer considerations and server memory constraints:

The server doesn't actually need to give a shit about terrain until the player modifies it.
The server can generate resources when the player scans or excavates a location, store player
modifications to terrain, and other than that just not represent terrain at all.
The client can stream ship telemetry to the server including altitude over terrain. Collisions with
terrain can be computed client-side and cheaters can be detected by doing spot checks of their telemetry.
Stealth gameplay can be achieved by clients reporting the average horizon angle in 8 directions to the server
and the server comparing that to the sensor strength and location of other players nearby.

A celestial can be represented as the top-level object with the standard fields: orbit, mass, albedo,
mineral composition, atmospheric composition, temperature range, magnetic field and so on.

An alternative is to make the procedural generation algorithm really fast and only cache a small amount
of data for each celestial. Then we can cache the terrain immediately surrounding each player and vehicle
and generate more whenever the physics simulation runs collision checks against terrain that isn't cached.
This has the added benefit of making the client procgen performance better too.

On the client to enhance visual quality we can maybe pregenerate some assets like trees, rocks, textures
for various biomes and reuse them instead of generating unique textures and geometries for every little shrub

Layering 3-4 textures on top of each other can produce lots of variety from less initial data. Can also
reuse texture patterns with different colors.


*/

// 5800x3d
// 32 KB L1 data cache at 4 or 5 cycles
// 512 KB L2 cache at 14 cycles
// 96 MB shared L3 cache at 47 cycles


