#include "uid.cpp"
#include "terragen.h"
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <unordered_set> // For tracking inserted values

TEST_CASE("UID operations work correctly", "[uid]") {
    UID uid;
    
    SECTION("PID operations") {
        uid.setPID(0x123456789ABC);
        REQUIRE(uid.getPID() == 0x123456789ABC);
        
        uid.setPID(0);
        REQUIRE(uid.getPID() == 0);
    }
    
    SECTION("OID operations") {
        uid.setOID(0x12345678);
        REQUIRE(uid.getOID() == 0x12345678);
        
        uid.setOID(0xFFFF);
        REQUIRE(uid.getOID() == 0xFFFF);
    }
    
    SECTION("Index operations") {
        uid.setIdx(42);
        REQUIRE(uid.getIdx() == 42);
        
        uid.setIdx(0xFFFFFFFF);
        REQUIRE(uid.getIdx() == 0xFFFFFFFF);
    }
    
    SECTION("Combined operations") {
        uid.setPID(0x123456789ABC);
        uid.setOID(0xDEADBEEF);
        uid.setIdx(42);
        
        REQUIRE(uid.getPID() == 0x123456789ABC);
        REQUIRE(uid.getOID() == 0xDEADBEEF);
        REQUIRE(uid.getIdx() == 42);
    }
}

TEST_CASE("UIDHashTable operations", "[uid_hash]") {
    UIDHashTable table(4);
    
    SECTION("Basic insertion and lookup") {
        UID uid1, uid2;
        uid1.setPID(1); uid1.setOID(1);
        uid2.setPID(2); uid2.setOID(2);
        
        table.insert(uid1, 100);
        table.insert(uid2, 200);
        
        uint32_t val;
        REQUIRE(table.find(uid1, val));
        REQUIRE(val == 100);
        REQUIRE(table.find(uid2, val));
        REQUIRE(val == 200);
    }
    
    SECTION("Update existing value") {
        UID uid;
        uid.setPID(1); uid.setOID(1);
        
        table.insert(uid, 100);
        table.insert(uid, 200); // Update
        
        uint32_t val;
        REQUIRE(table.find(uid, val));
        REQUIRE(val == 200);
    }
    
    SECTION("Nonexistent lookup") {
        UID uid;
        uid.setPID(1); uid.setOID(1);
        
        uint32_t val;
        REQUIRE_FALSE(table.find(uid, val));
    }
    
    SECTION("Removal") {
        UID uid;
        uid.setPID(1); uid.setOID(1);
        
        table.insert(uid, 100);
        REQUIRE(table.remove(uid));
        uint32_t val;
        REQUIRE_FALSE(table.find(uid, val));
        REQUIRE_FALSE(table.remove(uid)); // Double remove
    }
    
    SECTION("Collision handling") {
        // Force collisions by using small table size
        UIDHashTable small_table(2);
        
        UID uid1, uid2, uid3;
        uid1.setPID(1); uid1.setOID(1);
        uid2.setPID(2); uid2.setOID(2);
        uid3.setPID(3); uid3.setOID(3);
        
        small_table.insert(uid1, 100);
        small_table.insert(uid2, 200);
        small_table.insert(uid3, 300);
        
        uint32_t val;
        REQUIRE(small_table.find(uid1, val));
        REQUIRE(val == 100);
        REQUIRE(small_table.find(uid2, val));
        REQUIRE(val == 200);
        REQUIRE(small_table.find(uid3, val));
        REQUIRE(val == 300);
    }
    
    SECTION("Clear and resize") {
        UID uid;
        uid.setPID(1); uid.setOID(1);
        
        table.insert(uid, 100);
        REQUIRE(table.size == 1);
        
        table.clear();
        REQUIRE(table.size == 0);
        
        // Test resize
        table.reserve(8);
        table.insert(uid, 100);
        REQUIRE(table.size == 1);
        
        uint32_t val;
        REQUIRE(table.find(uid, val));
        REQUIRE(val == 100);
    }

    SECTION("Large-scale pseudorandom operations") {
        const int iterations = 1000000;
        UIDHashTable large_table(1024);
        Prng_xoshiro prng;
        prng.init(0x12345678, 0x9ABCDEF0);

        // Insert random UIDs
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setOID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setIdx(i);
            large_table.insert(uid, i);
        }
        REQUIRE(large_table.size == iterations);

        // Reset PRNG to same seed to get same sequence
        prng.init(0x12345678, 0x9ABCDEF0);

        // Verify all inserted UIDs can be found
        uint32_t val;
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setOID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setIdx(i);
            REQUIRE(large_table.find(uid, val));
            REQUIRE(val == i);
        }

        // Reset PRNG again
        prng.init(0x12345678, 0x9ABCDEF0);

        // Remove all UIDs
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setOID(prng.get() & 0x0000FFFFFFFFFFFF);
            uid.setIdx(i);
            REQUIRE(large_table.remove(uid));
        }
        REQUIRE(large_table.size == 0);
    }

    SECTION("Large-scale sequential operations") {
        const int iterations = 1000000;
        UIDHashTable seq_table(1024);

        // Insert sequential UIDs
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(i);
            uid.setOID(i * 2);
            uid.setIdx(i % 1000);
            seq_table.insert(uid, i);
        }
        REQUIRE(seq_table.size == iterations);

        // Verify all inserted UIDs can be found
        uint32_t val;
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(i);
            uid.setOID(i * 2);
            uid.setIdx(i % 1000);
            REQUIRE(seq_table.find(uid, val));
            REQUIRE(val == i);
        }

        // Remove all UIDs
        for (int i = 0; i < iterations; i++) {
            UID uid;
            uid.setPID(i);
            uid.setOID(i * 2);
            uid.setIdx(i % 1000);
            REQUIRE(seq_table.remove(uid));
        }
        REQUIRE(seq_table.size == 0);
    }
}

