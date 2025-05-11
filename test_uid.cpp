#include "uid.cpp"
#include "terragen.h"
#include <iomanip>

int main() {        
    uint64_t errors = 0;

    UIDHashTable large_table;
    large_table.init(1024);
    for(uint64_t epoch = 1; epoch < 11; epoch++) {
        //const uint64_t seed_a = 1 ^ epoch;
        //const uint64_t seed_b = 2 ^ epoch;
        const uint64_t seed_a = 0x123456789ABCDEF0 ^ epoch;
        const uint64_t seed_b = 0xFEDCBA9876543210 ^ epoch;
//        const int num_operations = 800000000; // needs a lot of RAM
//        const int num_operations = 80000000; // should mostly stay in RAM
        const int num_operations = 1000000 * epoch; // mix of cache and RAM
//        const int num_operations = 800000; // should fit comfortably in 3D V-cache
        const int check_interval = num_operations / 10;
        
        Prng_xoshiro rng;
        //Prng_sha256 rng;
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
                if(large_table[uid] != (i % 0x100000000))
                    errors++;
            }
        }

        auto distances = large_table.count_probe_distances();
        std::cout << "Probing distribution with load factor " << 1.0 * num_operations / large_table.slots.capacity << ":\n";
        for (int i = 0; i < distances.size(); i++) {
            std::cout << distances[i] << "  ";
        }
        std::cout << "\n";
        distances.destroy();
    
        // Reset RNG to regenerate same sequence
        rng.init(seed_a, seed_b);
        
        // Delete phase
        for (uint32_t i = 0; i < num_operations; i++) {
            UID uid;
            uint64_t pid = rng.get() % 0x0001000000000000;
            uint64_t oid = rng.get() % 0x0001000000000000;
            if (epoch % 2 == 0) {
                pid = epoch;
                oid = i;
            }
            uid.setPID(pid);
            uid.setOID(oid);
            if (large_table[uid] != (i % 0x100000000))
                errors++;
            large_table.remove(uid);
        }
        
        if(large_table.size != 0) {
            std::cerr << "Large table should be empty but size is " << large_table.size << std::endl;
            errors++;
        }
        if(errors) {
            std::cerr << "Errors found: " << std::dec << errors << std::endl;
            return 1;
        }
    }
    large_table.destroy();
    return 0;
}

