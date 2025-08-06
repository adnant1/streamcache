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

    std::optional<std::string> Cache::get(const std::string& key) {
        if (cache.find(key) != cache.end()) {
            const auto& entry = cache.at(key);
            auto now = std::chrono::system_clock::now();
            if (entry.expiration && entry.expiration.value() < now) {
                cache.erase(key);
                return std::nullopt;
            } else {
                return entry.value;
            }
        }

        return std::nullopt;
    }
}