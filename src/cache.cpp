#include "cache.h"
#include <iostream>
#include <iomanip>


namespace streamcache {

    void Cache::set(const std::string& key, CacheEntry entry) {
        auto now {std::chrono::steady_clock::now()};
        
        /*
        * If the entry has no expiration, but the key already exists with an expiration,
        * we preserve the existing expiration time.
        */
        auto existingIt {m_cache.find(key)};
        if (!entry.expiration && existingIt != m_cache.end()
            && existingIt->second.expiration) {

                entry.expiration = existingIt->second.expiration;
        }
        
        entry.timeSet = now;
        m_cache[key] = entry;
        
        if (entry.expiration) {
            m_evictionQueue.push({entry.expiration.value(), key});
        }

        m_logs[key].push_back({now, entry.value});
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
        auto now {std::chrono::steady_clock::now()};
        
        while (!m_evictionQueue.empty() && m_evictionQueue.top().first < now) {
            std::pair<Timestamp, std::string> topEntry {m_evictionQueue.top()};
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
                    m_logs.erase(key);
                }
                
            }
            
            /*
            * If the key isn't found or the expiration time doesn't match, it's a stale entry and should be popped from the queue.
            */
            m_evictionQueue.pop();
        }
    }

    void Cache::pruneLog(const std::string& key, Timestamp cutoff) {
        auto& log {m_logs[key]};
        while (!log.empty() && log.front().timestamp < cutoff) {
            log.pop_front();
        }
    }

    void Cache::replay(const std::string& key) {
        auto it {m_cache.find(key)};
        if (it == m_cache.end()) {
            std::cout << "Key not found.\n";
            return;
        }

        /*
        * Use original TTL (expiration - timeSet) to create a fixed replay window.
        */
        const auto& entry {it->second};
        if (entry.expiration) {
            auto originalTTL {entry.expiration.value() - entry.timeSet};
            auto cutoff {std::chrono::steady_clock::now() - originalTTL};
            pruneLog(key, cutoff);
        }

        auto lit {m_logs.find(key)};
        if (lit == m_logs.end() || lit->second.empty()) {
            std::cout << "No recent history for key: " << key << "\n";
            return;
        }

        // Convert steady_clock timestamp to system_clock for display
        auto sysNow = std::chrono::system_clock::now();
        auto steadyNow = std::chrono::steady_clock::now();

        for (const auto& logEntry : lit->second) {
            
            // Explicitly cast the duration to system_clock duration
            auto duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(
                logEntry.timestamp - steadyNow
            );
            auto sysTime = sysNow + duration;

            std::time_t t = std::chrono::system_clock::to_time_t(sysTime);

            // Format time as YYYY-MM-DD HH:MM:SS
            std::cout << "[" << std::put_time(std::localtime(&t), "%F %T") << "] "
                    << logEntry.value << "\n";
        }
    }
}