#include "cache.h"
#include <iostream>
#include <iomanip>


namespace streamcache {

    void Cache::set(const std::string& key, CacheEntry entry) {
        auto now {std::chrono::steady_clock::now()};

        // Decide after unlocking whether to notify the eviction thread
        std::optional<Timestamp> notifyAt;

        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            
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
                const Timestamp t = *entry.expiration;
                m_evictionHeap.push({t, key});
                notifyAt = t;
                m_heapSize = m_evictionHeap.size();
            }
    
            m_logs[key].push_back({now, entry.value});
        }
        
        if (notifyAt) {
            notifyNewExpiry(*notifyAt);
        }
    }

    std::optional<std::string> Cache::get(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it {m_cache.find(key)};
        if (it != m_cache.end()) {
            const auto& entry {it->second};
            if (entry.expiration && *entry.expiration <= std::chrono::steady_clock::now()) {
                // Entry is expired, don't serve it or modify the cache
                return std::nullopt;
            }
            return entry.value;
        }

        return std::nullopt;
    }

    size_t Cache::evictExpired(Timestamp now) {
        std::vector<std::string> expiredKeys {};
        size_t evictedCount {0};
        
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);

            while (!m_evictionHeap.empty() && m_evictionHeap.top().first <= now) {
                std::pair<Timestamp, std::string> topEntry {m_evictionHeap.top()};
                auto expiry {topEntry.first};
                auto key {topEntry.second};
                
                auto it {m_cache.find(key)};
                
                if (it != m_cache.end()) {
                    const auto& cacheEntry {it->second};
                    
                    /*
                    * Remove the entry from the cache if it matches the expiration time.
                    */
                    if (cacheEntry.expiration && cacheEntry.expiration.value() == expiry) {
                        expiredKeys.push_back(key);
                        m_cache.erase(it);
                        ++evictedCount;
                    }
                    
                }
                
                /*
                * If the key isn't found or the expiration time doesn't match, it's a stale entry and should be popped from the queue.
                */
                m_evictionHeap.pop();
            }
    
            if (evictedCount > 0) {
                ++m_evictionBatches;
            }
            m_evictionsTotal += evictedCount;
            m_heapSize = m_evictionHeap.size();
        }

        /*
        * Erase the logs outside the lock to avoid holding it for potentially long operations.
        */
       for (auto& k: expiredKeys) {
            m_logs.erase(k);
       }

        return evictedCount;
    }


    std::deque<LogEntry> Cache::getLogsForReplay(const std::string& key, Timestamp cutoff) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto lit {m_logs.find(key)};
        if (lit == m_logs.end()) {
            return {};
        }

        std::deque<LogEntry> replayLog {};
        for (const auto& logEntry: lit->second) {
            if (logEntry.timestamp >= cutoff) {
                replayLog.push_back(logEntry);
            }
        }

        return replayLog;
    }

    void Cache::replay(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        
        auto it {m_cache.find(key)};
        if (it == m_cache.end()) {
            std::cout << "Key not found.\n";
            return;
        }

        /*
        * Use original TTL (expiration - timeSet) to create a fixed replay window.
        */
        const auto& entry {it->second};
        std::deque<LogEntry> replayLog;
        
        if (entry.expiration) {
            auto originalTTL {entry.expiration.value() - entry.timeSet};
            auto cutoff {std::chrono::steady_clock::now() - originalTTL};
            replayLog = getLogsForReplay(key, cutoff);
        } else {
            // No expiration, show all logs
            replayLog = getLogsForReplay(key, Timestamp{});
        }

        if (replayLog.empty()) {
            std::cout << "No recent history for key: " << key << "\n";
            return;
        }

        // Convert steady_clock timestamp to system_clock for display
        auto sysNow = std::chrono::system_clock::now();
        auto steadyNow = std::chrono::steady_clock::now();

        for (const auto& logEntry : replayLog) {
            
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

    void Cache::pruneAllLogs(Timestamp cutoff) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        for (auto& [key, log] : m_logs) {
            while (!log.empty() && log.front().timestamp < cutoff) {
                log.pop_front();
            }
        }
    }

    std::optional<Timestamp> Cache::peekNextExpiry() const {
        std::shared_lock lock(m_mutex);
        
        if (m_evictionHeap.empty()) {
            return std::nullopt;
        }

        return m_evictionHeap.top().first;
    }

    void Cache::notifyNewExpiry(Timestamp t) {
        bool shouldNotify {false};
        {
            std::shared_lock lock(m_mutex);
            const bool earlier {
                m_evictionHeap.empty() || t < m_evictionHeap.top().first
            };

            if (earlier) {
                ++m_notifyEarlierExpiryCount;
                shouldNotify = true;
            }
        }

        if (shouldNotify && m_notifyWakeup) {
            m_notifyWakeup();
        }
    }
}