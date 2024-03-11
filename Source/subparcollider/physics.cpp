#include <algorithm>
#include <boost/align/aligned_allocator.hpp>
#include <deque>
#include <iostream>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using std::string;

using glm::dvec3;
using glm::vec3;
using glm::dmat4;
using glm::dmat3;
using glm::dquat;

// right now (2024) Intel and AMD cpus have 64 byte cache lines and Apple's M series have 128 bytes.
// Apple can suck a dick because they hate free software (opengl is deprecated on mac).
struct cacheline {
    void *ptr[8];
};

struct mempool {
    std::vector<cacheline, boost::alignment::aligned_allocator<cacheline, 64>> data;
    std::vector<cacheline*> recycler;

    cacheline* alloc();
    void free(cacheline *line);
};

cacheline* mempool::alloc() {
    if(recycler.size() > 0) {
        cacheline* rax = recycler.back();
        recycler.pop_back();
        return(rax);
    } else {
        data.push_back({0});
        return &data.back();
    }
}

void mempool::free(cacheline *line) {
    recycler.push_back(line);
}

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

int16_t d2hi(double in) {
    if(in > 0.0) return (int16_t)(in + 0.5);
    if(in < 0.0) return (int16_t)(in - 0.5);
    return 0;
}

hvec3::hvec3() { x = 0; y = 0; z = 0; }
hvec3::hvec3(int16_t in) { x = in; y = in; z = in; }
hvec3::hvec3(dvec3 in) {
    x = d2hi(in.x);
    y = d2hi(in.y);
    z = d2hi(in.z);
}

hvec3 hvec3::max(hvec3 lhs, hvec3 rhs) {
    return hvec3{
            glm::max(lhs.x, rhs.x),
            glm::max(lhs.y, rhs.y),
            glm::max(lhs.z, rhs.z)};
}

hvec3 hvec3::min(hvec3 lhs, hvec3 rhs) {
    return hvec3{
            glm::min(lhs.x, rhs.x),
            glm::min(lhs.y, rhs.y),
            glm::min(lhs.z, rhs.z)};
}

/*
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
*/
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

struct dTri {
    uint32_t verts[3];
    glm::dvec3 normal;

    dTri();
    dTri(uint32_t pverts[3], dvec3* vertData, dvec3 center);
};

struct dMesh {
    dvec3 *verts;
    dTri *tris;
    uint32_t num_verts;
    uint32_t num_tris;

    void destroy();

    private:
    dMesh(dvec3* pverts, uint32_t pnumVerts, dTri* ptris, uint32_t pnumTris);

    public:
    dMesh();
    static dMesh createBox(dvec3 center, double width, double height, double depth);
};

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
    sleeping, // the forces acting on it are in equilibrium and it's not moving
    immovable, // immovable objects are things like terrain and buildings. They can become active in dire circumstances.
    ghost // can be attached to other objects but does not collide with anything and is not affected by any forces
};


// mesh vertices should be expressed in the main object's coordinate space, which means that they will have to be
// recalculated when a component detaches from the main object
// the benefit is that when collision testing a small object against a large object we don't have to transform the
// vertices of the large object, only transform the small object into the large object's coordinate space.
// limbs attached to joints should be exeptions to this because they need to rotate around the joint
// so for every joint we have to calculate a combined transformation matrix of the parent and the joint

// impacts that cause damage should create a new, recessed vertex at the impact point with depth proportional
// to the damage. For damage that would cause a dent deeper than the component the component can be destroyed.

mempool collision_pool;

struct PhysicsObject {
    PhysicsObject *parent;
    Joint *joint; // joint can only represent the connection to parent. directed acyclic graph is the only valid topology.
    PhysicsObject **limbs; // limbs are subassemblies connected via joints
    PhysicsObject **components; // components are rigidly attached to the object
    cacheline *active_collisions; // I will use this later to implement multi-body collisions

    dMesh mesh;
    double radius; // radius of the bounding sphere for early elimination
    enum physics_state state;
    double mass;
    dmat3 inertia_tensor;
    dvec3 pos; // center of the bounding sphere, not center of mass
    dvec3 vel;
    dquat rot;
    dquat spin;
    double temperature;

    PhysicsObject() { bzero(this, sizeof(PhysicsObject)); }

    PhysicsObject(dMesh pmesh, PhysicsObject *pparent) {
        bzero(this, sizeof(PhysicsObject));
        parent = pparent;
        active_collisions = collision_pool.alloc();
        mesh = pmesh;
        radius = 1.0; // TODO: calculate radius from mesh (easy, just loop over all the vertices)
        state = active;
        mass = 1.0;
        rot = glm::dquat(1.0, 0.0, 0.0, 0.0);
    }

    glm::vec3 zoneSpacePosition() {
        if(!parent){
            return glm::vec3(pos.x, pos.y, pos.z);
        }
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

struct unit_order {
    string command_type;
    dvec3 target_pos;
    uint64_t target_id;
};

uint64_t unit_next_uid = 1;

struct Unit {
    string name;
    uint64_t id;
    uint64_t owner_id;
    PhysicsObject body;
    std::vector<PhysicsObject> limbs;
    std::deque<unit_order> order_queue;
    
    Unit(PhysicsObject pbody) {
        bzero(this, sizeof(Unit));
        name = "prototype";
        id = unit_next_uid++;
        body = pbody;
    }
};

dTri::dTri() {}
dTri::dTri(uint32_t pverts[3], dvec3* vertData, dvec3 center) {
    verts[0] = pverts[0];
    verts[1] = pverts[1];
    verts[2] = pverts[2];
    normal = glm::normalize(glm::cross((vertData[verts[0]] - vertData[verts[1]]), (vertData[verts[0]] - vertData[verts[2]])));
    double dotProduct = glm::dot(normal, vertData[verts[0]] - center);
    if(dotProduct < 0.0){
        normal = -normal;
    }
}

void dMesh::destroy() {
    free(verts);
    verts = 0;
    tris = 0;
    num_verts = 0;
    num_tris = 0;
}

dMesh::dMesh(dvec3* pverts, uint32_t pnumVerts, dTri* ptris, uint32_t pnumTris) {
    verts = pverts;
    num_verts = pnumVerts;
    tris = ptris;
    num_tris = pnumTris;
}

dMesh::dMesh() {}

dMesh dMesh::createBox(dvec3 center, double width, double height, double depth) {
    const int numVertices = 8;
    const int numTriangles = 12;

    dvec3 *vertices = (dvec3*)malloc(numVertices * sizeof(dvec3) + numTriangles * sizeof(dTri));
    dTri *triangles = (dTri*)&vertices[numVertices];

    double halfwidth = width / 2.0;
    double halfheight = height / 2.0;
    double halfdepth = depth / 2.0;

    int vertexIndex = 0;
    for (int i = 0; i < 8; ++i) {
        double xCoord = i & 1 ? halfwidth : -halfwidth;
        double yCoord = i & 4 ? halfheight : -halfheight;
        double zCoord = i & 2 ? halfdepth : -halfdepth;
        vertices[vertexIndex++] = center + dvec3(xCoord, yCoord, zCoord);
    }

    uint32_t faces[6 * 4] =
        {0, 1, 2, 3, // top
        0, 1, 4, 5, // front
        0, 2, 4, 6, // left
        7, 5, 3, 1, // right
        7, 6, 3, 2, // back
        7, 6, 5, 4}; // bottom

    for(int i = 0; i < 6; i++) {
        uint32_t tri1[3] = {faces[i * 4], faces[i * 4 + 1], faces[i * 4 + 2]};
        uint32_t tri2[3] = {faces[i * 4 + 1], faces[i * 4 + 2], faces[i * 4 + 3]};
        triangles[i * 2] = dTri(tri1, vertices, center);
        triangles[i * 2 + 1] = dTri(tri2, vertices, center);
    }

    return dMesh(vertices, numVertices, triangles, numTriangles);
}


// Thoughts on convex/concave hull
//
// We should separately collision test anything connected to a joint, and also anything protruding from the main hull.
// The final authority on whether a component collides with something should be its hull mesh. Each component should have a very
// simple hull mesh and the main body can have a hull mesh that combines the outer surface into one mesh except protruding 
// and jointed features since they will be tested separately.

// TODO: calculate the bounding box from the collision shape.
// 1. physics objects move, rotate, appear and disappear
// 2. we calculate a transformation matrix from the object's new rotation
// 3. we transform each point in the collision shape's mesh to calculate a new bounding box (but leave the mesh untransformed)
// 3b. we may consider caching a transformed copy of the mesh for faster collision detection
// 4. we add the physics object's position to the bounding box's
// 5. we update the BVH
// 6. we can now read from the BVH and write to the physics objects to our heart's content for the rest of the tick

struct ctleaf {
    PhysicsObject *object;

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
    void subdivide(ctnode *nodes, uint32_t *poolPtr, ctleaf *primitives, uint32_t first, uint32_t last);
};


#define MAX_MAX_DEPTH 16
struct CollisionTree {
    dvec3 pos;
    ctnode *root = 0;

    CollisionTree(dvec3 origo);
};

struct Zone {
    Zone *neighbors[6]; // +x, -x, +y, -y, +z, -z

    CollisionTree tree;
    Celestial *frame_of_reference;
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

void ctnode::setBounds(AABB bounds) { hi = bounds.max; lo = bounds.min; }

void ctnode::subdivide(ctnode* nodes, uint32_t* poolPtr, ctleaf* primitives, uint32_t first, uint32_t last) {
    if (first == last) {
        this->first_leaf = first;
        this->count = 1;
        return;
    }
    this->left_child = *poolPtr;
    *poolPtr += 2;
    ctnode *left = &nodes[left_child];
    ctnode *right = &nodes[left_child + 1];

    // sort the primitives
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
    // split them in a dumb way that is not optimal but better than worst case
    // here we could instead do a binary search to find the geometric middle or even search for the biggest gap
    uint32_t split = (first + last) >> 1;

    left->setBounds(calculateBounds(primitives, first, split));
    right->setBounds(calculateBounds(primitives, split + 1, last));
    left->subdivide(nodes, poolPtr, primitives, first, split);
    right->subdivide(nodes, poolPtr, primitives, split + 1, last);
    this->count = last - first + 1;
}


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
CollisionTree::CollisionTree(dvec3 origo) {
    pos = origo;
    root = 0;
}


// TODO:
// we should put all the zones into a zone BVH for each celestial so it's easy to find the correct zone for any location near a celestial
// for zones not near a celestial we need a solution with a bigger coordinate system than 16 bits


/*

"collision shape" should probably just be simplified to an AABB for each object from the tree's
perspective. Then when testing object collisions we can test the polygon mesh if the AABBs intersect.

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


Long-term data storage:
Consider PostGIS


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


