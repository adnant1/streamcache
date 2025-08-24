#include "cache.h"

namespace streamcache {

    Cache::Cache(size_t numShards)
        : m_numShards(numShards), m_shards(numShards) {
    }
    
    Cache::~Cache() {
        // Shards clean themselves up
    }

    size_t Cache::shardFor(const std::string& key) const {
        size_t h {std::hash<std::string>{}(key)};
        return h % m_numShards;
    }

    void Cache::set(const std::string& key, CacheEntry entry) {
        size_t shardIdx {shardFor(key)};
        m_shards[shardIdx].set(key, std::move(entry));
    }

    std::optional<std::string> Cache::get(const std::string& key) {
        size_t shardIdx {shardFor(key)};
        return m_shards[shardIdx].get(key);
    }

    void Cache::replay(const std::string& key) {
        size_t shardIdx {shardFor(key)};
        m_shards[shardIdx].replay(key);
    }

    void Cache::pruneAllLogs(Timestamp cutoff) {
        for (auto& shard : m_shards) {
            shard.pruneAllLogs(cutoff);
        }
    }
}