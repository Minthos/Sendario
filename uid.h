#ifndef UID_H
#define UID_H

#include <endian.h>
#include <stdint.h>
#include <cassert>

struct UID {
    uint64_t data64[2];

    uint64_t getPID();
    void setPID(uint64_t);
    uint64_t getOID();
    void setOID(uint64_t);
    uint32_t getIdx();
    void setIdx(uint32_t);
};

#endif // UID_H
