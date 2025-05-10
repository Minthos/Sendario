#ifndef UID_H
#define UID_H

#include <endian.h>
#include <stdint.h>
#include <cassert>
#include "physics.h"

struct UID {
    union {
        uint64_t data[2];
        uint32_t data32[4];
    };

    uint64_t getPID() const;
    void setPID(uint64_t);
    uint64_t getOID() const;
    void setOID(uint64_t);
    uint32_t getIdx() const;
    void setIdx(uint32_t);

    uint64_t hash() const;
};

struct UIDHashTable {
    nonstd::vector<UID> slots;
    size_t size = 0;
    uint32_t zero_value = 0;
    uint32_t flags = 0;

    void init(size_t initial_capacity = 16);
    void reserve(size_t new_capacity);
    void destroy();
    void wipe();

    void insert(const UID& key);
    void remove(const UID& key);
    uint32_t& operator[](const UID& key);
    
    // Count how many values have been moved N places from their ideal position (0-8)
    nonstd::vector<int> count_probe_distances() const;
};

#endif // UID_H
