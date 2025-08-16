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
}