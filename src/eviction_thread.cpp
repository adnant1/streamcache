#include "eviction_thread.h"
#include <cassert>

namespace streamcache {

    void EvictionThread::start(Cache& target) {
        assert(!m_thread.joinable());
        assert(!m_running.load(std::memory_order_relaxed));
        assert(m_cache == nullptr);

        m_cache = &target;

        target.setNotifyWakeup([this] {
            m_cv.notify_all();
        });

        m_running.store(true, std::memory_order_relaxed);
        m_thread = std::thread(&EvictionThread::runLoop, this);
    }
}