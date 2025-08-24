#pragma once
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>

namespace streamcache {

    // Foward declaration to avoid circular dependency
    class Shard;
    
    /**
    * @class EvictionThread
    * @brief Owns and manages the background eviction thread for a single shard.
    *
    * The EvictionThread monitors the earliest expiry in the target cache and
    * wakes exactly when needed to evict expired entries in batches. Additionally,
    * it maintains log entries by pruning logs older than a fixed retention duration
    * to prevent unbounded memory growth. It is designed to be event-driven (not 
    * polling) and uses a condition variable to sleep until either:
    *   1. The next scheduled eviction time is reached.
    *   2. It is notified of an earlier expiry via Shard::notifyNewExpiry().
    *
    * On each wake-up cycle, the thread performs:
    * - Cache eviction: Removes expired cache entries
    * - Log maintenance: Prunes log entries older than the retention duration
    *
    * Lifecycle:
    * - Call start(Shard&) once to launch the eviction thread.
    * - Call stop() (or let the destructor call it) to shut down the thread
    *   cleanly before destruction.
    * - Safe to call stop() multiple times (idempotent).
    *
    * Thread safety:
    * - The EvictionThread does not modify Shard internals directly; it calls
    *   public, lock-aware methods on the Shard (peekNextExpiry, evictExpired, pruneAllLogs).
    * - Condition variable mutex is used only for sleep/wake coordination.
    *
    */
    class EvictionThread {
        public:
            /**
             * Default constructor.
             */
            EvictionThread() = default;

            /**
             * Start the eviction thread and begin monitoring the shard.
             * 
             * @param target Reference to the shard this thread will manage.
             */
            void start(Shard& target);

            /**
             * Signal the eviction thread to exit, wake if sleeping, and join() it.
             */
            void stop();

            /**
             * Automatically stop and clean up the eviction thread if the object
             * is destroyed without calling stop().
             */
            ~EvictionThread();

        private:
            std::thread m_thread;
            std::atomic<bool> m_running {false};
            std::condition_variable_any m_cv {};
            std::mutex m_cvMutex {};
            Shard* m_shard {nullptr};

            /**
             * Main loop for the eviction thread.
             * Waits until the next scheduled expiry or until notified of an earlier one,
             * then evicts expired entries in batches until none are due.
             */
            void runLoop();
    };

}