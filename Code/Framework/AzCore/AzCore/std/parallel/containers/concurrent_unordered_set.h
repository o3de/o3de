/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_PARALLEL_CONTAINERS_CONCURRENT_UNORDERED_SET_H
#define AZSTD_PARALLEL_CONTAINERS_CONCURRENT_UNORDERED_SET_H 1

#include <AzCore/std/parallel/containers/internal/concurrent_hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class Hasher, class EqualKey, class Allocator, bool isMultiSet, AZStd::size_t NumLocks>
        struct ConcurrentUnorderedSetTableTraits
        {
            typedef Key         key_type;
            typedef EqualKey    key_equal;
            typedef Hasher      hasher;
            typedef Key         value_type;
            typedef Allocator   allocator_type;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiSet,
                is_dynamic = true,
                num_locks = NumLocks
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    }

    /**
     * Concurrent unordered set container, similar to unordered_set, but uses a striped array of locks to allow
     * concurrent access.
     */
    template<class Key, AZStd::size_t NumLocks = 8, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator>
    class concurrent_unordered_set
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, false, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_unordered_set<Key, NumLocks, Hasher, EqualKey, Allocator> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, false, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_equal   key_equal;
        typedef typename base_type::hasher      hasher;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        AZ_FORCE_INLINE concurrent_unordered_set()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        AZ_FORCE_INLINE concurrent_unordered_set(size_type numBucketsHint)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE concurrent_unordered_set(size_type numBucketsHint, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE concurrent_unordered_set(size_type numBucketsHint, const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            rehash(numBucketsHint);
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_set(Iterator first, Iterator last)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_set(Iterator first, Iterator last, size_type numBucketsHint)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_set(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_set(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
    };

    template<class Key, AZStd::size_t NumLocks, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(concurrent_unordered_set<Key, NumLocks, Hasher, EqualKey, Allocator>& left, concurrent_unordered_set<Key, NumLocks, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }

    /**
     * Concurrent unordered multiset container, similar to unordered_multiset, but uses a striped array of locks to
     * allow concurrent access.
     */
    template<class Key, AZStd::size_t NumLocks = 8, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator>
    class concurrent_unordered_multiset
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, true, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_unordered_multiset<Key, NumLocks, Hasher, EqualKey, Allocator> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, true, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_equal   key_equal;
        typedef typename base_type::hasher      hasher;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        AZ_FORCE_INLINE concurrent_unordered_multiset()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        AZ_FORCE_INLINE concurrent_unordered_multiset(size_type numBuckets)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            rehash(numBuckets);
        }
        AZ_FORCE_INLINE concurrent_unordered_multiset(size_type numBuckets, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            rehash(numBuckets);
        }
        AZ_FORCE_INLINE concurrent_unordered_multiset(size_type numBuckets, const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            rehash(numBuckets);
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_multiset(Iterator first, Iterator last)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_multiset(Iterator first, Iterator last, size_type numBuckets)
            : base_type(hasher(), key_equal(), allocator_type())
        {
            rehash(numBuckets);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_multiset(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            rehash(numBuckets);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE concurrent_unordered_multiset(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            rehash(numBuckets);
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
    };

    template<class Key, AZStd::size_t NumLocks, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(concurrent_unordered_multiset<Key, NumLocks, Hasher, EqualKey, Allocator>& left, concurrent_unordered_multiset<Key, NumLocks, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }
}

#endif // AZSTD_PARALLEL_CONTAINERS_CONCURRENT_UNORDERED_SET_H
#pragma once
