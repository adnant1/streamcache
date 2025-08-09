#pragma once
#include <string>
#include <unordered_map>
#include <queue>    
#include <vector>
#include <utility>
#include <optional>
#include <chrono>

namespace streamcache {
    using Timestamp = std::chrono::steady_clock::time_point;

    /*
    * Entry structure containing a value and relevent metadata.
    */
    struct CacheEntry {
        std::string value {};
        std::optional<Timestamp> expiration {};
        Timestamp timeSet {};
    };

    /*
    * Log structure containing the value and its timestamp.
    */
   struct LogEntry {
        Timestamp timestamp {};
        std::string value {};
   };

    /*
    * Custom comparator for the eviction queue. Makes sure that keys with earlier
    * expiration times are prioritized for removal.
    */
   struct EvictionComparator {
        bool operator() (const std::pair<Timestamp, std::string>& a,
                        const std::pair<Timestamp, std::string>& b) const {
            return a.first > b.first; // earlier expiration  => higher priority
        }
    };

    class Cache {
        public:
            /**
            * Adds or updates an entry in the cache.
            * If the entry has an expiration time, it is added to the eviction queue.
            * Appends the value to the key's log.
            *
            * @param key The key for the cache entry.
            * @param entry The value + metadata to be stored in the cache.
            */
           void set(const std::string& key, const CacheEntry& entry);

            /**
            * Retrieves a value from the cache.
            *
            * @param key The key for the cache entry.
            * @return The value associated with the key, or NULL if not found.
            */
           std::optional<std::string> get(const std::string& key);

            /**
            * Removes expired entries from the cache based on the eviction queue.
            */
           void evictExpired();

           /**
            * Makes sure that the key's log only contains entries still inside the key's
            * original TTL window.
            * 
            * @param key The key for which the log should be pruned.
            * @param cutoff The timestamp before which all log entries should be removed.
            */
           void pruneLog(const std::string& key, Timestamp cutoff);

           /**
            * Displays a key's recent values within its TTL window.
            * 
            * @param key The key for which the log should be displayed.
            */
           void replay(const std::string& key);

        private:
            std::unordered_map<std::string, CacheEntry> m_cache {};
            std::priority_queue<
                std::pair<Timestamp, std::string>,
                std::vector<std::pair<Timestamp, std::string>>,
                EvictionComparator
            > m_evictionQueue {};
            std::unordered_map<std::string, std::deque<LogEntry>> m_logs {};
       
    };
}