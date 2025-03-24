#ifndef ML_DECISION_TREE_H
#define ML_DECISION_TREE_H

#include <cstdint>
#include "cache.h"  // ✅ for champsim::cache_block

#include <iostream>

enum class Decision { KEEP, EVICT };

class DecisionTreePolicy {
public:
    static constexpr int NUM_LEAVES = 5;
    uint8_t leaf_counters[NUM_LEAVES] = {2, 2, 2, 2, 2}; // 2-bit counters

    Decision classify(const champsim::cache_block& blk) {
        int leaf_idx = get_leaf_index(blk);
	std::cerr << "[ML] Leaf " << leaf_idx
          << ", Counter = " << (int)leaf_counters[leaf_idx]
          << ", Decision = " << ((leaf_counters[leaf_idx] >= 2) ? "KEEP" : "EVICT") << "\n";
        return (leaf_counters[leaf_idx] >= 2) ? Decision::KEEP : Decision::EVICT;
    }

    void update_on_eviction(const champsim::cache_block& blk, bool was_reused) {
        int leaf_idx = get_leaf_index(blk);
        if (was_reused) {
            if (leaf_counters[leaf_idx] > 0) leaf_counters[leaf_idx]--;
        } else {
            if (leaf_counters[leaf_idx] < 3) leaf_counters[leaf_idx]++;
        }
    std::cerr << "[UPDATE] Leaf " << leaf_idx
              << ", Reused? " << was_reused
              << ", Counter = " << static_cast<int>(leaf_counters[leaf_idx]) << "\n";

    }

private:
    int get_leaf_index(const champsim::cache_block& blk) const {
        if (blk.hits_since_insertion == 0) {
            if (blk.prefetch) return 0;
            if (blk.recency > 6) return 1;
            return 2;
        } else {
            if (blk.reused) return 3;
            return 4;
        }
    }
};

#endif // ML_DECISION_TREE_H


