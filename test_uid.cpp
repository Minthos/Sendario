#include "uid.cpp"
#include "terragen.h"
#include <stdexcept>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <iomanip>

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
    UIDHashTable table;
    table.init(4);
    
    SECTION("Basic insertion and lookup") {
        UID uid1, uid2;
        uid1.setPID(1); uid1.setOID(1); uid1.setIdx(100);
        uid2.setPID(2); uid2.setOID(2); uid2.setIdx(200);
        
        REQUIRE(table.size == 0);
        REQUIRE(table.slots.capacity >= 4);
        table.insert(uid1);
        REQUIRE(table.size == 1);
        REQUIRE(table.slots.capacity >= 4);
        table.insert(uid2);
        REQUIRE(table.size == 2);
        REQUIRE(table.slots.capacity >= 4);
        
        REQUIRE(table[uid1] == 100);
        REQUIRE(table[uid2] == 200);
        
        // Test lookup of non-existent key
        UID uid3;
        uid3.setPID(3); uid3.setOID(3);
        uint32_t idx;
        REQUIRE_THROWS(idx = table[uid3]);
        REQUIRE(table.size == 2);
        REQUIRE_THROWS(table[uid3] == idx);
    }
    
    SECTION("Update existing value") {
        UID uid;
        uid.setPID(1); uid.setOID(1); uid.setIdx(100);
        
        table.insert(uid);
        REQUIRE(table[uid] == 100);
        
        // Update with new value
        uid.setIdx(200);
        table.insert(uid);
        REQUIRE(table[uid] == 200);
    }
    
    SECTION("Removal") {
        UID uid;
        uid.setPID(1); uid.setOID(1); uid.setIdx(100);
        
        table.insert(uid);
        REQUIRE(table.size == 1);
        table.remove(uid);
        REQUIRE(table.size == 0);
        
        // Test removing non-existent key
        table.remove(uid); // Should not crash
    }
    
    SECTION("Collision handling") {
        UIDHashTable small_table;
        small_table.init(2);
        
        UID uid1, uid2, uid3;
        uid1.setPID(1); uid1.setOID(1); uid1.setIdx(100);
        uid2.setPID(2); uid2.setOID(2); uid2.setIdx(200);
        uid3.setPID(3); uid3.setOID(3); uid3.setIdx(300);
        
        small_table.insert(uid1);
        small_table.insert(uid2);
        small_table.insert(uid3);
        
        REQUIRE(small_table[uid1] == 100);
        REQUIRE(small_table[uid2] == 200);
        REQUIRE(small_table[uid3] == 300);
    }
    
    SECTION("Clear and resize") {
        UID uid;
        uid.setPID(1); uid.setOID(1); uid.setIdx(100);
        
        table.insert(uid);
        REQUIRE(table.size == 1);
        
        table.clear();
        REQUIRE(table.size == 0);
        
        // Test resize
        table.reserve(8);
        table.insert(uid);
        REQUIRE(table.size == 1);
        REQUIRE(table[uid] == 100);
    }

    SECTION("Hash function") {
        UID uid1, uid2;
        uid1.setPID(1); uid1.setOID(1);
        uid2.setPID(2); uid2.setOID(2);
        
        REQUIRE(uid1.hash() != uid2.hash());
        
        // Test same UID has same hash
        UID uid3;
        uid3.setPID(1); uid3.setOID(1);
        REQUIRE(uid1.hash() == uid3.hash());
        
        // Test that index doesn't affect hash
        uid3.setIdx(42);
        REQUIRE(uid1.hash() == uid3.hash());
    }

    SECTION("Edge cases") {
        UID uid;
        // Test max PID
        uid.setPID(0x0000FFFFFFFFFFFF);
        REQUIRE(uid.getPID() == 0x0000FFFFFFFFFFFF);
        
        // Test max OID 
        uid.setOID(0x0000FFFFFFFFFFFF);
        REQUIRE(uid.getOID() == 0x0000FFFFFFFFFFFF);
        
        // Test max index
        uid.setIdx(0xFFFFFFFF);
        REQUIRE(uid.getIdx() == 0xFFFFFFFF);
        
        // Test zero UID operations
        UID zero = {0, 0};
        REQUIRE(zero.getPID() == 0);
        REQUIRE(zero.getOID() == 0);
        REQUIRE(zero.getIdx() == 0);
    }

    SECTION("Table destruction") {
        UIDHashTable local_table;
        local_table.init(4);
        
        UID uid;
        uid.setPID(1); uid.setOID(1); uid.setIdx(100);
        local_table.insert(uid);
        
        local_table.destroy();
        // Should be able to re-init after destroy
local_table.init(4);
        local_table.insert(uid);
        REQUIRE(local_table[uid] == 100);
    }

    SECTION("Large-scale operations with pseudorandom UIDs") {
        
        UIDHashTable large_table;
        large_table.init(1024);
        for(uint64_t epoch = 0; epoch < 10; epoch++) {
            //const uint64_t seed_a = 1 ^ epoch;
            //const uint64_t seed_b = 2 ^ epoch;
            const uint64_t seed_a = 0x123456789ABCDEF0 ^ epoch;
            const uint64_t seed_b = 0xFEDCBA9876543210 ^ epoch;
    //        const int num_operations = 800000000;
            const int num_operations = 8000000;
            const int check_interval = num_operations / 10;
            
            Prng_xoshiro rng;
            rng.init(seed_a, seed_b);

            // Insert phase
            for (int i = 0; i < num_operations; i++) {
                UID uid;
                uint64_t pid = rng.get() % 0x0001000000000000;
                uint64_t oid = rng.get() % 0x0001000000000000;
                if(epoch % 2 == 0){
                    pid = epoch;
                    oid = i;
                }
                //std::cerr << "Raw PID: " << std::hex << pid << " OID: " << oid << "\n";
                uid.setPID(pid);
                uid.setOID(oid);
                uid.setIdx(i % 0x100000000);
                //std::cerr << "Stored UID: " << std::hex << uid.data[0] << " " << uid.data[1] << "\n";
                if(uid.data[0] == 0 && uid.data32[2] == 0) {
                    std::cerr << "nullified!\n";
                }

                large_table.insert(uid);
                
                if (i % check_interval == 0) {
                    // Verify the inserted UID is present
                    REQUIRE(large_table[uid] == (i % 0x100000000));
                }
            }

            if(epoch == 4){
                auto distances = large_table.count_probe_distances();
                std::cout << "Probing distribution with load factor " << 1.0 * num_operations / large_table.slots.capacity << ":\n";
                for (int i = 0; i < distances.size(); i++) {
                    std::cout << distances[i] << "  ";
                }
                std::cout << "\n";
                distances.destroy();
            }
            
            // Reset RNG to regenerate same sequence
            rng.init(seed_a, seed_b);
            
            // Delete phase
            for (int i = 0; i < num_operations; i++) {
                UID uid;
                uint64_t pid = rng.get() % 0x0001000000000000;
                uint64_t oid = rng.get() % 0x0001000000000000;
                if(epoch % 2 == 0){
                    pid = epoch;
                    oid = i;
                }
                uid.setPID(pid);
                uid.setOID(oid);
                CHECK(large_table[uid] == (i % 0x100000000)); // should be REQUIRE
                large_table.remove(uid);
                REQUIRE_THROWS(large_table[uid] == (i % 0x100000000));
            }
            
            CHECK(large_table.size == 0); // it's supposed to be 0
        }
        large_table.destroy();
    }
}

