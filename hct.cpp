/*
Can the whole universe be fluid simulated?

the parts that players aren't interacting with are not simulated in real time

and probably not bullets, yeah? sigh.. ok fine bullets too.

so a bullet speeds through the simulation. around the bullet is a volume of turbulent air with steep
velocity gradients and small grid size. as the bullet moves from one grid node to its neighbor the
neighbor may need to be subdivided, if it wasn't already from the pressure wave preceding the bullet.
as the gradients smooth out the grid nodes can be merged back together.

how fine grid we can use for bullets depends on how much compute we can spend on each bullet.
not much in a massive fleet fight. round it to zero-ish. but for a solo adventure mission it could be considerable.

we can fluid simulate indoor environments while players are interacting with them.
erosion, disasters and player made changes to the world can be simulated while they are happening and then saved.
in big fleet fights we will limit the resolution of the bullet simulation as much as we need to gain performance.
maybe simply disable the part where bullets cause the grid to subdivide.

----


in outdoor environments, why don't we do fluid simulations in 2d and only on surfaces?
and let's call an exhaust plume a surface. so it's a 2d fluid simulation wrapped around a growing snake of
near-cylindrical bands.
and then we simulate the surface conditions of objects moving through atmosphere and the surface conditions of liquids
and some layers of fog and clouds for good measure. Let's give each simulation 2 height values each per 2d grid cell,
bottom and top (inner and outer respectively). some velocity information, density, temperature, presence of liquids
and gases other than atmosphere

and all of this happens on the gpu so we can access any of this easily from shaders.

conceptually a rocket plume starts at the base of the rocket - the structure of the plume is fixed relative to the
rocket while the mass flows through it and produces new bands at the end of the plume. as their mass velocity
dissipates further from the rocket their positional velocity lerps closer to the ambient wind velocity and away from
the rocket's velocity.

----

the macroscale grid for a star system can have 64 bit integer coordinates.
the smallest resolution we need is just to serve as origo for floating point calculations.

indoor grid should have (2.4m)^3 default cell size to represent standard rooms and corridors.
1/4 of 2.4 is 0.6, 1/4 of 0.6 is 0.15, 1/4 of 0.15 is 0.0375. maybe best not to go any lower.

hmm, a 0.0375 m resolution gives a 64 bit integer coordinate system 6.917529027641082e+17 meters range.
that's 73.11625650186113 light years apparently. should be more than enough
to model a star system. now I'm tempted to use this as global coordinate system that's unambiguous throughout a
star system. local physics simulations can be done with floating point values offset from cells in this grid.

but.. I'm still going to want separate grids for indoor environments so gravity, walls and floors can be axis-aligned.
and I want the grid for a planet to be relative to the planet, not the star.

ok so, the 2.4m cell is the base of the local coordinate system used for physics and rendering. objects that cross into another cell must add or subtract 2.4m to the correct coordinate value so they will be positioned relative to the new
cell.

the player character's cell is the base of the local coordinate system used for rendering.

need to refactor hct to integer coordinates.

----

I just had an idea for mercenary work.
people need to log off and relax right? sign up to a mercenary service and install intrusion detection systems on
your properties. if you're under attack while offline you will get an alart and the system will automatically issue
a mercenary contract to defend. you can configure the criteria it uses to select candidates, the payment, etc in
proportion to the estimated threat.
the system maintains a database of mercenaries and rates them based on performance. you might for example configure
your software to select the highest rated mercenary who has signed up before the deadline runs out.

----

oh and here's hct.cpp, enjoy untested machine translation

*/


#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>

const int MAXDEPTH = 26;

struct Vector {
    double x, y, z;

    Vector operator-(const Vector& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vector operator*(double factor) const {
        return {x * factor, y * factor, z * factor};
    }
};

struct BBox {
    Vector center;
    double halfsize;

    Vector bottom() const {
        return {center.x - halfsize, center.y - halfsize, center.z - halfsize};
    }

    bool contains(const Vector& point) const {
        return std::abs(point.x - center.x) <= halfsize &&
               std::abs(point.y - center.y) <= halfsize &&
               std::abs(point.z - center.z) <= halfsize;
    }

    bool intersects(const BBox& other) const {
        return std::abs(center.x - other.center.x) <= (halfsize + other.halfsize) &&
               std::abs(center.y - other.center.y) <= (halfsize + other.halfsize) &&
               std::abs(center.z - other.center.z) <= (halfsize + other.halfsize);
    }

    BBox selectQuadrant(uint8_t index) const {
        double quarterSize = halfsize * 0.5;
        double threeEighthsSize = halfsize * 0.75;
        return {
            {center.x + (index & 0x03) * quarterSize - threeEighthsSize,
             center.y + ((index >> 2) & 0x03) * quarterSize - threeEighthsSize,
             center.z + ((index >> 4) & 0x03) * quarterSize - threeEighthsSize},
            quarterSize
        };
    }
};

template <typename T>
struct HctItem {
    T data;
    Vector position;
};

template <typename T>
class HctNode {
public:
    uint64_t bit_field = 0;
    std::vector<HctNode*> children;
    std::vector<HctItem<T>> data;

    ~HctNode() {
        for (auto child : children) {
            delete child;
        }
    }

    std::vector<HctItem<T>> everything() const {
        if (bit_field == 0) {
            return data;
        } else {
            std::vector<HctItem<T>> allData;
            for (const auto& child : children) {
                auto childData = child->everything();
                allData.insert(allData.end(), childData.begin(), childData.end());
            }
            return allData;
        }
    }

    void subdivide(int depth, int binSize, const std::vector<uint8_t>& path);
};

template <typename T>
class HctTree {
public:
    HctNode<T> root;
    BBox dims;
    int binSize;
    int numItems = 0;

    HctTree(double initialSize, int binSize = 1024) : binSize(binSize) {
        dims = { {0, 0, 0}, initialSize };
    }

    std::vector<uint8_t> quickResolve(const Vector& position) const {
        Vector p = (position - dims.bottom()) * (double(1 << (MAXDEPTH * 2)) / (dims.halfsize * 0.5));
        int x = static_cast<int>(p.x), y = static_cast<int>(p.y), z = static_cast<int>(p.z);
        std::vector<uint8_t> result;
        for (int i = 0; i < MAXDEPTH; ++i) {
            result.push_back((x & 3) | ((y & 3) << 2) | ((z & 3) << 4));
            x >>= 2; y >>= 2; z >>= 2;
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    void insert(const T& item, const Vector& position);
    void remove(const T& item, const Vector& position);
    std::vector<T> values() const;
};

template <typename T>
void HctTree<T>::insert(const T& item, const Vector& position) {
    if (!dims.contains(position)) {
        std::cerr << "Error: Position outside bounds!\n";
        assert(false);
    }

    auto path = quickResolve(position);
    HctNode<T>* currentNode = &root;
    int depth = 0;

    while (true) {
        if (currentNode->bit_field == 0) {
            currentNode->data.push_back({item, position});
            if (currentNode->data.size() > binSize && depth < MAXDEPTH) {
                currentNode->subdivide(depth, binSize, path);
            }
            ++numItems;
            return;
        }

        uint8_t index = path[depth];
        if (currentNode->bit_field & (1ULL << index)) {
            auto it = std::find_if(currentNode->children.begin(), currentNode->children.end(),
                                   [index, currentNode](HctNode<T>* child) {
                                       return child->bit_field & (1ULL << index);
                                   });
            if (it != currentNode->children.end()) {
                currentNode = *it;
            }
        } else {
            HctNode<T>* newNode = new HctNode<T>();
            currentNode->bit_field |= (1ULL << index);
            currentNode->children.push_back(newNode);
            currentNode = newNode;
        }
        ++depth;
    }
}

template <typename T>
void HctNode<T>::subdivide(int depth, int binSize, const std::vector<uint8_t>& path) {
    std::vector<int> indices;
    for (const auto& item : data) {
        indices.push_back(path[depth]);
    }

    std::vector<std::pair<int, HctItem<T>>> pairs;
    for (size_t i = 0; i < data.size(); ++i) {
        pairs.push_back({indices[i], data[i]});
    }
    std::sort(pairs.begin(), pairs.end());

    data.clear();
    for (const auto& [index, item] : pairs) {
        if (!(bit_field & (1ULL << index))) {
            bit_field |= (1ULL << index);
            children.push_back(new HctNode<T>());
        }
        children.back()->data.push_back(item);
    }

    for (auto* child : children) {
        if (child->data.size() > binSize && depth + 1 < MAXDEPTH) {
            child->subdivide(depth + 1, binSize, path);
        }
    }
}

template <typename T>
std::vector<T> HctTree<T>::values() const {
    auto items = root.everything();
    std::vector<T> results;
    for (const auto& item : items) {
        results.push_back(item.data);
    }
    return results;
}




