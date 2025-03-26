#ifndef ML_DECISION_TREE_H
#define ML_DECISION_TREE_H

#include <cstdint>
#include <array>
#include "cache.h"  // ✅ for champsim::cache_block
#include <iostream>

enum class Decision { KEEP, EVICT };

class PCSignatureReusePredictor {
public:
      static constexpr int SIG_BITS = 8;                         // 🧠 Use 8-bit signatures
    static constexpr int TABLE_SIZE = 1 << SIG_BITS;           // 256 entries
    std::array<uint8_t, TABLE_SIZE> counters{}; // 2-bit counters (0-3)
// Predict reuse: true if counter is confident (≥ 2)
    bool predict(uint16_t pc_sig) const {
	uint8_t sig = static_cast<uint8_t>(pc_sig & 0xFF);     // 🧼 Mask to 8 bits
        return counters[sig] >= 2;
    }

    void update(uint16_t pc_sig, bool reused) {
	           uint8_t sig = static_cast<uint8_t>(pc_sig & 0xFF);     // 🧼 Mask to 8 bits
        auto& ctr = counters[sig];
        if (reused) {
            if (ctr < 3) ++ctr;
        } else {
            if (ctr > 0) --ctr;
        }
    }
};


class EnhancedDecisionTreePolicy {
public:
    static constexpr int NUM_LEAVES = 6;
    uint8_t leaf_counters[NUM_LEAVES] = {2, 2, 2, 2, 2, 2}; // Start neutral
    PCSignatureReusePredictor* reuse_predictor;

    EnhancedDecisionTreePolicy() : reuse_predictor(nullptr) {}
    EnhancedDecisionTreePolicy(PCSignatureReusePredictor* predictor) : reuse_predictor(predictor) {}

    Decision classify(const champsim::cache_block& blk) {
    int leaf = get_leaf_index(blk);
    return (leaf_counters[leaf] >= 2) ? Decision::KEEP : Decision::EVICT;
}


    void update_on_eviction(const champsim::cache_block& blk, bool was_reused) {
        int leaf = get_leaf_index(blk);
        if (was_reused) {
            if (leaf_counters[leaf] > 0) --leaf_counters[leaf];
        } else {
            if (leaf_counters[leaf] < 3) ++leaf_counters[leaf];
        }
        if (reuse_predictor)
            reuse_predictor->update(blk.pc_signature, was_reused);
    }

private:
    int get_leaf_index(const champsim::cache_block& blk) const {
        bool reuse_pred = reuse_predictor && reuse_predictor->predict(blk.pc_signature);

        if (blk.hits_since_insertion == 0) {
            if (blk.prefetch) return 0;                // Cold prefetch
            if (blk.recency > 6) return 1;  // Cold + old
            return 2;                                   // Cold + recent
        } else {
            if (reuse_pred) return 3;                  // Reuse predicted
            if (blk.dirty) return 4;                   // Dirty + not reused
            return 5;                                   // Clean + not reused
        }
    }
};


#endif // ML_DECISION_TREE_H


