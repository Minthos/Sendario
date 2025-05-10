#include "uid.h"
#include <stdexcept>
#include <iomanip>

#if __BYTE_ORDER == __LITTLE_ENDIAN

// UID methods unchanged
uint64_t UID::getPID() const { return data[0] & 0x0000FFFFFFFFFFFF; }

void UID::setPID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0xFFFF000000000000) | (in & 0x0000FFFFFFFFFFFF);
}

uint64_t UID::getOID() const {
    return (((data[0] & 0xFFFF000000000000) >> 48) | ((data[1] & 0x00000000FFFFFFFF) << 16));
}

void UID::setOID(uint64_t in) {
    assert((in & 0xFFFF000000000000) == 0);
    data[0] = (data[0] & 0x0000FFFFFFFFFFFF) | ((in & 0x000000000000FFFF) << 48);
    data[1] = (data[1] & 0xFFFFFFFF00000000) | ((in & 0x0000FFFFFFFF0000) >> 16);
}

uint32_t UID::getIdx() const {
    return data32[3]; 
}

void UID::setIdx(uint32_t in) { 
    data32[3] = in;
}

uint64_t UID::hash() const {
    uint64_t h = data[0] ^ data32[2];
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
}

void UIDHashTable::init(size_t initial_capacity) {
    bzero((void*)this, sizeof(UIDHashTable));
    assert(initial_capacity > 0);
    slots = nonstd::vector<UID>();
    slots.reserve(initial_capacity);
    slots.wipe();
}

void UIDHashTable::destroy() {
    slots.destroy();
    size = 0;
}

void UIDHashTable::wipe() {
    slots.wipe();
    size = 0;
}

void UIDHashTable::insert(const UID& key) {
    if(key.data[0] == 0 && key.data32[2] == 0) {
        zero_value = key.getIdx();
        if(!(flags & 1))
            size++;
        flags |= 1;
        return;
    }

    if (((size + 1) * 2) >= slots.capacity) { // Load factor < 0.5
        reserve(max(16, slots.capacity * 2));
    }

    assert(slots.capacity > 0);
    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while (probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            slot = key;
            size++;
            return;
        }
        if (slot.data[0] == key.data[0] && slot.data32[2] == key.data32[2]) {
            slot = key;
            std::cout << "double insert\n";
            return;
        }
        idx = (idx + 1) % slots.capacity; // Linear probing
        probe_count++;
    }
    assert(false);
}


void UIDHashTable::remove(const UID& key)
{
    if (key.data[0] == 0 && key.data32[2] == 0) {
        assert(flags & 1);
        zero_value = 0;
        size--;
        flags &= (~1);
        return;
    }

    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while(probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if(slot.data[0] == 0 && slot.data[1] == 0)
            return;

        if(slot.data[0] == key.data[0] && slot.data32[2] == key.data32[2])
        {
            size--;
            slot = UID{0ULL,0ULL};
            size_t current_idx = idx;
            size_t next_idx = (current_idx + 1) % slots.capacity;
            while (!(slots[next_idx].data[0] == 0 && slots[next_idx].data[1] == 0)) {
                size_t ideal_idx = slots[next_idx].hash() % slots.capacity;
                bool should_move = false;
                if (current_idx < next_idx) {
                    // Normal case (no wrap between current and next)
                    should_move = (ideal_idx <= current_idx) || (ideal_idx > next_idx);
                } else {
                    // Wrap-around case (next_idx wrapped around to 0)
                    should_move = (ideal_idx <= current_idx) && (ideal_idx > next_idx);
                }
                if (should_move) {
                    slots[current_idx] = slots[next_idx];
                    slots[next_idx] = UID{0ULL, 0ULL};
                    current_idx = next_idx;
                }
                next_idx = (next_idx + 1) % slots.capacity;
            }
            return;
        }
        idx = (idx + 1) % slots.capacity;
        probe_count++;
    }
    assert(false);
}

void UIDHashTable::reserve(size_t new_capacity) {
    assert(new_capacity > size);
    nonstd::vector<UID> old_slots = slots;
    nonstd::vector<UID> new_slots;
    new_slots.reserve(new_capacity);
    bzero(new_slots.data, new_slots.capacity * sizeof(UID));
    slots = new_slots;
    size = 0 + (flags & 1);

    for (size_t i = 0; i < old_slots.capacity; i++) {
        if (old_slots[i].data[0] != 0 || old_slots[i].data[1] != 0) {
            insert(old_slots[i]);
        }
    }
    
    old_slots.destroy();
}

uint32_t& UIDHashTable::operator[](const UID& key) {
    if(key.data[0] == 0 && key.data32[2] == 0) {
        if(flags & 1)
            return zero_value;
        assert(0);
        // exceptions prevent inlining
        //throw std::out_of_range(constfstr("Key not found: %llX %llX", key.getPID(), key.getOID() ));
    }

    size_t idx = key.hash() % slots.capacity;
    size_t probe_count = 0;
    while (probe_count < slots.capacity) {
        UID& slot = slots[idx];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            assert(0);
            // exceptions prevent inlining
//            throw std::out_of_range(constfstr("Key not found: (%llX %llX) idx: %lu, capacity: %lu", key.getPID(), key.getOID(), idx, slots.capacity ));
        }
        if (slot.data[0] == key.data[0] && slot.data32[2] == key.data32[2]) {
            return (uint32_t&)(slot.data32[3]);
        }
        idx = (idx + 1) % slots.capacity;
        probe_count++;
    }
    assert(false);
}

nonstd::vector<int> UIDHashTable::count_probe_distances() const {
    nonstd::vector<int> counts = nonstd::vector<int>();
    counts.reserve(16);
    bzero(counts.data, counts.capacity * sizeof(int));
    counts.count = 16;
   
    assert(slots.capacity > 0);
    assert(size > 0);
    for (size_t i = 0; i < slots.capacity; i++) {
        const UID& slot = slots.data[i];
        if (slot.data[0] == 0 && slot.data[1] == 0) {
            continue;
        }
        
        // Calculate ideal position
        size_t ideal_pos = slot.hash() % slots.capacity;
        
        // Calculate actual distance moved (handling wrap-around)
        size_t distance;
        if (i >= ideal_pos) {
            distance = i - ideal_pos;
        } else {
            distance = slots.capacity - ideal_pos + i;
        }
        
        // Increment appropriate counter (capping at 8)
        if (distance < counts.count) {
            counts[distance]++;
        } else {
            counts[counts.count - 1]++;
        }
    }
    
    return counts;
}

#endif // __BYTE_ORDER == __LITTLE_ENDIAN
