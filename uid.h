#ifndef UID_H
#define UID_H

#include <endian.h>
#include <stdint.h>
#include <cassert>
#include "physics.h" // for nonstd::vector

struct UID {
    uint64_t data[2];

    uint64_t getPID();
    void setPID(uint64_t);
    uint64_t getOID();
    void setOID(uint64_t);
    uint32_t getIdx();
    void setIdx(uint32_t);

    uint64_t hash() const;
};

struct UIDHashTable {
    struct Entry {
        UID key;
        uint32_t value;
    };

    nonstd::vector<nonstd::vector<Entry>> buckets;
    size_t size = 0;

    UIDHashTable(size_t initial_capacity = 16);
    ~UIDHashTable();

    void insert(const UID& key, uint32_t value);
    bool find(const UID& key, uint32_t& out_value);
    bool remove(const UID& key);
    void clear();
    void reserve(size_t new_capacity);
};

#endif // UID_H
