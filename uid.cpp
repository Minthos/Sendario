#include "uid.h"
#include <stdexcept>

#if __BYTE_ORDER == __LITTLE_ENDIAN

// UID methods unchanged
uint64_t UID::getPID() { return data[0] & 0x0000FFFFFFFFFFFF; }
void UID::setPID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0xFFFF000000000000) | (in & 0x0000FFFFFFFFFFFF);
}
uint64_t UID::getOID() {
    return (((data[0] & 0xFFFF000000000000) >> 48) | ((data[1] & 0x00000000FFFFFFFF) << 16));
}
void UID::setOID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0x0000FFFFFFFFFFFF) | ((in & 0x000000000000FFFF) << 48);
    data[1] = (data[1] & 0xFFFFFFFF00000000) | ((in & 0x0000FFFFFFFF0000) >> 16);
}
uint32_t UID::getIdx() const {
    return (uint32_t)(data[1] >> 32); 
}
void UID::setIdx(uint32_t in) { 
    data[1] = (data[1] & 0x00000000FFFFFFFF) | ((uint64_t)in << 32); 
}
uint64_t UID::hash() const {
    uint64_t h = data[0] ^ (data[1] & 0x00000000FFFFFFFF);
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
}

void UIDHashTable::init(size_t initial_capacity) {
    slots = nonstd::vector<UID>();
    slots.reserve(initial_capacity);
    bzero(slots.data(), slots.size() * sizeof(UID));
    size = 0;
}

void UIDHashTable::destroy() {
    slots.destroy();
    size = 0;
}

void UIDHashTable::clear() {
    bzero(slots.data(), slots.size() * sizeof(UID));
    size = 0;
}

void UIDHashTable::insert(const UID& key) {
    if (size * 2 >= slots.size()) { // Load factor < 0.5
        reserve(slots.size() * 2);
    }

    size_t idx = key.hash() % slots.size();
    size_t probe_count = 0;
    while (probe_count < slots.size()) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            slot = key;
            size++;
            return;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            slot = key;
            return;
        }
        idx = (idx + 1) % slots.size(); // Linear probing
        probe_count++;
    }
    assert(false);
}

bool UIDHashTable::remove(const UID& key) {
    size_t idx = key.hash() % slots.size();
    size_t probe_count = 0;
    while (probe_count < slots.size()) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            return false;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            size--;
            slot = UID{0, 0};

            size_t current_idx = idx;
            size_t next_idx = (current_idx + 1) % slots.size();
            while (!(slots[next_idx].data[0] == 0 && slots[next_idx].data[1] == 0)) {
                size_t ideal_idx = slots[next_idx].hash() % slots.size();
                if ((ideal_idx <= current_idx) || (current_idx < next_idx && ideal_idx > next_idx)) {
                    slots[current_idx] = slots[next_idx];
                    slots[next_idx] = UID{0, 0};
                    current_idx = next_idx;
                }
                next_idx = (next_idx + 1) % slots.size();
            }
            return true;
        }
        idx = (idx + 1) % slots.size(); // Linear probing
        probe_count++;
    }
    assert(false);
}

void UIDHashTable::reserve(size_t new_capacity) {
    nonstd::vector<UID> *old_slots = &slots;
    nonstd::vector<UID> new_slots;
    new_slots.reserve(new_capacity);
    bzero(new_slots.data(), new_slots.size() * sizeof(UID));
    slots = new_slots;
    
    for (const UID& slot : *old_slots) {
        if (slot.data[0] != 0 || slot.data[1] != 0) {
            slots = new_slots;
            insert(slot);
        }
    }
    
    old_slots->destroy();
}

uint32_t UIDHashTable::operator[](const UID& key) {
    size_t idx = key.hash() % slots.size();
    size_t probe_count = 0;
    while (probe_count < slots.size()) {
        const UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            slot = key;
            size++;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            return slot.getIdx();
        }
        idx = (idx + 1) % slots.size(); // Linear probing
        probe_count++;
    }
    assert(false);
}


#endif // __BYTE_ORDER == __LITTLE_ENDIAN
