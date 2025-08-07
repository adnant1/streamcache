#pragma once
#include <string>
#include <unordered_map>
#include <queue>    
#include <vector>
#include <utility>
#include <optional>
#include <chrono>

namespace streamcache {
    using TimePoint = std::chrono::system_clock::time_point;

    /*
    * Entry structure containing a value and relevent metadata.
    */
    struct CacheEntry {
        std::string value;
        std::optional<TimePoint> expiration;
    };

    /*
    * Custom comparator for the eviction queue. Makes sure that keys with earlier
    * expiration times are prioritized for removal.
    */
   struct EvictionComparator {
        bool operator() (const std::pair<TimePoint, std::string>& a,
                        const std::pair<TimePoint, std::string>& b) const {
            return a.first > b.first; // earlier expiration  => higher priority
        }
    };

    class Cache {
        public:
            /*
            * Checks if the cache is empty.
            *
            * @return True if the cache is empty, false otherwise.
            */
           bool empty() const;

            /*
            * Adds or updates an entry in the cache.
            * If the entry has an expiration time, it is added to the eviction queue.
            *
            * @param key The key for the cache entry.
            * @param entry The value + metadata to be stored in the cache.
            */
           void set(const std::string& key, const CacheEntry& entry);

            /*
            * Retrieves a value from the cache. If the key has expired, it is removed from the cache.
            *
            * @param key The key for the cache entry.
            * @return The value associated with the key, or NULL if not found.
            */
           std::optional<std::string> get(const std::string& key);

            /*
            * Removes expired entries from the cache based on the eviction queue.
            */
           void evictExpired();

        private:
            std::unordered_map<std::string, CacheEntry> m_cache;
            std::priority_queue<
                std::pair<TimePoint, std::string>,
                std::vector<std::pair<TimePoint, std::string>>,
                EvictionComparator
            > m_evictionQueue;
       
    };
}