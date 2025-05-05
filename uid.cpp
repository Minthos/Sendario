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
    assert(initial_capacity > 0);
    slots = nonstd::vector<UID>();
    slots.reserve(initial_capacity);
    bzero(slots.data, slots.capacity * sizeof(UID));
    size = 0;
}

void UIDHashTable::destroy() {
    slots.destroy();
    size = 0;
}

void UIDHashTable::clear() {
    bzero(slots.data, slots.capacity * sizeof(UID));
    size = 0;
}

void UIDHashTable::insert(const UID& key) {
    if (((size + 1) * 2) >= slots.capacity) { // Load factor < 0.5
        reserve(max(16, slots.capacity * 2));
    }

    assert(slots.capacity > 0);
    std::cerr << "before insert: " << slots.capacity << "\n";
    std::cerr << "before insert: " << size << "\n";
    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while (probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            slot = key;
            size++;
            std::cerr << "after insert: " << slots.capacity << "\n";
            std::cerr << "after insert: " << size << "\n";
            return;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            slot = key;
            std::cerr << "after update: " << slots.capacity << "\n";
            std::cerr << "after update: " << size << "\n";
            return;
        }
        idx = (idx + 1) % slots.capacity; // Linear probing
        probe_count++;
    }
    assert(false);
}

bool UIDHashTable::remove(const UID& key) {
    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while (probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            return false;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            size--;
            slot = UID{0, 0};

            size_t current_idx = idx;
            size_t next_idx = (current_idx + 1) % slots.capacity;
            while (!(slots[next_idx].data[0] == 0 && slots[next_idx].data[1] == 0)) {
                size_t ideal_idx = slots[next_idx].hash() % slots.capacity;
                if ((ideal_idx <= current_idx) || (current_idx < next_idx && ideal_idx > next_idx)) {
                    slots[current_idx] = slots[next_idx];
                    slots[next_idx] = UID{0, 0};
                    current_idx = next_idx;
                }
                next_idx = (next_idx + 1) % slots.capacity;
            }
            return true;
        }
        idx = (idx + 1) % slots.capacity; // Linear probing
        probe_count++;
    }
    assert(false);
}

void UIDHashTable::reserve(size_t new_capacity) {
    assert(new_capacity > slots.capacity);
    nonstd::vector<UID> old_slots = slots;
    nonstd::vector<UID> new_slots;
    new_slots.reserve(new_capacity);
    bzero(new_slots.data, new_slots.capacity * sizeof(UID));
    slots = new_slots;
    size = 0;

    for (size_t i = 0; i < old_slots.capacity; i++) {
        if (old_slots[i].data[0] != 0 || old_slots[i].data[1] != 0) {
            insert(old_slots[i]);
        }
    }
    
    old_slots.destroy();
}

uint32_t UIDHashTable::operator[](const UID& key) {
    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while (probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            slot = key;
            size++;
        }
        if (slot.data[0] == key.data[0] &&
            (slot.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            return slot.getIdx();
        }
        idx = (idx + 1) % slots.capacity; // Linear probing
        probe_count++;
    }
    assert(false);
}


#endif // __BYTE_ORDER == __LITTLE_ENDIAN
