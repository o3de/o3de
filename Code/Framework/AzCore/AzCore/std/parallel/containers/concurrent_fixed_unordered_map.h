/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_MAP_H
#define AZSTD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_MAP_H 1

#include <AzCore/std/parallel/containers/internal/concurrent_hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class Hasher, class EqualKey, bool isMultiMap, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, AZStd::size_t NumLocks>
        struct ConcurrentUnorderedFixedMapTableTraits
        {
            typedef Key                             key_type;
            typedef EqualKey                        key_equal;
            typedef Hasher                          hasher;
            typedef AZStd::pair<Key, MappedType>     value_type;
            typedef AZStd::no_default_allocator     allocator_type;
            enum
            {
                has_multi_elements = isMultiMap,
                is_dynamic = false,
                fixed_num_buckets = FixedNumBuckets, // a prime number between 1/3 - 1/4 of the number of elements.
                fixed_num_elements = FixedNumElements,
                num_locks = NumLocks
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };
    }

    /**
     * Concurrent fixed unordered map container, similar to fixed_unordered_map, but uses a striped array of locks to
     * allow concurrent access.
     */
    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, AZStd::size_t NumLocks = 8, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class concurrent_fixed_unordered_map
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_equal   key_equal;
        typedef typename base_type::hasher      hasher;
        typedef MappedType                      mapped_type;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        AZ_FORCE_INLINE concurrent_fixed_unordered_map()
            : base_type(hasher(), key_equal()) {}
        AZ_FORCE_INLINE concurrent_fixed_unordered_map(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual) {}
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_fixed_unordered_map(Iterator first, Iterator last)
            : base_type(hasher(), key_equal())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_fixed_unordered_map(Iterator first, Iterator last, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }

        bool find(const key_type& keyValue) const
        {
            return base_type::find(keyValue);
        }

        bool find(const key_type& keyValue, mapped_type* mappedOut) const
        {
            value_type result;
            if (base_type::find(keyValue, &result))
            {
                *mappedOut = result.second;
                return true;
            }
            return false;
        }
    };

    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, AZStd::size_t NumLocks, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(concurrent_fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& left, concurrent_fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }

    /**
     * Concurrent fixed unordered multimap container, similar to fixed_unordered_multimap, but uses a striped array of
     * locks to allow concurrent access.
     */
    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, AZStd::size_t NumLocks = 8, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class concurrent_fixed_unordered_multimap
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_equal   key_equal;
        typedef typename base_type::hasher      hasher;
        typedef MappedType                      mapped_type;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        AZ_FORCE_INLINE concurrent_fixed_unordered_multimap()
            : base_type(hasher(), key_equal()) {}
        AZ_FORCE_INLINE concurrent_fixed_unordered_multimap(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual) {}
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_fixed_unordered_multimap(Iterator first, Iterator last)
            : base_type(hasher(), key_equal())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_fixed_unordered_multimap(Iterator first, Iterator last, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }

        bool find(const key_type& keyValue) const
        {
            return base_type::find(keyValue);
        }

        bool find(const key_type& keyValue, mapped_type* mappedOut) const
        {
            value_type result;
            if (base_type::find(keyValue, &result))
            {
                *mappedOut = result.second;
                return true;
            }
            return false;
        }
    };

    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, AZStd::size_t NumLocks, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(concurrent_fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& left, concurrent_fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }
}

#endif // AZSTD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_MAP_H
#pragma once
