#include "uid.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN

uint64_t UID::getPID() {
    return data[0] & 0x0000FFFFFFFFFFFF;
}

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

uint32_t UID::getIdx() {
    return ((uint32_t*)(data))[3];
}

void UID::setIdx(uint32_t in) {
    ((uint32_t*)(data))[3] = in;
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

UIDHashTable::UIDHashTable(size_t initial_capacity) {
    buckets.reserve(initial_capacity);
    for (size_t i = 0; i < initial_capacity; i++) {
        buckets.emplace_back();
    }
}

UIDHashTable::~UIDHashTable() {
    clear();
}

void UIDHashTable::insert(const UID& key, uint32_t value) {
    if (size >= buckets.size() * 2) {
        reserve(buckets.size() * 2);
    }

    size_t bucket_idx = key.hash() % buckets.size();
    auto& bucket = buckets[bucket_idx];

    // Check if key already exists
    for (auto& entry : bucket) {
        if (entry.key.data[0] == key.data[0] && 
            entry.key.data[1] == key.data[1]) {
            entry.value = value;
            return;
        }
    }

    bucket.push_back({key, value});
    size++;
}

bool UIDHashTable::find(const UID& key, uint32_t& out_value) {
    size_t bucket_idx = key.hash() % buckets.size();
    auto& bucket = buckets[bucket_idx];

    for (auto& entry : bucket) {
        if (entry.key.data[0] == key.data[0] && 
            (entry.key.data[1] & 0x00000000FFFFFFFF) == (key.data[1] & 0x00000000FFFFFFFF)) {
            out_value = entry.value;
            return true;
        }
    }
    return false;
}

bool UIDHashTable::remove(const UID& key) {
    size_t bucket_idx = key.hash() % buckets.size();
    auto& bucket = buckets[bucket_idx];

    for (size_t i = 0; i < bucket.size(); i++) {
        if (bucket[i].key.data[0] == key.data[0] && 
            bucket[i].key.data[1] == key.data[1]) {
            // Swap with last element and pop
            if (i != bucket.size() - 1) {
                bucket[i] = bucket.back();
            }
            bucket.pop_back();
            size--;
            return true;
        }
    }
    return false;
}

void UIDHashTable::clear() {
    for (auto& bucket : buckets) {
        bucket.destroy();
    }
    buckets.destroy();
    size = 0;
}

void UIDHashTable::reserve(size_t new_capacity) {
    nonstd::vector<nonstd::vector<Entry>> new_buckets;
    new_buckets.reserve(new_capacity);
    for (size_t i = 0; i < new_capacity; i++) {
        new_buckets.emplace_back();
    }

    // Rehash all elements
    for (auto& bucket : buckets) {
        for (auto& entry : bucket) {
            size_t new_bucket_idx = entry.key.hash() % new_capacity;
            new_buckets[new_bucket_idx].push_back(entry);
        }
        bucket.destroy();
    }

    buckets.destroy();
    buckets = new_buckets;
}

#endif
