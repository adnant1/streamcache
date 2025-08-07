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
        auto it {m_cache.find(key)};
        if (it != m_cache.end()) {
            const auto& entry {it->second};
            return entry.value;
        }

        return std::nullopt;
    }

    void Cache::evictExpired() {
        auto now {std::chrono::system_clock::now()};
        
        while (!m_evictionQueue.empty() && m_evictionQueue.top().first < now) {
            std::pair<TimePoint, std::string> topEntry {m_evictionQueue.top()};
            auto expirationTime {topEntry.first};
            auto key {topEntry.second};
            
            auto it {m_cache.find(key)};
            
            if (it != m_cache.end()) {
                const auto& cacheEntry {it->second};
                
                /*
                * Remove the entry from the cache if it matches the expiration time.
                */
                if (cacheEntry.expiration && cacheEntry.expiration.value() == expirationTime) {
                    m_cache.erase(it);
                }
                
            }
            
            /*
            * If the key isn't found or the expiration time doesn't match, it's a stale entry and should be popped from the queue.
            */
            m_evictionQueue.pop();
        }
    }
}