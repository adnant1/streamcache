#pragma once
#include <string>
#include <unordered_map>

namespace streamcache {

    class Cache {
        private:
            std::unordered_map<std::string, std::string> cache;
        
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
            * @param value The value to be stored in the cache.
            */
           void set(const std::string& key, const std::string& value);

            /*
            * Retrieves a value from the cache.
            *
            * @param key The key for the cache entry.
            * @return The value associated with the key, or an empty string if not found.
            */
           std::string get(const std::string& key) const;
    };
}