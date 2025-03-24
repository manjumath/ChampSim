#ifndef ML_LOGGER_H
#define ML_LOGGER_H

#include "cache.h"
#include "block.h"
#include "champsim.h"
#include <cstdint>

void log_ml_features(const CACHE* cache, const champsim::cache_block& blk, uint64_t cycle,
                     long set_idx, long way_idx, champsim::address addr,
                     access_type type, bool hit);

#endif

