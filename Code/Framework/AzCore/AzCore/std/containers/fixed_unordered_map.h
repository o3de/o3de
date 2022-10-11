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
        template<class Key, class MappedType, class Hasher, class EqualKey, bool isMultiMap, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements>
        struct UnorderedFixedMapTableTraits
        {
            using key_type = Key;
            using key_equal = EqualKey;
            using hasher = Hasher;
            using value_type = AZStd::pair<Key, MappedType>;
            using allocator_type = AZStd::no_default_allocator;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiMap,
                is_dynamic = false,
                fixed_num_buckets = FixedNumBuckets, // a prime number between 1/3 - 1/4 of the number of elements.
                fixed_num_elements = FixedNumElements
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };

        /**
         * Used when we want to insert entry with a key only, default construct for the
         * value. This rely on that AZStd::pair (map value type) can be constructed with a key only (first element).
         */
        template<class KeyType>
        struct ConvertKeyTypeFixed
        {
            typedef KeyType             key_type;

            AZ_FORCE_INLINE const KeyType&      to_key(const KeyType& key) const    { return key; }
            // We return key as the value so the pair is constructed using Pair(first) ctor.
            AZ_FORCE_INLINE const KeyType&      to_value(const KeyType& key) const  { return key; }
        };
    }

    /**
     * Fixed unordered map container is based on unordered map \ref CTR1 (6.2.4.4) but has fixed amount of buckets and elements.
     * This is an associative container with pair(Key,MappedType), all Keys are unique.
     * insert function will return false, if you try to add key that is in the map.
     * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
     * a prime number and about 1/3-1/4 of the FixedNumElements.
     *
     * It has the following extensions from \ref hash_table
     * and \ref UMapExtensions.
     * Check the fixed_unordered_map \ref AZStdExamples.
     */
    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class fixed_unordered_map
        : public hash_table< Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        using base_type = hash_table<Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements>>;
    public:
        using traits_type = typename base_type::traits_type;

        using key_type = typename base_type::key_type;
        using key_equal = typename base_type::key_equal;
        using hasher = typename base_type::hasher;
        using mapped_type = MappedType;

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

        using pair_iter_bool = typename base_type::pair_iter_bool;

        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;
        using insert_return_type = AssociativeInternal::insert_return_type<iterator, node_type>;

        fixed_unordered_map()
            : base_type(hasher(), key_equal()) {}
        explicit fixed_unordered_map(size_type numBucketsHint,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class InputIterator>
        fixed_unordered_map(InputIterator first, InputIterator last, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_map(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }

        fixed_unordered_map(const fixed_unordered_map& rhs)
            : base_type(rhs) {}
        fixed_unordered_map(fixed_unordered_map&& rhs)
            : base_type(AZStd::move(rhs)) {}

        fixed_unordered_map(initializer_list<value_type> list, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(list);
        }
        fixed_unordered_map(size_type numBucketsHint, const hasher& hash)
            : fixed_unordered_map(numBucketsHint, hash, key_equal())
        {
        }
        template<class InputIterator>
        fixed_unordered_map(InputIterator f, InputIterator l, size_type n, const hasher& hf)
            : fixed_unordered_map(f, l, n, hf, key_equal())
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_map(from_range_t, R&& rg, size_type n, const hasher& hf)
            : fixed_unordered_map(from_range, AZStd::forward<R>(rg), n, hf, key_equal())
        {
        }
        fixed_unordered_map(initializer_list<value_type> il, size_type n, const hasher& hf)
            : fixed_unordered_map(il, n, hf, key_equal())
        {
        }

        /// This constructor is AZStd extension
        fixed_unordered_map(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual)
        {
        }

        using base_type::insert;
        using base_type::insert_range;
        AZ_FORCE_INLINE pair_iter_bool insert(const value_type& value)
        {
            return base_type::insert_impl(value);
        }
        /**
        * Look up operator if element doesn't exists inserts a new one with (key,mapped_type()).
        */
        AZ_FORCE_INLINE mapped_type& operator[](const key_type& key)
        {
            pair_iter_bool iterBool = insert_key(key);
            return iterBool.first->second;
        }
        /**
        * Returns mapped type with based on the key, if the element doesn't exist an assert it triggered!
        */
        AZ_FORCE_INLINE mapped_type& at(const key_type& key)
        {
            iterator iter = base_type::find(key);
            AZSTD_CONTAINER_ASSERT(iter != base_type::end(), "Element with key %d is not present", key);
            return iter->second;
        }
        AZ_FORCE_INLINE const mapped_type& at(const key_type& key) const
        {
            const_iterator iter = base_type::find(key);
            AZSTD_CONTAINER_ASSERT(iter != base_type::end(), "Element with key %d is not present", key);
            return iter->second;
        }

        /*
         * \anchor UMapExtensions
         * \name Extensions
         * @{
         */
        /**
         * Insert a pair with default value base on a key only (AKA lazy insert). This can be a speed up when
         * the object has complicated assignment function.
         */
        AZ_FORCE_INLINE pair_iter_bool insert_key(const key_type& key)
        {
            Internal::ConvertKeyTypeFixed<key_type> converter;
            return base_type::insert_from(key, converter, base_type::m_hasher, base_type::m_keyEqual);
        }
        /// @}
    };

    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }

    /**
     * Fixed unordered multi map container is based on unordered_multimap \ref CTR1 (6.2.4.6)
     * The only difference from the fixed_unordered_map is that we allow multiple entries with
     * the same key. You can iterate over them, by checking if the key is the same.
     * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
     * a prime number and about 1/3-1/4 of the FixedNumElements.
     *
     * Check the fixed_unordered_multimap \ref AZStdExamples.
     */
    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key> >
    class fixed_unordered_multimap
        : public hash_table< Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        using base_type = hash_table<Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements>>;
    public:
        using traits_type = typename base_type::traits_type;

        using key_type = typename base_type::key_type;
        using key_equal = typename base_type::key_equal;
        using hasher = typename base_type::hasher;
        using mapped_type = MappedType;

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

        using pair_iter_bool = typename base_type::pair_iter_bool;

        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;

        fixed_unordered_multimap()
            : base_type(hasher(), key_equal()) {}
        explicit fixed_unordered_multimap(size_type numBucketsHint,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class InputIterator>
        fixed_unordered_multimap(InputIterator first, InputIterator last, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_multimap(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }

        fixed_unordered_multimap(const fixed_unordered_multimap& rhs)
            : base_type(rhs) {}
        fixed_unordered_multimap(fixed_unordered_multimap&& rhs)
            : base_type(AZStd::move(rhs)) {}

        fixed_unordered_multimap(initializer_list<value_type> list, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal())
            : base_type(hash, keyEqual)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(list);
        }
        fixed_unordered_multimap(size_type numBucketsHint, const hasher& hash)
            : fixed_unordered_multimap(numBucketsHint, hash, key_equal())
        {
        }
        template<class InputIterator>
        fixed_unordered_multimap(InputIterator f, InputIterator l, size_type n, const hasher& hf)
            : fixed_unordered_multimap(f, l, n, hf, key_equal())
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        fixed_unordered_multimap(from_range_t, R&& rg, size_type n, const hasher& hf)
            : fixed_unordered_multimap(from_range, AZStd::forward<R>(rg), n, hf, key_equal())
        {
        }
        fixed_unordered_multimap(initializer_list<value_type> il, size_type n, const hasher& hf)
            : fixed_unordered_multimap(il, n, hf, key_equal())
        {
        }

        /// This constructor is AZStd extension
        fixed_unordered_multimap(const hasher& hash, const key_equal& keyEqual)
            : base_type(hash, keyEqual)
        {
        }

        using base_type::insert;
        using base_type::insert_range;
        AZ_FORCE_INLINE iterator insert(const value_type& value)
        {
            return base_type::insert_impl(value).first;
        }

        /**
        * Insert a pair with default value base on a key only (AKA lazy insert). This can be a speed up when
        * the object has complicated assignment function.
        */
        AZ_FORCE_INLINE pair_iter_bool insert_key(const key_type& key)
        {
            Internal::ConvertKeyTypeFixed<key_type> converter;
            return base_type::insert_from(key, converter, base_type::m_hasher, base_type::m_keyEqual);
        }
    };

    template<class Key, class MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey >
    AZ_FORCE_INLINE void swap(fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }
}
