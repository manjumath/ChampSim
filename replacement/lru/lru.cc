#include "lru.h"

#include <inttypes.h>
#include <algorithm>
#include <cassert>

//lru::lru(CACHE* cache) : lru(cache, cache->NUM_SET, cache->NUM_WAY) {}

//lru::lru(CACHE* cache, long sets, long ways) : replacement(cache), NUM_WAY(ways), last_used_cycles(static_cast<std::size_t>(sets * ways), 0) {}

lru::lru(CACHE* cache_ptr)
    : replacement(cache_ptr),
      NUM_WAY(cache_ptr->NUM_WAY), 
      last_used_cycles(static_cast<std::size_t>(cache_ptr->NUM_SET * cache_ptr->NUM_WAY), 0),
      cache(cache_ptr) {}


#include <iostream>  // ✅ Debugging

long lru::get_actual_num_ways(long set) const {
    return cache->get_actual_num_ways(set);  // Fetch from cache instance
}


long lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
long num_ways = get_actual_num_ways(set);
    long selected_way = -1;
    uint64_t oldest_timestamp = UINT64_MAX;

    //std::cerr << "🛠️ DEBUG: find_victim() -> Set = " << set
    //      << ", Max Way Index = " << num_ways - 1 << "\n";

    // ✅ Check for an invalid block first
    for (long way = 0; way < num_ways; ++way) {
        if (!current_set[way].valid) {
            selected_way = way;
            break;
        }
    }

    // ✅ If no invalid block, find the LRU block
    if (selected_way == -1) {
        for (long way = 0; way < num_ways; ++way) {
            if (current_set[way].prefetch) continue;

            if (current_set[way].dirty < oldest_timestamp) {
                oldest_timestamp = current_set[way].dirty;
                selected_way = way;
            }
        }
    }

    // 🚨 Ensure the selected way is valid
    if (selected_way < 0 || selected_way >= num_ways) {
        std::cerr << "🚨 ERROR: find_victim() selected invalid way = " << selected_way
                  << " for Set = " << set
                  << ", Valid range = [0, " << num_ways - 1 << "]\n";
        selected_way = num_ways - 1;  // Clamp to the last valid way
    }

    //std::cerr << "✅ find_victim: Selected way = " << selected_way
    //          << " for Set = " << set
    //          << ", Max Way Index = " << num_ways - 1 << "\n";

    return selected_way;
}


/* long lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
    const int NUM_WAYS = 16; // ⚠️ Ensure this matches your cache associativity

    long selected_way = -1;  // Track selected eviction way
    uint64_t oldest_timestamp = UINT64_MAX;

    // ✅ First, try to find an invalid block
    for (long way = 0; way < NUM_WAYS; ++way) {
        if (!current_set[way].valid) {
            selected_way = way;
            break;
        }
    }

    // ✅ If no invalid block, select Least Recently Used (LRU)
    if (selected_way == -1) {
        for (long way = 0; way < NUM_WAYS; ++way) {
            if (current_set[way].prefetch) continue;  // Prefer keeping prefetched blocks

            if (current_set[way].dirty < oldest_timestamp) {
                oldest_timestamp = current_set[way].dirty;
                selected_way = way;
            }
        }
    }

    // 🚨 Ensure we NEVER return an invalid way
    if (selected_way < 0 || selected_way >= NUM_WAYS) {
        std::cerr << "🚨 ERROR: find_victim() selected invalid way = " << selected_way << " (Valid range: 0-" << (NUM_WAYS - 1) << ")\n";
        //selected_way = 0;  // Default to safe value
	selected_way = NUM_WAYS - 1;  // Default to the last valid index
    }

        std::cerr << "✅ find_victim: Selected way = " << selected_way
              << " for Set = " << set << "\n";
    return selected_way;
}


long lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
    const int NUM_WAYS = 16; // ⚠️ Set this to the correct associativity of the LLC

    // Manually define begin and end pointers
    const champsim::cache_block* begin = current_set;
    const champsim::cache_block* end = current_set + NUM_WAYS;  // Move pointer to end

    // Pick the first invalid block, or the least prefetched one as a fallback
    auto victim = std::find_if(begin, end, [](const champsim::cache_block& block) {
        return !block.valid;  // Prefer evicting invalid blocks first
    });

    if (victim == end) {
        // If all blocks are valid, evict the first prefetched block as a heuristic
        victim = std::min_element(begin, end, [](const champsim::cache_block& a, const champsim::cache_block& b) {
            return a.prefetch && !b.prefetch;  // Prefer evicting prefetched blocks first
        });
    }

    assert(begin <= victim);
    assert(victim < end);

    const champsim::cache_block& victim_block = *victim;  // ✅ Ensure it's a `cache_block`

    // Extract the correct eviction data
        uint64_t victim_address = victim_block.address.to<uint64_t>();
    bool is_dirty = victim_block.dirty;
    bool was_prefetched = victim_block.prefetch;
    uint32_t prefetch_metadata = victim_block.pf_metadata;

    // Log eviction data for ML training
    FILE *logfile = fopen("cache_evictions.csv", "a");
    if (logfile) {
	    fprintf(logfile, "%" PRIx64 ", %d, %d, %d\n",
        victim_address, was_prefetched, is_dirty, prefetch_metadata);

        fclose(logfile);
    }

    return std::distance(begin, victim);
}


long lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                      champsim::address full_addr, access_type type)
{
  auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  // Find the way whose last use cycle is most distant
  //auto victim = std::min_element(begin, end);
  auto victim = std::min_element(begin, end, [](const champsim::cache_block& a, const champsim::cache_block& b) {
    return a.lru < b.lru;  // Compare LRU values
});
  assert(begin <= victim);
  assert(victim < end);
      // Extract useful eviction features
    const champsim::cache_block& victim_block = *victim;
    uint64_t victim_address = victim_block.address;
uint64_t victim_pc = victim_block.ip;
bool is_dirty = victim_block.dirty;
bool was_prefetched = victim_block.prefetched;
int recency = std::distance(begin, victim);
int frequency = victim_block.access_count;

    // Log eviction data for ML training
    FILE *logfile = fopen("cache_evictions.csv", "a");
    if (logfile) {
        fprintf(logfile, "%llx, %llx, %d, %d, %d, %d\n",
                victim_pc, victim_address, recency, frequency, was_prefetched, is_dirty);
        fclose(logfile);
    }
  return std::distance(begin, victim);
}
*/
void lru::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                 access_type type)
{
// Log information about the replacement event
    FILE *logfile = fopen("cache_evictions.csv", "a");
    if (logfile) {
	    fprintf(logfile, "Eviction Event: Set %ld, Way %ld, Victim Address: %" PRIx64 ", New Address: %" PRIx64 "\n",
        set, way, victim_addr.to<uint64_t>(), full_addr.to<uint64_t>());

        fclose(logfile);
    }
  // Mark the way as being used on the current cycle
  last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
}

void lru::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type, uint8_t hit)
{
  // Mark the way as being used on the current cycle
  if (hit && access_type{type} != access_type::WRITE) // Skip this for writeback hits
    last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
}
