#include "shard.h"
#include "eviction_thread.h"
#include <iostream>
#include <iomanip>

namespace streamcache {
    Shard::Shard() : m_evictionThread(std::make_unique<EvictionThread>()) {
        m_evictionThread->start(*this);
    }

    Shard::~Shard() {
        if (m_evictionThread) {
            m_evictionThread->stop();
        }
    }

    void Shard::set(const std::string& key, CacheEntry entry) {
        auto now {std::chrono::steady_clock::now()};

        // Decide after unlocking whether to notify the eviction thread
        std::optional<Timestamp> notifyAt;

        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            
            /*
            * If the entry has no expiration, but the key already exists with an expiration,
            * preserve the existing expiration time.
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
            }

            m_logs[key].push_back({now, entry.value});
        }
        
        if (notifyAt) {
            notifyNewExpiry(*notifyAt);
        }
    }

    std::optional<std::string> Shard::get(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it {m_cache.find(key)};
        if (it != m_cache.end()) {
            const auto& entry {it->second};
            if (entry.expiration && *entry.expiration <= std::chrono::steady_clock::now()) {
                // Entry is expired, don't serve it (cleanup left to eviction thread)
                return std::nullopt;
            }
            return entry.value;
        }

        return std::nullopt;
    }

    void Shard::evictExpired(Timestamp now) {
        std::vector<std::string> expiredKeys {};
        
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
                    * Remove the entry from the shard if it matches the expiration time.
                    */
                    if (cacheEntry.expiration && cacheEntry.expiration.value() == expiry) {
                        expiredKeys.push_back(key);
                        m_cache.erase(it);
                    }
                }
                
                /*
                * If the key isn't found or the expiration time doesn't match, it's a stale entry → pop it.
                */
                m_evictionHeap.pop();
            }
        }

        /*
        * Erase the logs outside the lock to avoid holding it for potentially long operations.
        */
        for (auto& k: expiredKeys) {
            m_logs.erase(k);
        }

    }

    std::deque<LogEntry> Shard::getLogsForReplay(const std::string& key, Timestamp cutoff) const {
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

    void Shard::replay(const std::string& key) {
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

    void Shard::pruneAllLogs(Timestamp cutoff) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto startTime = std::chrono::steady_clock::now();
        const auto MAX_PRUNE_TIME = std::chrono::milliseconds(5);

        for (auto& [key, log] : m_logs) {
            while (!log.empty() && log.front().timestamp < cutoff) {
                log.pop_front();
            }

            if (std::chrono::steady_clock::now() - startTime > MAX_PRUNE_TIME) {
                break;
            }
        }
    }

    std::optional<Timestamp> Shard::peekNextExpiry() const {
        std::shared_lock lock(m_mutex);
        
        if (m_evictionHeap.empty()) {
            return std::nullopt;
        }

        return m_evictionHeap.top().first;
    }

    void Shard::notifyNewExpiry(Timestamp t) {
        bool shouldNotify {false};
        {
            std::shared_lock lock(m_mutex);
            const bool earlier {
                m_evictionHeap.empty() || t < m_evictionHeap.top().first
            };

            if (earlier) {
                shouldNotify = true;
            }
        }

        if (shouldNotify && m_notifyWakeup) {
            m_notifyWakeup();
        }
    }

}
