#pragma once
#include "shard.h"

namespace streamcache {

    /**
     * Cache = top-level router that distributes keys across multiple shards.
     * Each shard is a self-contained mini-cache with its own index, logs,
     * eviction queue, and synchronization primitives.
     */
    class Cache {
        public:
            explicit Cache(size_t numShards);
            ~Cache();

            /**
             * Public API for Cache operations.
             * These methods route to the appropriate shard based on the key.
             */

            void set(const std::string& key, CacheEntry entry);

            std::optional<std::string> get(const std::string& key);

            void replay(const std::string& key);

            void pruneAllLogs(Timestamp cutoff);

        private:
            std::vector<Shard> m_shards {};
            size_t m_numShards {};

            size_t shardFor(const std::string& key) const;
    };
}