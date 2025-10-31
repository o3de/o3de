/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::RHI
{
    template <typename ObjectType>
    struct ObjectCacheEvictionCallbackNull
    {
        void operator() (ObjectType* itemType) const
        {
            (void)itemType;
        }
    };

    //! ObjectCache is a least-recently-used map with a fixed capacity. If the capacity is exceeded,
    //! objects will be evicted from the map. When eviction occurs, the user-provided eviction callback is called.
    //! The cache holds a reference on the object. When eviction occurs that reference is released.
    //!
    //! The user is expected to search for an object, and if it doesn't exist, insert it. The inserted
    //! object should not be cached externally unless a reference is taken, since eviction results in
    //! forfeit of its internal reference.
    template <typename ObjectType, typename KeyType = size_t, typename EvictionCallback = ObjectCacheEvictionCallbackNull<ObjectType>>
    class ObjectCache
    {
    public:
        ObjectCache() = default;
        ObjectCache(EvictionCallback evictionCallback)
            : m_evictionCallback{ evictionCallback }
        {}

        ~ObjectCache();

        //! Clears the object cache. This will call the eviction callback on each object, and
        //! release all internal references. The cache will then be empty.
        void Clear()
        {
            while (m_cacheList.size())
            {
                ReleaseItem();
            }
        }

        //! Specify a custom eviction callback to call when an object is evicted.
        void SetEvictionCallback(EvictionCallback evictionCallback)
        {
            m_evictionCallback = evictionCallback;
        }

        //! Sets the capacity of the cache. If capacity is exceeded when inserting, the least-recently-used object
        //! will be evicted.
        void SetCapacity(size_t capacity)
        {
            AZ_Assert(capacity != 0, "Capacity cannot be 0.");

            // If the user set the capacity to something smaller than the working set,
            // then purge items from the cache until we are below the new watermark.
            while (m_cacheList.size() >= m_capacity)
            {
                ReleaseItem();
            }

            m_capacity = capacity;
        }

        //! Returns the capacity of the cache.
        size_t GetCapacity() const
        {
            return m_capacity;
        }

        //! Returns the number of objects in the cache.
        size_t GetSize() const
        {
            return m_cacheList.size();
        }

        //! Finds an object in the cache. If the object exists, it will be returned. If not,
        //! nullptr will be returned.
        ObjectType* Find(KeyType key)
        {
            auto findIt = m_cacheMap.find(key);
            if (findIt != m_cacheMap.end())
            {
                CacheItem& item = *findIt->second;

                // Pull item from list and put at front of LRU.
                m_cacheList.erase(item);
                m_cacheList.push_front(item);

                return item.m_object.get();
            }
            return nullptr;
        }

        //! Erase a specific item with the provided key from the cache. 
        void EraseItem(KeyType key)
        {
            auto findIt = m_cacheMap.find(key);
            if (findIt != m_cacheMap.end())
            {
                CacheItem* cacheItem = findIt->second;
                m_cacheMap.erase(cacheItem->m_key);
                m_cacheList.erase(*cacheItem);
                m_evictionCallback(cacheItem->m_object.get());
                delete cacheItem;
            }
        }

        //! Inserts an object with a specified key into the cache. The cache will evict an
        //! object before inserting if at capacity.
        void Insert(KeyType key, RHI::Ptr<ObjectType> object)
        {
            if (m_cacheList.size() == m_capacity)
            {
                ReleaseItem();
            }

            // Updates require more complicated logic to evict the old object. For simplicity, this
            // is currently not allowed.
            AZ_Assert(Find(key) == nullptr, "Updating an existing key is currently unsupported.");

            CacheItem* cacheItem = aznew CacheItem;
            cacheItem->m_object = AZStd::move(object);
            cacheItem->m_key = key;

            m_cacheList.push_front(*cacheItem);
            m_cacheMap.emplace(key, cacheItem);
        }

    private:
        void ReleaseItem()
        {
            CacheItem* cacheItem = &m_cacheList.back();
            m_cacheMap.erase(cacheItem->m_key);
            m_cacheList.pop_back();
            m_evictionCallback(cacheItem->m_object.get());
            delete cacheItem;
        }

        class CacheItem
            : public AZStd::intrusive_list_node<CacheItem>
        {
        public:
            AZ_CLASS_ALLOCATOR(CacheItem, AZ::ThreadPoolAllocator);

            KeyType m_key;
            Ptr<ObjectType> m_object = nullptr;
        };

        size_t m_capacity = static_cast<size_t>(-1); /// Defaults to infinity.
        AZStd::unordered_map<KeyType, CacheItem*> m_cacheMap;
        AZStd::intrusive_list<CacheItem, AZStd::list_base_hook<CacheItem>> m_cacheList;
        EvictionCallback m_evictionCallback;
    };

    template <typename ObjectType, typename KeyType, typename DeleterType>
    ObjectCache<ObjectType, KeyType, DeleterType>::~ObjectCache()
    {
        Clear();
    }
}
