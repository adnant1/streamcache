#pragma once
#include <string>
#include <unordered_map>
#include <queue>
#include <deque>
#include <vector>
#include <optional>
#include <chrono>
#include <shared_mutex>
#include <atomic>
#include <memory>

namespace streamcache {
    using Timestamp = std::chrono::steady_clock::time_point;

    // Forward declaration to avoid circular dependency
    class EvictionThread;

    /*
    * Entry structure containing a value and relevant metadata.
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
        bool operator()(const std::pair<Timestamp, std::string>& a,
                        const std::pair<Timestamp, std::string>& b) const {
            return a.first > b.first; // earlier expiration => higher priority
        }
    };

    /*
    * A Shard is a self-contained mini-cache with its own index, logs,
    * eviction queue, and synchronization primitives.
    */
    class Shard {
    public:
        Shard();
        ~Shard();

        /*
        * Non-copyable, moveable only.
        */
        Shard(const Shard&) = delete;
        Shard& operator=(const Shard&) = delete;
        Shard(Shard&&) noexcept = delete;
        Shard& operator=(Shard&&) noexcept = delete;

        /**
        * Adds or updates an entry in the shard.
        * If the entry has an expiration time, it is added to the eviction heap.
        * Appends the value to the key's log and prunes old log entries.
        *
        * @param key The key for the shard entry.
        * @param entry The value + metadata to be stored in the shard.
        *              Passed by value so it can be safely modified without affecting
        *              the caller's original object.
        */
        void set(const std::string& key, CacheEntry entry);

        /**
        * Retrieves a value from the cache.
        *
        * @param key The key for the cache entry.
        * @return The value associated with the key, or NULL if not found.
        */
        std::optional<std::string> get(const std::string& key);

        /**
        * Displays a key's recent values within its TTL window.
        * 
        * @param key The key for which the log should be displayed.
        */
        void replay(const std::string& key);

        /**
        * Prunes log entries for all keys that are older than the cutoff timestamp.
        * This cutoff is calculated by (now - log retention duration).
        * The log retention duration is a fixed value of 1 hour. This means that
        * logs older than 1 hour will be removed, regardless of the individual key's TTL.
        * 
        * @param cutoff The timestamp before which all log entries should be removed.
        */
        void pruneAllLogs(Timestamp cutoff);

        /**
        * Called by the eviction thread to check when the next eviction should occur.
        * Requires a shared lock to safely read the eviction heap without blocking
        * other readers.
        * 
        * @return The timestamp of the next scheduled eviction, or nullopt if no evictions are scheduled.
        */
        std::optional<Timestamp> peekNextExpiry() const;

        /**
        * Removes all keys from the cache whose expiry time is <= @param now.
        * Called by the eviction thread when it wakes up.
        * Requires an exclusive lock to safely modify the cache and eviction heap.
        * 
        * @param now The cutoff timestamp.
        * 
        */
        void evictExpired(Timestamp now);

        /**
        * Compares the new expiry time against the current earliest expiry in the heap.
        * If the new time is earlier (or if there were no expiring keys before), signals
        * the eviction thread to recalculate its wakeup deadline.
        * Uses minimal locking; never blocks on the eviction thread.
        * 
        * @param t The new earliest expiry time.
        */
        void notifyNewExpiry(Timestamp t);

        /**
        * Gives the eviction thread a way to register the wakeup function.
        * 
        * @param cb The callback to call when the eviction thread needs to wake up.
        */
        void setNotifyWakeup(std::function<void()> cb) { m_notifyWakeup = std::move(cb); }
       
        
    private:
        std::unordered_map<std::string, CacheEntry> m_cache {};
        std::priority_queue<
            std::pair<Timestamp, std::string>,
            std::vector<std::pair<Timestamp, std::string>>,
            EvictionComparator
        > m_evictionHeap {};
        std::unordered_map<std::string, std::deque<LogEntry>> m_logs {};
        std::function<void()> m_notifyWakeup {};
        mutable std::shared_mutex m_mutex {};
        std::unique_ptr<EvictionThread> m_evictionThread;

        /**
        * Returns the logs needed for REPLAY for a given key.
        */
        std::deque<LogEntry> getLogsForReplay(const std::string& key, Timestamp cutoff) const;
    };
}
