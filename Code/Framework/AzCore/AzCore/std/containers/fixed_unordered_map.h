/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZSTD_FIXED_UNORDERED_MAP_H
#define AZSTD_FIXED_UNORDERED_MAP_H 1

#include <AzCore/std/hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class Hasher, class EqualKey, bool isMultiMap, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements>
        struct UnorderedFixedMapTableTraits
        {
            typedef Key                             key_type;
            typedef EqualKey                        key_eq;
            typedef Hasher                          hasher;
            typedef AZStd::pair<Key, MappedType>     value_type;
            typedef AZStd::no_default_allocator     allocator_type;
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

        typedef fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey> this_type;
        typedef hash_table< Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;
        typedef MappedType                      mapped_type;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::iterator                    iterator;
        typedef typename base_type::const_iterator              const_iterator;

        //typedef typename base_type::reverse_iterator          reverse_iterator;
        //typedef typename base_type::const_reverse_iterator        const_reverse_iterator;

        typedef typename base_type::value_type                  value_type;

        typedef typename base_type::local_iterator              local_iterator;
        typedef typename base_type::const_local_iterator        const_local_iterator;

        typedef typename base_type::pair_iter_bool              pair_iter_bool;

        AZ_FORCE_INLINE fixed_unordered_map()
            : base_type(hasher(), key_eq()) {}
        AZ_FORCE_INLINE fixed_unordered_map(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual) {}
        template<class Iterator>
        fixed_unordered_map(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        fixed_unordered_map(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }

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

        typedef fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey> this_type;
        typedef hash_table< Internal::UnorderedFixedMapTableTraits<Key, MappedType, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;
        typedef MappedType                      mapped_type;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::iterator                    iterator;
        typedef typename base_type::const_iterator              const_iterator;

        //typedef typename base_type::reverse_iterator          reverse_iterator;
        //typedef typename base_type::const_reverse_iterator        const_reverse_iterator;

        typedef typename base_type::value_type                  value_type;

        typedef typename base_type::local_iterator              local_iterator;
        typedef typename base_type::const_local_iterator        const_local_iterator;

        typedef typename base_type::pair_iter_bool              pair_iter_bool;

        AZ_FORCE_INLINE fixed_unordered_multimap()
            : base_type(hasher(), key_eq()) {}
        AZ_FORCE_INLINE fixed_unordered_multimap(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual) {}
        template<class Iterator>
        fixed_unordered_multimap(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        fixed_unordered_multimap(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
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

#endif // AZSTD_FIXED_UNORDERED_MAP_H
#pragma once
