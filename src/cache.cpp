#include "cache.h"
#include <string>
#include <unordered_map>

namespace streamcache {

    bool Cache::empty() const {
        return cache.empty();
    }

    void Cache::set(const std::string& key, const CacheEntry& entry) {
        cache[key] = entry;
    }

    std::string Cache::get(const std::string& key) const {
        if (cache.find(key) != cache.end()) {
            const auto& entry = cache.at(key);
            if (!entry.expiration || entry.expiration.value() > std::chrono::system_clock::now()) {
                return entry.value;
            }
        }

        return {};
    }
}