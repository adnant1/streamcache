#include "cache.h"
#include <string>
#include <unordered_map>

namespace streamcache {

    bool Cache::empty() const {
        return cache.empty();
    }

    void Cache::set(const std::string& key, const std::string& value) {
        cache[key] = value;
    }

    std::string Cache::get(const std::string& key) const {
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }
        return {};
    }

}