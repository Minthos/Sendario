#include <endian.h>


struct UID {
    uint64_t data64[2];

    uint64_t getPID();
    void setPID(uint64_t);
    uint64_t getOID();
    void setOID(uint64_t);
    uint32_t getIdx();
    void setIdx(uint32_t);
};

#if __BYTE_ORDER == __LITTLE_ENDIAN

uint64_t getPID() {
    return data[0] & 0x0000FFFFFFFFFFFF;
}

void setPID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0xFFFF000000000000) | (in & 0x0000FFFFFFFFFFFF);
}

uint64_t getOID() {
    return (((data[0] & 0xFFFF000000000000) >> 48) | ((data[1] & 0x00000000FFFFFFFF) << 16));
}

void setOID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0x0000FFFFFFFFFFFF) | ((in & 0x000000000000FFFF) << 48)
    data[1] = (data[1] & 0xFFFFFFFF00000000) | (in & 0x0000FFFFFFFF0000 >> 16)
}

uint32_t getIdx() {
    return ((uint32_t*)(data))[3];
}

void setIdx(uint32_t in) {
    ((uint32_t*)(data))[3] = in;
}


#endif

// TODO: support big endian hardware









