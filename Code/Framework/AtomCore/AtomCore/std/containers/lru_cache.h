/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>

namespace AZStd
{
    /**
     * This class is a simple map which keeps an least-recently-used list of elements. If the capacity
     * of the map is exceeded with a new insertion, the oldest element is evicted.
     */
    template<typename KeyType, typename MappedType, class Hasher = AZStd::hash<KeyType>, class EqualKey = AZStd::equal_to<KeyType>, class Allocator = AZStd::allocator>
    class lru_cache
    {
    public:
        typedef KeyType                             key_type;
        typedef MappedType                          mapped_type;
        typedef pair<KeyType, MappedType>           value_type;
        typedef list<value_type, Allocator>         list_type;

        typedef typename list_type::iterator                  iterator;
        typedef typename list_type::const_iterator            const_iterator;
        typedef typename list_type::reverse_iterator          reverse_iterator;
        typedef typename list_type::const_reverse_iterator    const_reverse_iterator;
        typedef pair<iterator, bool>                          pair_iter_bool;

        lru_cache() = default;
        explicit lru_cache(size_t capacity) : m_capacity(capacity) {}

        /**
         * Inserts \rev value associated with \ref key. If the key already exists, replaces the existing value. The entry
         * is promoted to the most-recently-used.
         */
        pair_iter_bool insert(const KeyType& key, const MappedType& value)
        {
            return insert_impl(key, value);
        }

        /**
         * Constructs a new \ref MappedType with the provided arguments, associated with \ref key. If the key already exists,
         * replaces the existing mapped type. The entry is promoted to the most-recently-used.
         */
        template <typename... Args>
        pair_iter_bool emplace(const KeyType& key, Args&&... arguments)
        {
            return insert_impl(key, AZStd::forward<Args>(arguments)...);
        }

        /**
         * Returns the mapped type based on the key. The entry is promoted to be the most recently used.
         */
        iterator get(const KeyType& key)
        {
            auto it = m_cacheMap.find(key);
            if (it != m_cacheMap.end())
            {
                m_cacheList.splice(m_cacheList.begin(), m_cacheList, it->second);
                return begin();
            }
            return end();
        }

        /**
         * Returns whether the key exists in the container. Does *not* promote the entry.
         */
        bool exists(const key_type& key) const
        {
            return m_cacheMap.find(key) != m_cacheMap.end();
        }

        /**
         * Adjusts the capacity of the container. If the new capacity is smaller than the existing size, elements
         * will be evicted until the capacity is reached.
         */
        void set_capacity(size_t capacity)
        {
            m_capacity = capacity;
            trim_to_fit();
        }

        void clear()
        {
            m_cacheMap.clear();
            m_cacheList.clear();
        }

        size_t                  capacity() const { return m_capacity; }
        size_t                  size() const    { return m_cacheMap.size(); }
        bool                    empty() const   { return m_cacheMap.empty(); }

        iterator                begin()         { return m_cacheList.begin(); }
        const_iterator          begin() const   { return m_cacheList.begin(); }
        iterator                end()           { return m_cacheList.end(); }
        const_iterator          end() const     { return m_cacheList.end(); }

        reverse_iterator        rbegin()        { return m_cacheList.rbegin(); }
        const_reverse_iterator  rbegin() const  { return m_cacheList.rbegin(); }
        reverse_iterator        rend()          { return m_cacheList.rend(); }
        const_reverse_iterator  rend() const    { return m_cacheList.rend(); }

    private:
        template <typename... Args>
        pair_iter_bool insert_impl(const key_type& key, Args&&... arguments)
        {
            AZSTD_CONTAINER_ASSERT(m_capacity != 0, "Attempting to insert an element into cache with no capacity.");

            auto it = m_cacheMap.find(key);
            m_cacheList.emplace_front(key, AZStd::forward<Args>(arguments)...);

            const bool keyExisted = it != m_cacheMap.end();
            if (keyExisted)
            {
                m_cacheList.erase(it->second);
                m_cacheMap.erase(it);
            }
            m_cacheMap.emplace(key, m_cacheList.begin());

            trim_to_fit();
            return pair_iter_bool(begin(), keyExisted);
        }

        void trim_to_fit()
        {
            while (m_cacheMap.size() > m_capacity)
            {
                auto last = m_cacheList.end();
                last--;
                m_cacheMap.erase(last->first);
                m_cacheList.pop_back();
            }
        }

        /**
         * The map holds an iterator into the list. The list is ordered by most-to-least recently used, and holds the
         * key / value pair values. This allows us to expose the list iterator externally for easy traversal. The downside
         * is that this does require one more indirection at look-up time, however the list iterator is going to be promoted
         * to the front of the list on access anyway.
         */
        typedef AZStd::unordered_map<key_type, iterator, Hasher, EqualKey, Allocator> map_type;

        /// Contains the flat LRU list, sorted from most used to least recently used.
        list_type m_cacheList;

        /// Stores the key -> list iterator association.
        map_type m_cacheMap;

        /// Old elements will be evicted if the capacity is exceeded.
        size_t m_capacity = 0;
    };
} // namespace AZStd
