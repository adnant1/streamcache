#include "cache_builder.h"

namespace util {

    std::optional<streamcache::CacheEntry> buildCacheEntry(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            return std::nullopt;
        }

        streamcache::CacheEntry entry {};
        entry.value = tokens[2];
        entry.timeSet = std::chrono::steady_clock::now();

        /*
        * Optional metadata handling can be added here
        * For now, we just set the expiration to the current time + ttl
        */
        if (tokens.size() >= 4) {
            try {
                int ttl {std::stoi(tokens[3])};
                if (ttl < 0) {
                    return std::nullopt;
                }

                entry.expiration = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
            } catch (const std::exception&) {
                return std::nullopt;
            }
        } else {
            entry.expiration = std::nullopt;
        }
        
        return entry;
    }
}