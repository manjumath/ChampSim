#include <fstream>
#include <mutex>
#include "cache.h"
#include "ml_logger.h"
#include <iostream>

namespace {
std::ofstream ml_log_file("ml_cache_log.csv", std::ios::app);
std::mutex log_mutex;
bool header_written = false;
}

void log_ml_features(const CACHE* cache, const champsim::cache_block& blk, uint64_t cycle,
                     long set_idx, long way, champsim::address pc, access_type type, bool hit)
{
    (void)cache; // ✅ Suppress unused parameter warning
    std::lock_guard<std::mutex> lock(log_mutex);

    if (!header_written) {
        ml_log_file << "cycle,set,way,pc_sig,recency,hits,prefetch,dirty,reused,access_type,hit\n";
        header_written = true;
    }

    uint64_t pc_sig = pc.to<uint64_t>() & 0xFFF; // Example: 12-bit hash

    ml_log_file << cycle << ","
                << set_idx << ","
                << way << ","
                << pc_sig << ","
                << static_cast<int>(blk.recency) << ","
                << static_cast<int>(blk.hits_since_insertion) << ","
                << blk.prefetch << ","
                << blk.dirty << ","
                << blk.reused << ","
                << static_cast<int>(type) << ","
                << hit << "\n";
}

