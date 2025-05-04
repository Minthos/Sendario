#include "uid.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN

uint64_t UID::getPID() {
    return data64[0] & 0x0000FFFFFFFFFFFF;
}

void UID::setPID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data64[0] = (data64[0] & 0xFFFF000000000000) | (in & 0x0000FFFFFFFFFFFF);
}

uint64_t UID::getOID() {
    return (((data64[0] & 0xFFFF000000000000) >> 48) | ((data64[1] & 0x00000000FFFFFFFF) << 16));
}

void UID::setOID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data64[0] = (data64[0] & 0x0000FFFFFFFFFFFF) | ((in & 0x000000000000FFFF) << 48);
    data64[1] = (data64[1] & 0xFFFFFFFF00000000) | ((in & 0x0000FFFFFFFF0000) >> 16);
}

uint32_t UID::getIdx() {
    return ((uint32_t*)(data64))[3];
}

void UID::setIdx(uint32_t in) {
    ((uint32_t*)(data64))[3] = in;
}

#endif
