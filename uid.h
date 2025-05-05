#ifndef UID_H
#define UID_H

#include <endian.h>
#include <stdint.h>
#include <cassert>
#include "physics.h"

struct UID {
    uint64_t data[2];

    uint64_t getPID();
    void setPID(uint64_t);
    uint64_t getOID();
    void setOID(uint64_t);
    uint32_t getIdx() const;
    void setIdx(uint32_t);

    uint64_t hash() const;
};

struct UIDHashTable {
    nonstd::vector<UID> slots;
    size_t size = 0;

    void init(size_t initial_capacity = 16);
    void destroy();

    void insert(const UID& key);
    void remove(const UID& key);
    void clear();
    uint32_t operator[](const UID& key);
};

#endif // UID_H
