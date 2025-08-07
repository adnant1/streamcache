#include "cache.h"
#include <string>
#include <unordered_map>

namespace streamcache {

    bool Cache::empty() const {
        return m_cache.empty();
    }

    void Cache::set(const std::string& key, const CacheEntry& entry) {
        m_cache[key] = entry;

        if (entry.expiration) {
            m_evictionQueue.push({entry.expiration.value(), key});
        }
    }

    std::optional<std::string> Cache::get(const std::string& key) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            const auto& entry = it->second;
            auto now = std::chrono::system_clock::now();

            if(entry.expiration && entry.expiration.value() < now) {
                m_cache.erase(it);
                return std::nullopt;
            }

            return entry.value;
        }

        return std::nullopt;
    }
}