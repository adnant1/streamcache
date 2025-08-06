#pragma once
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include "cache.h"

namespace cache_builder {

    /*
     * Builds a cache entry from the given vector of tokens.
     *
     * @param tokens The vector of strings representing the command tokens.
     * @return An optional CacheEntry if the tokens are valid, otherwise std::nullopt
     */
    std::optional<streamcache::CacheEntry> buildCacheEntry(const std::vector<std::string>& tokens);
}