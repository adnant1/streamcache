#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>

namespace streamcache {

    /*
    * Entry structure containing a value and relevent metadata.
    */
    struct CacheEntry {
        std::string value;
        std::optional<std::chrono::system_clock::time_point> expiration;
    };

    class Cache {
        private:
            std::unordered_map<std::string, CacheEntry> cache;
       
        public:
            /*
            * Checks if the cache is empty.
            *
            * @return True if the cache is empty, false otherwise.
            */
           bool empty() const;

            /*
            * Adds or updates an entry in the cache.
            *
            * @param key The key for the cache entry.
            * @param entry The value + metadata to be stored in the cache.
            */
           void set(const std::string& key, const CacheEntry& entry);

            /*
            * Retrieves a value from the cache.
            *
            * @param key The key for the cache entry.
            * @return The value associated with the key, or an empty string if not found.
            */
           std::string get(const std::string& key) const;
    };
}