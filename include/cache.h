#pragma once
#include <string>
#include <unordered_map>
#include <queue>    
#include <vector>
#include <utility>
#include <optional>
#include <chrono>
#include <shared_mutex>

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
            * If the entry has an expiration time, it is added to the eviction heap.
            * Appends the value to the key's log and prunes old log entries.
            *
            * @param key The key for the cache entry.
            * @param entry The value + metadata to be stored in the cache.
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
            * @return The number of entries evicted. Used for metrics.
            */
            size_t evictExpired(Timestamp now);

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

            /**
            * @return Total number of keys evicted due to expiry since startup.
            */
            size_t getEvictionsTotal() const noexcept { return m_evictionsTotal.load(); }

            /**
            * @return Total number of eviction cycles (wakeups that resulted in evictions).
            */
            size_t getEvictionBatches() const noexcept { return m_evictionBatches.load(); }

            /**
            * @return Current number of entries in the eviction queue.
            */
            size_t getHeapSize() const noexcept { return m_heapSize.load(); }

            /**
            * @return How many times an earlier expiry triggered a wakeup.
            */
            size_t getNotifyEarlierDeadlineCount() const noexcept { return m_notifyEarlierExpiryCount.load(); }

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

            std::atomic<size_t> m_evictionsTotal {0};
            std::atomic<size_t> m_evictionBatches {0};
            std::atomic<size_t> m_heapSize {0};
            std::atomic<size_t> m_notifyEarlierExpiryCount {0};

            /**
            * Makes sure that the key's log only contains entries still inside the key's
            * original TTL window.
            * 
            * @param key The key for which the log should be pruned.
            * @param cutoff The timestamp before which all log entries should be removed.
            */
           void pruneLog(const std::string& key, Timestamp cutoff);
       
    };
}