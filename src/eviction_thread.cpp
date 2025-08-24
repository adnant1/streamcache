#include "eviction_thread.h"
#include "shard.h"
#include <cassert>
#include <chrono>

namespace streamcache {

    /*
    * Fixed log retention duration for all keys.
    */
    const auto LOG_RETENTION = std::chrono::hours(1);

    void EvictionThread::start(Shard& target) {
        assert(!m_thread.joinable());
        assert(!m_running.load(std::memory_order_relaxed));
        assert(m_shard == nullptr);

        m_shard = &target;

        target.setNotifyWakeup([this] {
            m_cv.notify_all();
        });

        m_running.store(true, std::memory_order_relaxed);
        m_thread = std::thread(&EvictionThread::runLoop, this);
    }

    void EvictionThread::stop() {
        if (!m_running.exchange(false)) {
            return;
        }

        m_cv.notify_all();

        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    EvictionThread::~EvictionThread() {
        stop();
    }

    void EvictionThread::runLoop() {
        while (m_running.load(std::memory_order_relaxed)) {
            std::optional<Timestamp> nextExpiry {m_shard->peekNextExpiry()};

            /*
            * If the time has already reached/passed, don't bother sleeping.
            * Fall through to shutdown check + eviction below.
            */
            if (nextExpiry && std::chrono::steady_clock::now() >= *nextExpiry) {

            } else {
                // Sleep until a deadline appears or arrives, or until shutdown.
                {
                    std::unique_lock<std::mutex> lock(m_cvMutex);
    
                    if(!nextExpiry) {
                        m_cv.wait(lock, [this] {
                            return !m_running.load(std::memory_order_relaxed) || m_shard->peekNextExpiry().has_value();
                        });
                    } else {
                        const Timestamp deadline {*nextExpiry};
                        m_cv.wait_until(lock, deadline, [this, deadline] {
                            return !m_running.load(std::memory_order_relaxed) || std::chrono::steady_clock::now() >= deadline;
                        });
                    }
                }
            }

            /*
            * If the thread woke up because a new deadline appeared (no-deadline case),
            * we don't necessarily want to evict yet. Re-loop to fetch it precisely.
            */
           if (!nextExpiry) {
                continue;
           }

            if (!m_running.load(std::memory_order_relaxed)) {
                break;
            }

            const auto now {std::chrono::steady_clock::now()};
            m_shard->evictExpired(now);
            m_shard->pruneAllLogs(now - LOG_RETENTION);
        }
    }
}