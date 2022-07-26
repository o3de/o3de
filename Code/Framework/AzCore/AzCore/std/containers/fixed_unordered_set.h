/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class Hasher, class EqualKey, bool isMultiSet, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements>
        struct UnorderedFixedSetTableTraits
        {
            using key_type = Key;
            using key_equal = EqualKey;
            using hasher = Hasher;
            using value_type = Key;
            using allocator_type = AZStd::no_default_allocator;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiSet,
                is_dynamic = false,
                fixed_num_buckets = FixedNumBuckets, // a prime number between 1/3 - 1/4 of the number of elements.
                fixed_num_elements = FixedNumElements
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    }

    /**
    * Fixed unordered set container is based on unordered set and complaint with \ref CTR1 (6.2.4.3).
    * This is an associative container, all Keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    * It has the extensions in the \ref hash_table class.
    * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
    * a prime number and about 1/3-1/4 of the FixedNumElements.
    *
    * Check the unordered_set \ref AZStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class fixed_unordered_set
        : public hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        using base_type = hash_table<Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements>>;
    public:
        using traits_type = typename base_type::traits_type;

        using key_type = typename base_type::key_type;
        using key_equal = typename base_type::key_equal;
        using hasher = typename base_type::hasher;

        using allocator_type = typename base_type::allocator_type;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using pointer = typename base_type::pointer;
        using const_pointer = typename base_type::const_pointer;
        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;

        using value_type = typename base_type::value_type;

        using local_iterator = typename base_type::local_iterator;
        using const_local_iterator = typename base_type::const_local_iterator;

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;

        fixed_unordered_set()
            : base_type(hasher(), key_equal()) {}
        explicit fixed_unordered_set(size_type numBuckets,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        fixed_unordered_set(Iterator first, Iterator last, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBuckets);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_set(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }
        fixed_unordered_set(const fixed_unordered_set& rhs)
            : base_type(rhs) {}
        fixed_unordered_set(fixed_unordered_set&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }


        fixed_unordered_set(const initializer_list<value_type>& list, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }
        fixed_unordered_set(size_type numBucketsHint, const hasher& hash)
            : fixed_unordered_set(numBucketsHint, hash, key_equal())
        {
        }

        template<class InputIterator>
        fixed_unordered_set(InputIterator f, InputIterator l, size_type n, const hasher& hf)
            : fixed_unordered_set(f, l, n, hf, key_equal())
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_set(from_range_t, R&& rg, size_type n, const hasher& hf)
            : fixed_unordered_set(from_range, AZStd::forward<R>(rg), n, hf, key_equal())
        {
        }
        fixed_unordered_set(initializer_list<value_type> il, size_type n, const hasher& hf)
            : fixed_unordered_set(il, n, hf, key_equal())
        {
        }

        /// This constructor is AZStd extension
        fixed_unordered_set(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual)
        {
        }
    };

    template<class Key, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }

    /**
    * Fixed unordered multi set container is based on unordered multiset and complaint with \ref CTR1 (6.2.4.5)
    * The only difference from the unordered_set is that we allow multiple entries with
    * the same key. You can iterate over them, by checking if the key stays the same.
    * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
    * a prime number and about 1/3-1/4 of the FixedNumElements.
    *
    * Check the unordered_multiset \ref AZStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class fixed_unordered_multiset
        : public hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        using base_type = hash_table<Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements>>;
    public:
        using traits_type = typename base_type::traits_type;

        using key_type = typename base_type::key_type;
        using key_equal = typename base_type::key_equal;
        using hasher = typename base_type::hasher;

        using allocator_type = typename base_type::allocator_type;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using pointer = typename base_type::pointer;
        using const_pointer = typename base_type::const_pointer;
        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;

        using value_type = typename base_type::value_type;

        using local_iterator = typename base_type::local_iterator;
        using const_local_iterator = typename base_type::const_local_iterator;

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;

        fixed_unordered_multiset()
            : base_type(hasher(), key_equal()) {}
        explicit fixed_unordered_multiset(size_type numBuckets,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        fixed_unordered_multiset(Iterator first, Iterator last, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBuckets);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_multiset(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }
        fixed_unordered_multiset(const fixed_unordered_multiset& rhs)
            : base_type(rhs) {}
        fixed_unordered_multiset(fixed_unordered_multiset&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }

        fixed_unordered_multiset(const initializer_list<value_type>& list, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }
        fixed_unordered_multiset(size_type numBucketsHint, const hasher& hash)
            : fixed_unordered_multiset(numBucketsHint, hash, key_equal())
        {
        }
        template<class InputIterator>
        fixed_unordered_multiset(InputIterator f, InputIterator l, size_type n, const hasher& hf)
            : fixed_unordered_multiset(f, l, n, hf, key_equal())
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_multiset(from_range_t, R&& rg, size_type n, const hasher& hf)
            : fixed_unordered_multiset(from_range, AZStd::forward<R>(rg), n, hf, key_equal())
        {
        }
        fixed_unordered_multiset(initializer_list<value_type> il, size_type n, const hasher& hf)
            : fixed_unordered_multiset(il, n, hf, key_equal())
        {
        }

        /// This constructor is AZStd extension
        fixed_unordered_multiset(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual)
        {
        }
    };

    template<class Key, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }
}
