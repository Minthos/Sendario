#include "terragen.h"

#include <algorithm>
#include <boost/align/aligned_allocator.hpp>
#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>
#include <vector>

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#define nil nullptr



using std::isnan;
using std::mutex;
using std::string;
auto now = std::chrono::high_resolution_clock::now;
using glm::dvec3;
using glm::vec3;
using glm::dmat4;
using glm::dmat3;
using glm::dquat;

bool verbose = false;
bool POTATO_MODE = false;
uint32_t frame_counter = 0;
float PI = 3.14159265359f;

enum terrain_upload_status_enum {
    idle,
    should_exit,
    generating,
    done_generating,
    done_generating_first_time,
    uploading,
    done_uploading
};

enum vertex_type_id {
    VERTEX_TYPE_NONE,
    VERTEX_TYPE_TERRAIN,
    VERTEX_TYPE_TREETRUNK,
    VERTEX_TYPE_LEAF
};

size_t min(size_t a, size_t b) {
    return a > b ? b : a;
}

size_t max(size_t a, size_t b) {
    return a < b ? b : a;
}


uint32_t min(uint32_t a, uint32_t b) {
    return a > b ? b : a;
}

uint32_t max(uint32_t a, uint32_t b) {
    return a < b ? b : a;
}


double min(double a, double b) {
    return a > b ? b : a;
}

double max(double a, double b) {
    return a < b ? b : a;
}


namespace nonstd {

template <typename T> struct vector {
    T* data;
    size_t count;
    size_t capacity;

    vector() { bzero(this, sizeof(vector<T>)); }

    void destroy() {
        for(size_t i = 0; i < count; ++i){
            data[i].~T();
        }
        free(data);
        bzero(this, sizeof(vector<T>));
    }

    vector<T> copy() {
        vector<T> tmp;
        tmp.data = (T*)malloc(sizeof(T) * count);
        memcpy(tmp.data, data, sizeof(T) * count);
        tmp.count = count;
        tmp.capacity = count;
        return tmp;
    }

    size_t size() { return count; }

    void reserve(size_t new_capacity) {
        assert(new_capacity >= count);
        data = (T*)realloc(data, sizeof(T) * new_capacity);
        capacity = new_capacity;
    }

    void push_back(const T& val) {
        if(count == capacity) reserve(max(4, capacity) * 2);
        memcpy(&data[count++], &val, sizeof(T));
    }

    void push_back(T&& val) {
        if(count == capacity) reserve(max(4, capacity) * 2);
        memcpy(&data[count++], &val, sizeof(T));
    }

    template <typename... Args> constexpr T& emplace_back(Args&&... args) {
        if(count == capacity) reserve(max(4, capacity) * 2);
        return *new(&data[count++]) T(std::forward<Args>(args)...);
    }

    T pop_back() { return data[--count]; }
    T& operator[](size_t idx) { return data[idx]; }
    T* begin() { return data; }
    T* end() { return &data[count]; }
    T& back() { return data[count-1]; }
};

};

std::string fstr(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vector<char> buf(1 + std::vsnprintf(nil, 0, format, args));
    va_end(args);
    va_start(args, format);
    std::vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);
    return std::string(buf.data());
}

std::string str(dvec3 in) {
    return fstr("(%f, %f, %f)", in.x, in.y, in.z);
}


// right now (2024) Intel and AMD cpus have 64 byte cache lines and Apple's M series have 128 bytes.
// Apple can suck a dick because opengl is deprecated on macos and their ios app store policies are anticompetitive.
struct cacheline {
    void *ptr[8];
};

struct mempool {
    nonstd::vector<cacheline*> data;
    nonstd::vector<cacheline*> recycler;
    uint64_t data_idx;

    ~mempool() { data.destroy(); recycler.destroy(); }
    cacheline* alloc();
    void free(cacheline *line);
};

cacheline* mempool::alloc() {
    if(recycler.size() > 0) {
        return recycler.pop_back();
    } else {
        if( ! data_idx){
            data.push_back((cacheline *)calloc(4096, sizeof(cacheline)));
            uint64_t misalignment = (uint64_t)data.back() % sizeof(cacheline);
            if(misalignment != 0){
                if(verbose) std::cout << "pool allocator adjusted misaligned memory: off by " << misalignment <<" bytes\n";
                data[data.size()-1] -= misalignment;
                ++data_idx;
            } else {
                if(verbose) std::cout << "pool allocator got memory that was already aligned, nothing to adjust\n";
            }
        }
        cacheline* rax = &data.back()[data_idx];
        data_idx = (data_idx + 1) % 4096;
        return rax;
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

double length(dvec3 a) {
    dvec3 s = a * a;
    return sqrt(s.x + s.y + s.z);
}

dvec3 normalize(dvec3 a) {
    return glm::normalize(a);
/*    double length = ::length(a);
    assert(length != 0);
    if(length == 0){
        return a * length;
    }
    return a / length;*/
}

struct dTri {
    uint32_t verts[3];
    float elevations[3];
    glm::dvec3 normal;
    int32_t type_id;

    dTri() {}

    dTri(uint32_t pverts[3], dvec3* vertData, dvec3 center, int32_t ptype_id) {
        verts[0] = pverts[0];
        verts[1] = pverts[1];
        verts[2] = pverts[2];
        normal = normalize(glm::cross((vertData[verts[0]] - vertData[verts[1]]), (vertData[verts[0]] - vertData[verts[2]])));
        double dotProduct = glm::dot(normal, vertData[verts[0]] - center);
        if(dotProduct < 0.0){
            normal = -normal;
        }
        type_id = ptype_id;
    }
};

struct dMesh {
    dvec3 *verts;
    dTri *tris;
    uint32_t num_verts;
    uint32_t num_tris;

    dMesh() { bzero(this, sizeof(dMesh)); }

    dMesh(dvec3* pverts, uint32_t pnumVerts, dTri* ptris, uint32_t pnumTris) {
        verts = pverts;
        num_verts = pnumVerts;
        tris = ptris;
        num_tris = pnumTris;
    }

    void destroy() {
        if(verts)
            free(verts);
        verts = 0;
        tris = 0;
        num_verts = 0;
        num_tris = 0;
    }

    // TODO: create a cylinder mesh and remember to make a function that produces the correct texture coordinates for
    // the mesh upload function

    static dMesh createBox(dvec3 center, double width, double height, double depth) {
        assert(width > 0);
        assert(height > 0);
        assert(depth > 0);
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
            triangles[i * 2] = dTri(tri1, vertices, center, VERTEX_TYPE_NONE);
            triangles[i * 2 + 1] = dTri(tri2, vertices, center, VERTEX_TYPE_NONE);
        }
        return dMesh(vertices, numVertices, triangles, numTriangles);
    }
    
};

struct texvert {
    glm::vec4 xyz;
//    int32_t type_id; (opengl clobbered this when I passed it as int so I packed it into the w component of xyz as a workaround)
    glm::vec3 uvw;
    texvert(glm::vec3 a, int32_t ptype_id, glm::vec3 b) { xyz = glm::vec4(a.x, a.y, a.z, *(float*)(&ptype_id)); uvw = b; }
};

void mkquad(nonstd::vector<texvert> *dest, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 uvw, int32_t type_id){
    dest->push_back({a, type_id, uvw});
    dest->push_back({b, type_id, uvw});
    dest->push_back({c, type_id, uvw});
    dest->push_back({c, type_id, uvw});
    dest->push_back({b, type_id, uvw});
    dest->push_back({d, type_id, uvw});
}

void mktree(nonstd::vector<texvert> *dest, float h_trunk, float r_trunk, float h_canopy, float r_canopy, glm::vec3 origin){
    float h_root = -0.5;
    // trunk, should be at least 4 quads
    mkquad(dest,
            glm::vec3(-r_trunk, h_root, -r_trunk) + origin,
            glm::vec3(r_trunk, h_root, -r_trunk) + origin,
            glm::vec3(-r_trunk, h_trunk, -r_trunk) + origin,
            glm::vec3(r_trunk, h_trunk, -r_trunk) + origin,
            glm::vec3(0.0f), VERTEX_TYPE_TREETRUNK);
    mkquad(dest,
            glm::vec3(r_trunk, h_root, r_trunk) + origin,
            glm::vec3(-r_trunk, h_root, r_trunk) + origin,
            glm::vec3(r_trunk, h_trunk, r_trunk) + origin,
            glm::vec3(-r_trunk, h_trunk, r_trunk) + origin,
            glm::vec3(0.333333f), VERTEX_TYPE_TREETRUNK);
    mkquad(dest,
            glm::vec3(r_trunk, h_root, -r_trunk) + origin,
            glm::vec3(r_trunk, h_root, r_trunk) + origin,
            glm::vec3(r_trunk, h_trunk, -r_trunk) + origin,
            glm::vec3(r_trunk, h_trunk, r_trunk) + origin,
            glm::vec3(0.666667f), VERTEX_TYPE_TREETRUNK);
    mkquad(dest,
            glm::vec3(-r_trunk, h_root, -r_trunk) + origin,
            glm::vec3(-r_trunk, h_root, r_trunk) + origin,
            glm::vec3(-r_trunk, h_trunk, -r_trunk) + origin,
            glm::vec3(-r_trunk, h_trunk, r_trunk) + origin,
            glm::vec3(1.0f), VERTEX_TYPE_TREETRUNK);

    // canopy, should be at least 4 triangles
    mkquad(dest,
            glm::vec3(-r_canopy, h_trunk, -r_canopy) + origin,
            glm::vec3(-r_canopy, h_trunk, r_canopy) + origin,
            glm::vec3(0.0, h_trunk + h_canopy, 0.0) + origin,
            glm::vec3(r_canopy, h_trunk, r_canopy) + origin,
            glm::vec3(0.0f), VERTEX_TYPE_LEAF);
    mkquad(dest,
            glm::vec3(r_canopy, h_trunk, r_canopy) + origin,
            glm::vec3(r_canopy, h_trunk, -r_canopy) + origin,
            glm::vec3(0.0, h_trunk + h_canopy, 0.0) + origin,
            glm::vec3(-r_canopy, h_trunk, -r_canopy) + origin,
            glm::vec3(1.0f), VERTEX_TYPE_LEAF);
}

struct Motor {
    double max_force;
    double min_force; // negative values for motors that can produce power in both directions
    double max_braking_force;
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
    uint64_t zone;
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

    ~PhysicsObject() {
        if(active_collisions) {
            collision_pool.free(active_collisions);
            active_collisions = 0;
        }
        mesh.destroy();
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
    nonstd::vector<PhysicsObject> limbs;
    nonstd::vector<PhysicsObject> components;
    bool limbs_dirty;
    bool components_dirty;

    std::deque<unit_order> order_queue;
    
    Unit() {
        bzero(this, sizeof(Unit));
        name = "prototype";
        id = unit_next_uid++;
        owner_id = 0;
        new(&body) PhysicsObject();
        limbs_dirty = false;
        components_dirty = false;
    }

    void addLimb(dMesh pmesh) {
        limbs.emplace_back(pmesh, &body);
        limbs_dirty = true;
    }

    void addComponent(dMesh pmesh) {
        components.emplace_back(pmesh, &body);
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
        leaves = 0;
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
    uint64_t path;
    uint32_t first_child;
    uint32_t triangle;
    uint32_t neighbors[3];
    uint32_t rendered_at_level; // number of subdivisions to reach this node, if it was rendered. otherwise 0.
    double elevations[3]; // elevation of the node's 3 corners above the planet's spheroid
    double roughnesses[3]; // terrain roughness at the node's 3 corners
    dvec3 verts[3]; // vertices (node space (octahedron with manhattan distance to center = r everywhere on the surface))
    uint32_t last_used_at_frame;
    nonstd::vector<texvert> vegetation;


    // add more stuff like temperature, moisture, vegetation
    std::string str(){
        return fstr("node: 0x%llx elevation: %f verts: %s, %s, %s", path, elevation(), ::str(verts[0]).c_str(), ::str(verts[1]).c_str(), ::str(verts[2]).c_str());
    }
   
    dvec3 spheroidPosition(dvec3 vert, double radius) {
        double length = glm::length(vert);
        return vert * (radius / length);
    }

    dvec3 globalPosition(double radius, int i) {     
        double length = glm::length(verts[i]);
        return verts[i] * ((radius + elevations[i]) / length);
    }

    // position of 0th vertex transformed to zone space (LOD space)
    vec3 zone_space_position(dvec3 location, double radius, int i) {
        glm::vec3 point = verts[i];
        double length = glm::length(point);
        point = point * (radius / length); // scaled from node space to spheroid
        point = point + (elevations[i] * (point / length)) - location; // subtracting location.. adding -gravnorm * elevations
        return point; // sounds about right, yeah?
    }

    // node space center
    dvec3 center() {
        dvec3 pos(0.0);
        for(int i = 0; i < 3; i++) {
            pos += verts[i];
        }
        pos /= 3.0;
        return pos;
    }

    double elevation() {
        double elevation = 0.0;
        for(int i = 0; i < 3; i++) {
            elevation += elevations[i];
        }
        elevation /= 3.0;
        return elevation;
    }

    double elevation_kludgehammer(dvec3 pos, dMesh* mesh, dvec3 gravity_dir) {
        double p = elevation_projected(pos, mesh, gravity_dir);
        double n = elevation_naive(pos, mesh);
        if(isnan(p) && isnan(n)) assert(0);
        if(isnan(p)) return n;
        if(isnan(n)) return p;
        if(n < 0 && abs(p + n) < 100.0){
            return -p;
        } else if(abs(p - n) > 100.0){
            return n;
        } else {
            return p;
        }
    }

    double player_altitude(dvec3 pos, dMesh* mesh, dvec3 gravity_dir) {
        dTri *tri = &mesh->tris[triangle];
        dvec3 a = mesh->verts[tri->verts[0]];
        dvec3 b = mesh->verts[tri->verts[1]];
        dvec3 c = mesh->verts[tri->verts[2]];
        dvec3 ab = b - a;
        dvec3 ac = c - a;
        dvec3 norm = normalize(glm::cross(ab, ac));
        double divisor = dot(norm, gravity_dir);
        for(int i = 0; i < 9; i++){
            assert(!isnan(((double*)(&mesh->verts[tri->verts[i/3]]))[i % 3])); // fight me
        }
        assert(divisor != 0.0);
        double d = dot(norm, (a - pos)) / divisor;
        return d;
    }

    double elevation_projected(dvec3 pos, dMesh* mesh, dvec3 gravity_dir) {
        dTri *tri = &mesh->tris[triangle];
        dvec3 a = mesh->verts[tri->verts[0]];
        dvec3 b = mesh->verts[tri->verts[1]];
        dvec3 c = mesh->verts[tri->verts[2]];
        dvec3 ab = b - a;
        dvec3 ac = c - a;
        dvec3 norm = normalize(glm::cross(ab, ac));
        double d = dot(norm, (a - pos)) / dot(norm, gravity_dir);
        dvec3 intersection = pos + (gravity_dir * d);
        return glm::length(intersection);
    }
   
    // this one is wrong in both directions
    double elevation_naive(dvec3 pos, dMesh* mesh) {
        dTri *tri = &mesh->tris[triangle];
        double e = 0;
        double sum_distance = 0;
        for(uint64_t i = 0; i < 3; i++){
            double distance = glm::length(pos - tri->verts[i]);
            sum_distance += distance;
        }
        for(uint64_t i = 0; i < 3; i++){
            double distance = glm::length(pos - tri->verts[i]);
            e += (sum_distance * 0.5 - distance) * elevations[i];
        }
        return e / (sum_distance * 0.5);
    }
};

glm::vec3 calculate_center(uint64_t level, uint64_t address, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
	assert(level < 32);
    for(int i = 0; i < level; i++){
        glm::vec3 mid0 = (v0 + v1) * 0.5f;
        glm::vec3 mid1 = (v1 + v2) * 0.5f;
        glm::vec3 mid2 = (v2 + v0) * 0.5f;
        switch ((address >> (2 * i)) & 3) {
            case 0: // Center triangle
                v0 = mid0;
                v1 = mid1;
                v2 = mid2;
                break;
            case 1: // First corner triangle
                v1 = mid0;
                v2 = mid2;
                break;
            case 2: // Second corner triangle
                v2 = mid1;
                v0 = mid0;
                break;
            case 3: // Third corner triangle
                v0 = mid2;
                v1 = mid1;
                break;
        }
    }
    glm::vec3 center = (v0 + v1 + v2) / 3.0f;
    return center;
}

struct TerrainTree {
    uint64_t seed;
    double radius;
    double noise_yscaling;
    double LOD_DISTANCE_SCALE;
    int MAX_LOD;
    float highest_point = 0.0f;
    float lowest_point = 0.0f;

    TerrainGenerator *generator;
    nonstd::vector<ttnode> nodes;

    void destroy() {
        for(int i = 0; i > nodes.count; i++){
            nodes[i].vegetation.destroy();
        }
        nodes.destroy();
    }

    ~TerrainTree() {
    }

    TerrainTree() {
        bzero(this, sizeof(TerrainTree));
    }

    TerrainTree copy() {
        TerrainTree tmp;
        memcpy(&tmp, this, sizeof(TerrainTree));
        tmp.nodes = nodes.copy();
        return tmp;
    }

    void descend_and_omit(TerrainTree *tmp, ttnode* node, uint32_t cutoff) {
        if(node->last_used_at_frame < cutoff){
            node->first_child = 0;
            return;
        }
        if( ! node->first_child){
            return;
        }
        uint32_t first_child = node->first_child;
        node->first_child = tmp->nodes.count;
        for(uint32_t i = 0; i < 4; ++i){
            tmp->nodes.push_back(nodes[first_child + i]);
        }
        for(uint32_t i = 0; i < 4; ++i){
            descend_and_omit(tmp, &tmp->nodes[node->first_child + i], cutoff);
        }
    }

    // omit nodes that haven't been used since the cutoff
    TerrainTree omitting_copy(uint32_t cutoff) {
        TerrainTree tmp;
        memcpy(&tmp, this, sizeof(TerrainTree));
        bzero(&tmp.nodes, sizeof(tmp.nodes));
        tmp.nodes.reserve(nodes.count);
        for(int i = 0; i < 8; i++) {
            tmp.nodes.push_back(nodes[i]);
        }
        for(int i = 0; i < 8; i++) {
            descend_and_omit(&tmp, &tmp.nodes[i], cutoff);
        }
        return tmp;
    }

    // MAX_LOD should be no more than 18 for an Earth-sized planet because of precision artifacts in the fractal
    // noise generator.
    // LOD_DISTANCE_SCALE should be roughly on the order of 10 to 100 for decent performance. The gpu can handle more
    // polygons but generating the geometry on the cpu is slow
    TerrainTree(uint64_t pseed, double pLOD, double pradius, float roughness) {
        seed = pseed;
        radius = pradius;
        noise_yscaling = sqrt(radius);
        LOD_DISTANCE_SCALE = pLOD;
        MAX_LOD = 20;
        generator = new TerrainGenerator(seed, roughness);
        // 6 corners
        dvec3 initial_corners[6] = {
            dvec3(0.0, radius, 0.0),
            dvec3(0.0, -radius, 0.0),
            dvec3(0.0, 0.0, radius),
            dvec3(radius, 0.0, 0.0),
            dvec3(0.0, 0.0, -radius),
            dvec3(-radius, 0.0, 0.0)
        };
        uint32_t indices[8][3] = {
            {0, 2, 3},
            {0, 3, 4},
            {0, 4, 5},
            {0, 5, 2},
            {1, 2, 5},
            {1, 5, 4},
            {1, 4, 3},
            {1, 3, 2},
        };
        // neighboring triangles share an edge and 2 vertices
        // make sure neighbors are in ccw order so it's consistent with the vertex ordering
        uint32_t neighbors[8][3] = {
            {3, 7, 1},
            {0, 6, 2},
            {1, 5, 3},
            {2, 4, 0},
            {7, 3, 5},
            {4, 2, 6},
            {5, 1, 7},
            {6, 0, 4},
        };
        for(uint64_t i = 0; i < 8; i++){
            nodes.push_back({i, 0, 0, 0,
                    neighbors[i][0], neighbors[i][1], neighbors[i][2],
                    generator->getElevation(initial_corners[indices[i][0]]),
                    generator->getElevation(initial_corners[indices[i][1]]),
                    generator->getElevation(initial_corners[indices[i][2]]),
                    0, 0, 0,
                    initial_corners[indices[i][0]],
                    initial_corners[indices[i][1]],
                    initial_corners[indices[i][2]],
                    0, nonstd::vector<texvert>() } );
        }
    }

    // no rotation, only translation so the mesh is centered at location with spheroid = radius
    void generate(dvec3 location, uint32_t node_idx, nonstd::vector<glm::dvec3> *verts, nonstd::vector<dTri> *tris,
            uint64_t level, int min_level, uint64_t path, terrain_upload_status_enum *status) {
        if(status && *status == should_exit){
            return;
        }
        if(POTATO_MODE && node_idx % 10 == 0){
            usleep(1000);
        }
        nodes[node_idx].path = path;
        nodes[node_idx].last_used_at_frame = frame_counter;
        double noise_xzscaling = 0.0001;
        double noise_xzscaling2 = -0.00001;
        // a LOD going on here
        if(level > min_level) {
            dvec3 nodespace_center = nodes[node_idx].center();
            //double distance = glm::length(location - nodes[node_idx].verts[0]);
            double distance = glm::length(location - (nodespace_center * radius / glm::length(nodespace_center)));
            double nodeWidth = glm::length(nodes[node_idx].verts[0] - nodes[node_idx].verts[1]);
            double ratio = distance / nodeWidth;

            if(ratio > LOD_DISTANCE_SCALE || level >= MAX_LOD) {
                dTri t;
                t.type_id = VERTEX_TYPE_TERRAIN;
                dvec3 zonespace_center = {0, 0, 0};
                for(int i = 0; i < 3; i++) {
                    t.verts[i] = verts->size();
                    // scaling each point to the surface of the spheroid, subtracting location and adding the elevation value
                    glm::vec3 point = nodes[node_idx].zone_space_position(location, radius, i);
                    verts->push_back(point);
                    zonespace_center += point;
                    t.elevations[i] = nodes[node_idx].elevations[i];
                }
                zonespace_center /= 3.0;
                // the local normalized inverse gravity vector in spheroid and octahedron space
                t.normal = normalize(nodes[node_idx].verts[0]);
                
                nodes[node_idx].triangle = tris->size();
                tris->push_back(t);
                nodes[node_idx].rendered_at_level = level;

                // generate vegetation and shit
                int tree_render_level = 18;
                if(level >= tree_render_level) {
                    Prng rng;
                    uint64_t level_mask = 1;
                    for(int i = 0; i < tree_render_level; i++){
						level_mask <<= 2;
						level_mask |= 3;
                    }
                    // create a seed for this tile's level 16 parent (or the tile itself if at level 16)
                    rng.init(path & level_mask, seed);
                    //
                    // elevation 20-2000: vegetation and rocks
                    // low roughness: grass
                    // medium roughness: trees
                    // high roughness: rocks
                    // high inclination: nothing
                    uint64_t vegetation_random_value = rng.get();
                    bool should_generate = nodes[node_idx].vegetation.count == 0;
                    int num_subdivisions = MAX_LOD - level;
                    int offset = 1 + (2 * level);
                    int num_leaves = 1 << (2 * num_subdivisions);
                    glm::vec3 floatverts[3] = {
                        glm::vec3((*verts)[t.verts[0]]) - zonespace_center,
                        glm::vec3((*verts)[t.verts[1]]) - zonespace_center,
                        glm::vec3((*verts)[t.verts[2]]) - zonespace_center};
                    glm::vec3 surfacenormal = glm::normalize(glm::cross(floatverts[1] - floatverts[0], floatverts[2] - floatverts[0]));
                    glm::mat4 transformation = glm::toMat4(glm::rotation( glm::vec3(surfacenormal), glm::vec3(0.0, 1.0, 0.0)));
                    glm::vec3 object_space_verts[3] = {
                        glm::vec3(transformation * (glm::vec4(floatverts[0], 1.0))),
                        glm::vec3(transformation * (glm::vec4(floatverts[1], 1.0))),
                        glm::vec3(transformation * (glm::vec4(floatverts[2], 1.0)))};
                    float density = max(0.0f, min(1.0f, (nodes[node_idx].elevations[0] / 50.0f)));
                    density = max(0.0f, min(density, 1.0f - ((nodes[node_idx].elevations[0] - 1500.0f) / 1500.0f)));
                    float inclination = length(glm::vec3(t.normal) - surfacenormal);
                    density *= (1.0f - inclination);
                    for(int leaf = 0; leaf < num_leaves; leaf++){
                        uint64_t estimated_path = path;
                        uint64_t leaf_address = 0;
                        for(int j = 0; j < num_subdivisions; j++){
                        	leaf_address |= (  (leaf & (3ULL << (2 * j))) << (2 * (num_subdivisions - 1)) >> (4 * j)  );
                        }
                        estimated_path |= (leaf_address << offset);
                        assert((path | (leaf_address << offset)) == (path ^ (leaf_address << offset)));
                        uint64_t local_address = leaf_address;
                        uint64_t mask = local_address + (local_address << 12) + (local_address << 24) +
                            (local_address << 36) + (local_address << 48);
                        uint64_t notrandom = ((vegetation_random_value ^ estimated_path) % 1511);
                        glm::vec3 leaf_center = calculate_center(num_subdivisions, leaf_address,
                                object_space_verts[0], object_space_verts[1], object_space_verts[2]);
                        float probability_score = notrandom / 1511.0f;
                        if(density > probability_score) {
                            // generate vegetation if it doesn't exist yet
                            if(should_generate) {
                                double h_trunk = 3.0;
                                double r_trunk = 0.2;
                                double h_canopy = 10.0;
                                double r_canopy = 4.0;
                                mktree(&nodes[node_idx].vegetation, h_trunk, r_trunk, h_canopy, r_canopy, leaf_center);
                            }
                        }
                    }
                    // transform vegetation to node space coordinates
                    glm::mat4 rotation_matrix = glm::toMat4(glm::rotation(glm::vec3(0.0, 1.0, 0.0), surfacenormal));
                    for(int i = 0; i < nodes[node_idx].vegetation.count; i+= 3) {
                        dTri t2;
                        t2.type_id = *(uint32_t*)(&nodes[node_idx].vegetation[i].xyz.w);
                        for(int j = 0; j < 3; j++) {
                            t2.verts[j] = verts->size();
                            glm::vec4 point4 = nodes[node_idx].vegetation[i + j].xyz;
                            point4.w = 1.0f;
                            point4 = rotation_matrix * point4;
                            glm::dvec3 point = glm::dvec3(point4) + zonespace_center;
                            verts->push_back(point);
                            //t2.elevations[j] = nodes[node_idx].vegetation[i + j].uvw.x;
                            t2.elevations[j] = inclination;
                        }
                        t2.normal = t.normal;
                        tris->push_back(t2);
                    }
                }
                return;
            }
        }
        // procedurally generate terrain height values on demand
        if( ! nodes[node_idx].first_child) {
            nodes[node_idx].first_child = (uint32_t)nodes.size();
            vec3 new_verts[3] = {
                (nodes[node_idx].verts[0] + nodes[node_idx].verts[1]) * 0.5f,
                (nodes[node_idx].verts[1] + nodes[node_idx].verts[2]) * 0.5f,
                (nodes[node_idx].verts[2] + nodes[node_idx].verts[0]) * 0.5f};
            vec3 scaled_verts[12] = {
                glm::normalize(new_verts[0]) * radius * noise_xzscaling,
                glm::normalize(new_verts[1]) * radius * noise_xzscaling,
                glm::normalize(new_verts[2]) * radius * noise_xzscaling,
                glm::normalize(nodes[node_idx].verts[0]) * radius * noise_xzscaling,
                glm::normalize(nodes[node_idx].verts[1]) * radius * noise_xzscaling,
                glm::normalize(nodes[node_idx].verts[2]) * radius * noise_xzscaling,
                glm::normalize(new_verts[0]) * radius * noise_xzscaling2,
                glm::normalize(new_verts[1]) * radius * noise_xzscaling2,
                glm::normalize(new_verts[2]) * radius * noise_xzscaling2,
                glm::normalize(nodes[node_idx].verts[0]) * radius * noise_xzscaling2,
                glm::normalize(nodes[node_idx].verts[1]) * radius * noise_xzscaling2,
                glm::normalize(nodes[node_idx].verts[2]) * radius * noise_xzscaling2};
            float elevations[12];
            float roughnesses[12];
            generator->getMultiple(elevations, roughnesses, scaled_verts, 12, 0.2);

            for(int i = 0; i < 6; i++) {
                elevations[i] += (elevations[i+6] * 5.0);
                roughnesses[i] += (roughnesses[i+6]);
                lowest_point = glm::min(elevations[i] * (float)noise_yscaling, lowest_point);
                highest_point = glm::max(elevations[i] * (float)noise_yscaling, highest_point);
            }

            // the center triangle neighbors the other 3 triangles, that's easy
            nodes.push_back({0, 0, 0, 0,
                (uint32_t)nodes.size() + 1, (uint32_t)nodes.size() + 2, (uint32_t)nodes.size() + 3,
                elevations[0] * noise_yscaling,
                elevations[1] * noise_yscaling,
                elevations[2] * noise_yscaling,
                roughnesses[0],
                roughnesses[1],
                roughnesses[2],
                new_verts[0],
                new_verts[1],
                new_verts[2],
                0, nonstd::vector<texvert>() });
            // the other 3 triangles neighbor the center triangle and child trangles of the parent's neighbors
            // we can't know the parent's neighbors' children because they may not exist yet
            for(int i = 0; i < 3; i++) {
                nodes.push_back({0, 0, 0, 0,
                    0, 0, (uint32_t)nodes.size(),
                    elevations[i + 3] * noise_yscaling,
                    elevations[i] * noise_yscaling,
                    elevations[((i + 2) % 3)] * noise_yscaling,
                    roughnesses[i + 3],
                    roughnesses[i],
                    roughnesses[((i + 2) % 3)],
                    nodes[node_idx].verts[i],
                    new_verts[i],
                    new_verts[(i + 2) % 3],
                    0, nonstd::vector<texvert>() });
            }
        }
        // we need to go deeper
        for(uint64_t i = 0; i < 4; i++) {
            generate(location, nodes[node_idx].first_child + i, verts, tris, level + 1, min_level,
                    path | (i << (1 + 2 * level)), status);
        }
    }

    ttnode* operator[](uint64_t tile) {
        ttnode* n = &nodes[tile & 7];
        uint64_t i = 0;
        while(n->first_child){
            ++i;
            n = &nodes[n->first_child + ((tile >> (1 + 2 * i)) & 3)];
        }
        return n;
    }

    // pos must be a direction from the center of the celestial, length >= 1mm
    ttnode* operator[](dvec3 pos) {
        double manhattan_length = abs(pos[0]) + abs(pos[1]) + abs(pos[2]);
        if(manhattan_length < 0.001){
            return nil;
        }
        // location is pos projected onto the surface of the node space octahedron
        dvec3 location = pos * (radius / manhattan_length);
        
        uint64_t tile = (location[1] > 0) ? 0 : 4;
        uint64_t i = 0;
        ttnode* n = nil;
        do {
            n = &nodes[tile];
            double shortest_distance = glm::length(location - n->center());
            for(uint64_t i = 1; i < 4; i++){
                double distance = glm::length(location - nodes[tile + i].center());
                if(distance < shortest_distance){
                    shortest_distance = distance;
                    n = &nodes[tile + i];
                }
            }
            tile = n->first_child;
            ++i;
        } while (n->first_child);
        return n;
    }

    // to keep things simple I define a zone as a triangle at some reasonable subdivision level
    // (~1 km on earth's surface?)
    // and a zone's id will be encoded in a 64 bit unsigned integer
    // actually any terrain node can be encoded like this since the terrain tree is only 18 levels deep (=37 bits)
    // the two least significant bits encode which quadrant of the hemisphere
    // the third least significant bit separates north/south
    // for every 2 additional bits we encode one more level of subdivision in clockwise order


    // breadth-first traversal of all the nodes in the terrain tree
    // TODO: somehow set the correct neighbor pointers for each node
    // initially the top layer has all its neighbor pointers set correctly
    // for each layer we must set each missing neighbor pointer to the correct child of the parent's neighbor
    // if it exists, or the parent's neighbor otherwise.
    //
    // some nodes on the top level will legitimately have 0 as their neighbor but as long as min_level is at least
    // 2 the second level nodes should all have neighbors on their own level.
    //
    void get_to_know_the_neighbors(nonstd::vector<glm::dvec3> *verts, nonstd::vector<dTri> *tris) {
        nonstd::vector<uint32_t> stack;
        nonstd::vector<uint32_t> parents;
        stack.reserve(nodes.size());
        for(uint32_t i = 0; i < 8; i++) {
            stack.push_back(i);
            parents.push_back(i); // set the root nodes to be their own parents, doesn't matter, could be anything
        }
        for(uint32_t i = 0; i < stack.size(); i++) {
            ttnode &n = nodes[stack[i]];

            if(i > 7) {
                ttnode &parent = nodes[parents[i]];
                for(int j = 0; j < 2; j++){
                    if(n.neighbors[j] == 0){
                        ttnode &parents_neighbor = nodes[parent.neighbors[j]];
                        if( ! parents_neighbor.first_child) { // BINGO! we have a node whose vertex must be adjusted
                            n.neighbors[j] = parent.neighbors[j];
                        } else { // have to figure out which of the parent's children is node's neighbor
                            // le sigh
                            
                        }
                    }
                }
            }

            if( ! n.first_child) {
                continue;
            }
            for(int j = 0; j > 4; j++){
                stack.push_back(n.first_child + j);
                parents.push_back(i);
            }
        }
    }

    dMesh buildMesh(dvec3 location, int min_subdivisions, terrain_upload_status_enum *status) {
        if(verbose) std::cout << "building mesh from vantage point (" << location.x << ", " << location.y << ", " << location.z << ")\n";
        nonstd::vector<uint32_t> node_indices;
        nonstd::vector<glm::dvec3> verts;
        nonstd::vector<dTri> tris;
        for(int i = 0; i < 8; i++) {
            generate(location * radius / length(location), i, &verts, &tris, 1, min_subdivisions, i, status);
        }
        uint64_t num_verts = verts.size();
        uint64_t num_tris = tris.size();
        dvec3 *vertices = (dvec3*)malloc(verts.size() * sizeof(dvec3) + tris.size() * sizeof(dTri));
        dTri *triangles = (dTri*)&vertices[verts.size()];

        memcpy(vertices, &verts[0], verts.size() * sizeof(dvec3));
        memcpy(triangles, &tris[0], tris.size() * sizeof(dTri));
        if(verbose) std::cout << tris.size() << " triangles generated\n";
        verts.destroy();
        tris.destroy();
        return dMesh(vertices, num_verts, triangles, num_tris);
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
    nonstd::vector<Celestial> orbiting_bodies;

    Celestial(Celestial &&rhs) noexcept {
        memcpy(this, &rhs, sizeof(Celestial));
        std::cout << "Celestial " << name << " memcpy copy constructor\n";
    }

    Celestial(uint64_t pseed, double pLOD, std::string pname, double pradius, float proughness, Celestial *pnearest_star) {
        bzero(this, sizeof(Celestial));
        seed = pseed;
        name = pname;
        new(&terrain) TerrainTree(pseed, pLOD, pradius, proughness);
        auto time_begin = now();
        if(verbose) std::cout << "Generating mesh..\n";
        new(&body) PhysicsObject(terrain.buildMesh(dvec3(0, 6.37101e6, 0), 3, nil), nil);
        auto time_used = std::chrono::duration_cast<std::chrono::microseconds>(now() - time_begin).count();
        if(verbose) std::cout << "Celestial " << name << ": " << body.mesh.num_tris << " triangles procedurally generated in " << time_used/1000.0 << "ms\n";
        if(verbose) std::cout << "Lowest point: " << terrain.lowest_point << ", highest point: " << terrain.highest_point << "\n";
        surface_temp_min = 183.0;
        surface_temp_max = 331.0;
        nearest_star = pnearest_star;
    }
};

enum state_update_type {
    resource_deposit,
    fluidsim,
    telemetry,
    destruction,
    creation,
    commands
};


// here we can put all the dirty (updated) objects between two physics states
// typically from one tick to the next but can also be used as savegame format and for client<->server updates
// by not modifying the physics state while doing the physics calculations but instead accumulating updates in a
// copy of every physics object and applying them all in a batch we can make the game engine multithreaded and thread
// safe by default without jumping through hoops or inviting nasal demons
struct state_update {
    state_update_type update_type;
    void *data;
    uint64_t count;
    uint64_t tick;
};




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

moment of intertia: the main body has a precomputed intertia tensor without the limbs. The masses of the limbs
must be added each tick in order to produce the true intertia tensor taking each limb's center of mass into account.

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


