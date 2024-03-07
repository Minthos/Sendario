#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstring>

#include <glm/glm.hpp>

using glm::dvec3;
using glm::vec3;
using glm::dmat4;
using glm::dmat3;

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

int16_t d2hi(double in);

struct hvec3 {
    int16_t x;
    int16_t y;
    int16_t z;

    hvec3();
    hvec3(int16_t in);
    constexpr hvec3(int16_t px, int16_t py, int16_t pz) : x(px), y(py), z(pz) {};
    hvec3(dvec3 in);
    static hvec3 max(hvec3 lhs, hvec3 rhs);
    static hvec3 min(hvec3 lhs, hvec3 rhs);
};

constexpr hvec3 operator*(const hvec3 lhs, const int16_t rhs);
constexpr hvec3 operator/(const hvec3 lhs, const int16_t rhs);
constexpr hvec3 operator+(const hvec3 lhs, const int16_t rhs);
constexpr hvec3 operator-(const hvec3 lhs, const int16_t rhs);
constexpr hvec3 operator+(const hvec3 lhs, const hvec3 rhs);
constexpr hvec3 operator-(const hvec3 lhs, const hvec3 rhs);
constexpr dvec3 operator*(const dvec3 lhs, const double rhs);
constexpr dvec3 operator/(const dvec3 lhs, const double rhs);
constexpr dvec3 operator+(const dvec3 lhs, const double rhs);
constexpr dvec3 operator-(const dvec3 lhs, const double rhs);
constexpr dvec3 operator+(const dvec3 lhs, const vec3 rhs);
constexpr dvec3 operator-(const dvec3 lhs, const vec3 rhs);

struct dTri {
    uint32_t verts[3];
    glm::dvec3 normal;

    dTri();
    dTri(uint32_t pverts[3], dvec3* vertData, dvec3 center);
};

struct dMesh {
    dvec3 center;
    dvec3 *verts;
    dTri *tris;
    uint32_t num_verts;
    uint32_t num_tris;

    void destroy();

    private:
    dMesh(dvec3 pcenter, dvec3* pverts, uint32_t pnumVerts, dTri* ptris, uint32_t pnumTris);

    public:
    dMesh();
    static dMesh createBox(dvec3 center, double width, double height, double depth);
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
    dmat3 inertia_tensor;
    dvec3 pos; // center of coordinate transforms? the center of mass can shift so this should be the geometric center instead
    dvec3 vel;
    dvec3 rot;
    dvec3 spin;
};

struct CollisionShape {
    double radius; // radius of the bounding sphere for simplified math. Not all objects need a collision mesh.
    dMesh *convex_hull; // concave hull?
    PhysicsObject *object;
};

struct ctleaf {
    CollisionShape *shape;// 8 bytes

    // axis-aligned bounding box
    hvec3 hi; // 6 bytes (total 14)
    hvec3 lo; // 6 bytes (total 20)
};

struct AABB {
	hvec3 max;
	hvec3 min;
};

struct ctnode {
    hvec3 hi; // 6 bytes
    hvec3 lo; // 6 bytes (total 12)
    uint32_t count; // 4 bytes (total 16)
    union{ // 4 bytes (total 20)
        uint32_t left_child;
        uint32_t first_leaf;
    };

    void setBounds(AABB bounds);
    void subdivide(ctnode* nodes, uint32_t* poolPtr, ctleaf* primitives, uint32_t first, uint32_t last);
};


#define MAX_MAX_DEPTH 16
struct CollisionTree {
    dvec3 pos;
    ctnode *root = 0;
    std::vector<CollisionShape> shapes;

    CollisionTree(dvec3 origo);
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


