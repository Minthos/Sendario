#include "terragen.h"

#include <algorithm>
#include <boost/align/aligned_allocator.hpp>
#include <chrono>
#include <deque>
#include <iostream>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

using std::string;
auto now = std::chrono::high_resolution_clock::now;
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
//    assert(0); // using std::vectors this way causes segfaults, use malloc instead
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
    dvec3 todvec3() { return dvec3(x, y, z); }
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

    dTri() {}

    dTri(uint32_t pverts[3], dvec3* vertData, dvec3 center) {
        verts[0] = pverts[0];
        verts[1] = pverts[1];
        verts[2] = pverts[2];
        normal = glm::normalize(glm::cross((vertData[verts[0]] - vertData[verts[1]]), (vertData[verts[0]] - vertData[verts[2]])));
        double dotProduct = glm::dot(normal, vertData[verts[0]] - center);
        if(dotProduct < 0.0){
            normal = -normal;
        }
    }
};

struct dMesh {
    dvec3 *verts;
    dTri *tris;
    uint32_t num_verts;
    uint32_t num_tris;

    dMesh() {}

    dMesh(dvec3* pverts, uint32_t pnumVerts, dTri* ptris, uint32_t pnumTris) {
        verts = pverts;
        num_verts = pnumVerts;
        tris = ptris;
        num_tris = pnumTris;
    }

    void destroy() {
        if(verts)
            free(verts);
        if(tris)
            free(tris);
        verts = 0;
        tris = 0;
        num_verts = 0;
        num_tris = 0;
    }

    // TODO: create a cylinder mesh and remember to make a function that produces the correct texture coordinates for
    // the mesh upload function

    static dMesh createBox(dvec3 center, double width, double height, double depth) {
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
    
    // not in use
    void createTerrain(glm::dvec3 center, uint64_t seed) {
    }

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

struct RenderObject;

mempool collision_pool;

struct PhysicsObject {
    PhysicsObject *parent;
    RenderObject *ro;
    Joint *joint; // joint can only represent the connection to parent. directed acyclic graph is the only valid topology.
    PhysicsObject *limbs; // limbs are subassemblies connected via joints
    int num_limbs;
    PhysicsObject *components; // components are rigidly attached to the object
    int num_components;
    cacheline *active_collisions; // I will use this later to implement multi-body collisions

    dMesh mesh;
    double radius; // radius of the bounding sphere for early elimination
    enum physics_state state;
    double mass;
    dmat3 inertia_tensor;
    dvec3 pos; // center of the coordinate system (and therefore the bounding sphere), not center of mass
    dvec3 vel;
    dquat rot;
    dquat spin;
    double temperature;

    PhysicsObject() {
        bzero(this, sizeof(PhysicsObject));
        active_collisions = collision_pool.alloc();
        rot = glm::dquat(1.0, 0.0, 0.0, 0.0);
        spin = glm::dquat(1.0, 0.0, 0.0, 0.0);
        state = active;
    }

    PhysicsObject(dMesh pmesh, PhysicsObject *pparent) {
        bzero(this, sizeof(PhysicsObject));
        parent = pparent;
        active_collisions = collision_pool.alloc();
        mesh = pmesh;
        radius = calculateRadius();
        state = active;
        mass = 1.0;
        rot = glm::dquat(1.0, 0.0, 0.0, 0.0);
    }

    // this just assumes that pos is at the center, of course in practice it will often not be
    double calculateRadius() {
        double greatestRsquared = 0.0;
        for(int i = 0; i < mesh.num_verts; i++){
            dvec3 lvalue = mesh.verts[i] - pos;
            double rSquared = glm::length2(lvalue);
            greatestRsquared = glm::max(greatestRsquared, rSquared);
        }
        return sqrt(greatestRsquared);
    }

    glm::vec3 zoneSpacePosition() {
        dvec3 sum_pos = pos;
        PhysicsObject *next = parent;
        while(next){
            sum_pos += parent->pos;
            next = parent->parent;
        }
        return glm::vec3(sum_pos.x, sum_pos.y, sum_pos.z);
    }
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
    std::vector<PhysicsObject> components;
    bool limbs_dirty;
    bool components_dirty;

    std::deque<unit_order> order_queue;
    
    Unit() {
        bzero(this, sizeof(Unit));
        name = "prototype";
        id = unit_next_uid++;
        body = PhysicsObject();
    }

    void addLimb(dMesh pmesh) {
        limbs.push_back(PhysicsObject(pmesh, &body));
        limbs_dirty = true;
    }

    void addComponent(dMesh pmesh) {
        components.push_back(PhysicsObject(pmesh, &body));
        components_dirty = true;
    }

    void bake() {

        if(limbs_dirty){
            body.limbs = &limbs[0];
            body.num_limbs = limbs.size();
            limbs_dirty = false;
        }
        if(components_dirty){
            body.mesh.destroy();
            
            uint64_t num_tris = 0;
            uint64_t num_verts = 0;
            for(int i = 0; i < components.size(); i++){
                num_tris += components[i].mesh.num_tris;
                num_verts += components[i].mesh.num_verts;
            }
            dvec3 *verts = (dvec3 *)malloc(num_verts * sizeof(dvec3) + num_tris * sizeof(dTri));

            uint64_t tri_idx = 0;
            uint64_t vert_idx = 0;

            for(int i = 0; i < components.size(); i++){
                memcpy(&verts[vert_idx], components[i].mesh.verts, components[i].mesh.num_verts * sizeof(dvec3));
                vert_idx += components[i].mesh.num_verts;
            }

            dTri *tris = (dTri*)&verts[vert_idx];
            for(int i = 0; i < components.size(); i++){
                memcpy(&tris[tri_idx], components[i].mesh.tris, components[i].mesh.num_tris * sizeof(dTri));
                tri_idx += components[i].mesh.num_tris;
            }
            
            body.mesh = dMesh(verts, num_verts, tris, num_tris);

            body.components = &components[0];
            body.num_components = components.size();
            body.radius = body.calculateRadius();
            components_dirty = false;
        }
    }
};

// Thoughts on convex/concave hull
//
// We should separately collision test anything connected to a joint, and also anything protruding from the main hull.
// The final authority on whether a component collides with something should be its hull mesh. Each component should
// have a very simple hull mesh and the main body can have a hull mesh that combines the outer surface into one mesh
// except protruding and jointed features since they will be tested separately.

// TODO: calculate the bounding box from the collision shape.
// 1. physics objects move, rotate, appear and disappear
// 2. we calculate a transformation matrix from the object's new rotation
// 3. we transform each point in the collision shape's mesh to calculate a new bounding box (but leave the mesh
// untransformed)
// 3b. we may consider caching a transformed copy of the mesh for faster collision detection
// 4. we add the physics object's position to the bounding box's
// 5. we update the BVH
// 6. we can now read from the BVH and write to the physics objects to our heart's content for the rest of the tick

struct ctleaf {
    PhysicsObject *object;

    // axis-aligned bounding box
    hvec3 hi; // 6 bytes (total 14)
    hvec3 lo; // 6 bytes (total 20)

    ctleaf(PhysicsObject *o){
        object = o;
        hi = hvec3(o->pos + o->radius + 0.5);
        lo = hvec3(o->pos - o->radius - 0.5);
    }
};

struct AABB {
	hvec3 max;
	hvec3 min;
};

AABB calculateBounds(ctleaf* p, uint32_t first, uint32_t last) {
    AABB bounds;
    bounds.min = hvec3(32767);
    bounds.max = hvec3(-32768);
    for (uint32_t i = first; i < last; ++i) {
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
};



#define MAX_MAX_DEPTH 16
struct CollisionTree {
    dvec3 pos;
    ctnode *root = 0;
    ctleaf *leaves = 0;

    void destroy() {
        if(root){
            free(root);
            root = 0;
        }
        if(leaves){
//            free(leaves);
            leaves = 0;
        }
    }

    CollisionTree(dvec3 origo) {
        pos = origo;
        root = 0;
    }

    CollisionTree(dvec3 origo, ctleaf* pleaves, int N) {
        pos = origo;
        leaves = pleaves;
        ctnode* nodes = (ctnode*) malloc(sizeof(ctnode) * (2 * N));
        root = &nodes[0];
        root->count = N;
        AABB globalBounds = calculateBounds(pleaves, 0, N);
        root->setBounds(globalBounds);
        uint32_t poolPtr = 1;
        root->subdivide(nodes, &poolPtr, leaves, 0, N - 1);
    //	glBindBuffer(GL_SHADER_STORAGE_BUFFER, TLASBuffer);
    //	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ctnode) * poolPtr, nodes, GL_STATIC_DRAW);
    }

};

struct ttnode {
    uint32_t first_child;
    double elevation[3]; // elevation of the node's 3 corners
    dvec3 verts[3]; // vertices on the unit sphere

    // add more stuff like temperature, moisture, vegetation
};

struct TerrainTree {
    uint64_t seed;
    double radius;
    TerrainGenerator *generator;

    std::vector<ttnode> nodes;

    TerrainTree() {
        bzero(this, sizeof(TerrainTree));
    }

    TerrainTree(uint64_t pseed, double pradius) {
        seed = pseed;
        radius = pradius;
        generator = new TerrainGenerator(seed);
        // 6 corners
        dvec3 initial_corners[6] = {
            dvec3(0.0, radius, 0.0),
            dvec3(0.0, -radius, 0.0),
            dvec3(0.0, 0.0, radius),
            dvec3(radius, 0.0, 0.0),
            dvec3(0.0, 0.0, -radius),
            dvec3(-radius, 0.0, 0.0)
        };
        int indices[8][3] = {
            {0, 2, 3},
            {0, 3, 4},
            {0, 4, 5},
            {0, 5, 2},
            {1, 2, 5},
            {1, 5, 4},
            {1, 4, 3},
            {1, 3, 2},
        };
        for(int i = 0; i < 8; i++){
            nodes.push_back({ 0,
                    generator->getElevation(initial_corners[indices[i][0]]),
                    generator->getElevation(initial_corners[indices[i][1]]),
                    generator->getElevation(initial_corners[indices[i][2]]),
                    initial_corners[indices[i][0]],
                    initial_corners[indices[i][1]],
                    initial_corners[indices[i][2]]} );
        }
    }

    // to keep things simple I will define a zone as a triangle at some reasonable subdivision level
    // (~1 km on earth's surface?)
    // and a zone's id will be encoded in a 64 bit unsigned integer
    // the least significant bit separates north/south
    // the next two bits encode which of the 4 subdivisions to traverse
    // 0 - 3: lat lon (0 - 90, 0 - 90), (0 - 90, 90 - 180), (0 - 90, 180 - 270), (0 - 90, 270 - 0)
    // 4 - 7: lat lon (-90 - 0, -90 - 0), (-90 - 0, 90 - 180), (-90 - 0, 180 - 270), (-90 - 0, 270 - 0)
    // for every 2 additional bits we encode one more level of subdivision in clockwise order seen from above the
    // nearest pole (so the northern hemisphere will be traversed west-east and the southern east-west)
    //
    // initially we can just ignore location and set max_subdivisions to something low like 2 or 3

    void traverse(dvec3 location, uint32_t node_idx, std::vector<glm::dvec3> *verts, std::vector<dTri> *tris, int level, int max_level) {
        if(level > max_level) {
            double distance2 = glm::length2(location - nodes[node_idx].verts[0]);
            double nodeWidth2 = glm::length2(nodes[node_idx].verts[0] - nodes[node_idx].verts[1]);
            double ratio = distance2 / nodeWidth2;
            if(ratio > 1000.0) {
                dTri t;
                dvec3 center = {0, 0, 0};
                for(int i = 0; i < 3; i++) {
                    t.verts[i] = verts->size();
                    // scaling each point to the surface of the spheroid and adding the elevation value
                    verts->push_back(nodes[node_idx].verts[i] *
                            ((radius * 0.99 + 0.01 * radius * nodes[node_idx].elevation[i]) / glm::length(nodes[node_idx].verts[i])) );
                    center += nodes[node_idx].verts[i];
                }
                t.normal = center / glm::length(center);
                tris->push_back(t);
                return;
            }
        }
        if( ! nodes[node_idx].first_child) {
            nodes[node_idx].first_child = (uint32_t)nodes.size();
            dvec3 new_verts[3] = {
                (nodes[node_idx].verts[0] + nodes[node_idx].verts[1]) * 0.5,
                (nodes[node_idx].verts[1] + nodes[node_idx].verts[2]) * 0.5,
                (nodes[node_idx].verts[2] + nodes[node_idx].verts[0]) * 0.5};
            nodes.push_back({ 0,
                generator->getElevation(new_verts[0]),
                generator->getElevation(new_verts[1]),
                generator->getElevation(new_verts[2]),
                new_verts[0],
                new_verts[1],
                new_verts[2] });
            for(int i = 0; i < 3; i++) {
                nodes.push_back({ 0,
                    generator->getElevation(nodes[node_idx].verts[i]),
                    generator->getElevation(new_verts[i]),
                    generator->getElevation(new_verts[(i + 2) % 3]),
                    nodes[node_idx].verts[i],
                    new_verts[i],
                    new_verts[(i + 2) % 3] });
            }
        }
        for(int i = 0; i < 4; i++) {
            traverse(location, nodes[node_idx].first_child + i, verts, tris, level + 1, max_level);
        }
    }

    // TODO high priority
    // use distance from node to location to determine max subdivision level for that node
    // for this calculation double precision is precise enough even at planetary scale
    dMesh buildMesh(dvec3 location, int max_subdivisions) {
        std::vector<uint32_t> node_indices;
        std::vector<glm::dvec3> verts;
        std::vector<dTri> tris;
        for(int i = 0; i < 8; i++) {
            traverse(location / radius, i, &verts, &tris, 1, max_subdivisions);
        }
        dvec3 *vertices = (dvec3*)malloc(verts.size() * sizeof(dvec3) + tris.size() * sizeof(dTri));
        dTri *triangles = (dTri*)&vertices[verts.size()];

        memcpy(vertices, &verts[0], verts.size() * sizeof(dvec3));
        memcpy(triangles, &tris[0], tris.size() * sizeof(dTri));

        return dMesh(vertices, verts.size(), triangles, tris.size());
    }
};

struct Celestial {
    uint64_t seed;
    std::string name;
    PhysicsObject body;
    TerrainTree terrain;
    double surface_temp_min;
    double surface_temp_max;
    vec9 radiance;
    vec9 albedo;
    Celestial *nearest_star;
    std::vector<Celestial> orbiting_bodies;

    Celestial(uint64_t pseed, std::string pname, double pradius, Celestial *pnearest_star) {
        bzero(this, sizeof(Celestial));
        seed = pseed;
        name = pname;
        terrain = TerrainTree(pseed, pradius);
        auto time_begin = now();
        std::cout << "Generating mesh..\n";
        body = PhysicsObject(terrain.buildMesh(dvec3(0, 6.4e6, 0), 5), NULL);
        auto time_used = std::chrono::duration_cast<std::chrono::microseconds>(now() - time_begin).count();
        std::cout << "Celestial " << name << ": " << body.mesh.num_tris << " triangles procedurally generated in " << time_used/1000.0 << "ms\n";
        surface_temp_min = 183.0;
        surface_temp_max = 331.0;
        nearest_star = pnearest_star;
    }
};

struct Zone {
    Zone *neighbors[6]; // +x, -x, +y, -y, +z, -z

    CollisionTree tree;
    Celestial *frame_of_reference;
};


// TODO:
// we should put all the zones into a zone BVH for each celestial so it's easy to find the correct zone for any location near a celestial
// for zones not near a celestial we need a solution with a bigger coordinate system than 16 bits


/*

Physics solving

To intersect two meshes we can do point-triangle intersection and edge-edge intersection.


Interesting article about solvers
https://box2d.org/posts/2024/02/solver2d/

https://matthias-research.github.io/pages/tenMinutePhysics/index.html
https://www.youtube.com/watch?v=zzy6u1z_l9A recommends to solve locally instead of globally to simplify and to do
multiple iterations

----

moment of intertia: the main body has a precomputed intertia tensor without the limbs and the masses of the limbs
must be added each tick in order to produce the true intertia tensor taking each limb's center of mass into account

----

"collision shape" should probably just be simplified to an AABB for each object from the tree's
perspective. Then when testing object collisions we can test the polygon mesh if the AABBs intersect.

constructing the AABB from a sphere makes it bigger than it has to be but it saves us from having to recompute it with
transformed vertices every frame due to rotation, we can just take the position and add the radius in each dimension.

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

