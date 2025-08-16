#pragma once
#include "cache.h"
#include <thread>
#include <condition_variable>

namespace streamcache {
    
    /**
    * @class EvictionThread
    * @brief Owns and manages the background eviction thread for a Cache instance.
    *
    * The EvictionThread monitors the earliest expiry in the target cache and
    * wakes exactly when needed to evict expired entries in batches. It is
    * designed to be event-driven (not polling) and uses a condition variable
    * to sleep until either:
    *   1. The next scheduled eviction time is reached.
    *   2. It is notified of an earlier expiry via Cache::notifyNewExpiry().
    *
    * Lifecycle:
    * - Call start(Cache&) once to launch the eviction thread.
    * - Call stop() (or let the destructor call it) to shut down the thread
    *   cleanly before destruction.
    * - Safe to call stop() multiple times (idempotent).
    *
    * Thread safety:
    * - The EvictionThread does not modify Cache internals directly; it calls
    *   public, lock-aware methods on Cache (peekNextExpiry, evictExpired).
    * - Condition variable mutex is used only for sleep/wake coordination.
    *
    * Typical usage:
    *   streamcache::EvictionThread thread;
    *   runner.start(myCache);
    *   ...
    *   runner.stop(); // optional, destructor will also stop
    */
    class EvictionThread {
        public:
            /**
             * Start the eviction thread and begin monitoring the cache.
             * 
             * @param target Reference to the cache this thread will manage.
             */
            void start(Cache& target);

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
            Cache* m_cache {nullptr};

            /**
             * Main loop for the eviction thread.
             * Waits until the next scheduled expiry or until notified of an earlier one,
             * then evicts expired entries in batches until none are due.
             */
            void runLoop();
    };

}