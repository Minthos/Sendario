#include <cstdint>
#include <vector>
//#include <glm/glm.hpp>

// shadows glm type
struct dvec3 {
    double x;
    double y;
    double z;
};

// shadows glm type
struct ivec3 {
    int x;
    int y;
    int z;
};

// returns the number of children by counting high bits in the bit field
int count_bits(uint64_t v) {
    int c = 0;
    while(v != 0) {
        v &= v - 1; // clear the least significant bit set
        c += 1;
    }
    return c;
}

// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
// expects a bit field with a single bit set to high
// returns the index of that bit
uint8_t whichBit(uint64_t input){
    uint64_t masks[] = {
        0xaaaaaaaaaaaaaaaa,
        0xcccccccccccccccc,
        0xf0f0f0f0f0f0f0f0,
        0xff00ff00ff00ff00,
        0xffff0000ffff0000,
        0xffffffff00000000
    };
    uint8_t bit = 0;
    for(uint32_t i = 0; i < 6; i++) {
        bit += (!(!(input & masks[i]))) << i;
    }
    return bit;
}

// returns an array of numbers indicating which bit in the bit field represents
// the child node at the same index in the children array
//
// example:
// bit_field 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 0000 0001 0001
// children [node, node, node]
// decode: [0,4,12]
// then you can zip decode with children and get something like [(0, node), (4, node), (12, node)]
void decode(uint64_t bit_field, uint8_t *results) {
    uint64_t v = bit_field;
    int i = 0;
    while(v != 0) {
        uint64_t prev = v;
        v &= v - 1; // clear the least significant bit set
        uint64_t diff = v ^ prev;
        results[i++] = whichBit(diff);
    }
    return;
}


struct Mesh {
    // put some vertices and triangles here
    //
    // also a texture and shadow map
};

struct CollisionShape;

struct Motor {
    double maxForce;
    double minForce; // negative values for motors that can produce power in both directions
    double maxPower; // both positive and negative if applicable
};

struct Joint {
    dvec3 origin;
    dvec3 axis;
    Motor motor;
};

struct JointHinge: Joint {
    double targetAngle;
    double angle;
    double maxAngle;
    double minAngle;
};

struct JointSlide: Joint {
    double targetExtension;
    double extension;
    double maxExtension;
    double minExtension;
};

struct Joint6dof: Joint {
    dvec3 targetAngle;
    dvec3 angle;
};

struct PhysicsObject {
    CollisionShape *shape; // for objects with no parent this should be the total shape of the object including all descendants.
    // for objects with a parent it should only represent a single shape, not its children.
    PhysicsObject *parent;
    Joint *joint; // joint can only represent the connection to parent. directed acyclic graph is the only valid topology.
    PhysicsObject **children; // both direct and indirect children

    double mass;
    dvec3 pos; // center of mass, not necessarily the center of the bounding volume
    dvec3 vel;
    dvec3 rot;
    dvec3 spin;
};

struct CollisionShape {
    double radius;
    dvec3 center; // bounding sphere enclosing the shape .. yes I know we also have an AABB
    // but not all collision shapes will have an AABB and also a sphere is a good starting point
    // for creating the AABB quickly when the object moves and rotates without having to transform
    // every vertex
    Mesh *convex_hull;
    PhysicsObject *object;
};

struct ctleaf {
    // axis-aligned bounding box enclosing the entire object
    ivec3 hi;
    ivec3 lo;
    CollisionShape *shape;
};

struct ctnode {
    union{
        uint64_t occupancy; // if occupancy is empty, the first and only child is a collision shape
        uint64_t num_leaves; // if the node is at max depth, occupancy instead is the number of leaves
    };
    union{
        ctnode *first_child; // could also have been a uint32
        ctleaf *first_leaf;
    };
};

// resolution at bottom level: 4m
// 1 level up: 16
// 2 -> 64
// 3 -> 256
// 4 -> 1024 (one zone? maybe zones should be bigger, 1 km is fairly short range for mech/aircraft/spaceship weapons)
// 5 -> 4096
// 6 -> 16384
// 7 -> 65536 (nice round number..)
// A tree shouldn't be too big, it should represent a frame of reference for the physics engine
// We want to keep the numbers small to counter floating point stability issues
// Better to have many trees and move more frequently between them
struct CollisionTree {
    dvec3 origo;
    int numItems;
    ctnode *root;
};

// I want the tree to be packed in a contiguous array so that lookups can benefit maximally from
// cpu cache and prefetching. The physics engine can do all the position updates in a single
// batch and then we can update and repack the tree before we do lookups.
// Additional data like lighting/shadow information and terrain data can be kept in one or more
// separate arrays whose structures mirror the tree.

// 5800x3d
// 32 KB L1 data cache at 4 or 5 cycles
// 512 KB L2 cache at 14 cycles
// 96 MB shared L3 cache at 47 cycles

// Collision: bounce, deform, absorb, destroy

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

*/


