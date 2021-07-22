/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_UNORDERED_MAP_H
#define AZSTD_UNORDERED_MAP_H 1

#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator, bool isMultiMap>
        struct UnorderedMapTableTraits
        {
            typedef Key                             key_type;
            typedef EqualKey                        key_eq;
            typedef Hasher                          hasher;
            typedef AZStd::pair<Key, MappedType>     value_type;
            typedef Allocator                       allocator_type;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiMap,

                // These values are NOT used for dynamic maps
                is_dynamic = true,
                fixed_num_buckets = 1,
                fixed_num_elements = 4
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };

        /**
         * Used when we want to insert entry with a key only, default construct for the
         * value. This rely on that AZStd::pair (map value type) can be constructed with a key only (first element).
         */
        template<class KeyType>
        struct ConvertKeyType
        {
            typedef KeyType             key_type;

            AZ_FORCE_INLINE const KeyType&      to_key(const KeyType& key) const { return key; }
            // We return key as the value so the pair is constructed using Pair(first) ctor.
            AZ_FORCE_INLINE const KeyType&      to_value(const KeyType& key) const  { return key; }
        };
    }

    /**
     * Unordered map container is complaint with \ref CTR1 (6.2.4.4)
     * This is an associative container with pair(Key,MappedType), all Keys are unique.
     * insert function will return false, if you try to add key that is in the map.
     *
     * It has the following extensions from \ref hash_table
     * and \ref UMapExtensions.
     * Check the unordered_map \ref AZStdExamples.
     */
    template<class Key, class MappedType, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator >
    class unordered_map
        : public hash_table< Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, false> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef unordered_map<Key, MappedType, Hasher, EqualKey, Allocator> this_type;
        typedef hash_table< Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, false> > base_type;
    public:
        typedef typename base_type::traits_type traits_type;

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

        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;
        using insert_return_type = insert_return_type<iterator, node_type>;

        AZ_FORCE_INLINE unordered_map()
            : base_type(hasher(), key_eq(), allocator_type()) {}
        explicit unordered_map(const allocator_type& alloc)
            : base_type(hasher(), key_eq(), alloc) {}
        AZ_FORCE_INLINE unordered_map(const unordered_map& rhs)
            : base_type(rhs) {}
        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        AZ_FORCE_INLINE unordered_map(const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}
        AZ_FORCE_INLINE unordered_map(size_type numBucketsHint)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE unordered_map(size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE unordered_map(size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_map(Iterator first, Iterator last)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_map(Iterator first, Iterator last, size_type numBucketsHint)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_map(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_map(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        AZ_FORCE_INLINE unordered_map(const std::initializer_list<value_type>& list, const hasher& hash = hasher(), const key_eq& keyEqual = key_eq(), const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }

        AZ_FORCE_INLINE unordered_map(this_type&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            base_type::operator=(AZStd::move(rhs));
            return *this;
        }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            base_type::operator=(rhs);
            return *this;
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
            AZSTD_CONTAINER_ASSERT(iter != base_type::end(), "Element with key is not present");
            return iter->second;
        }
        AZ_FORCE_INLINE const mapped_type& at(const key_type& key) const
        {
            const_iterator iter = base_type::find(key);
            AZSTD_CONTAINER_ASSERT(iter != base_type::end(), "Element with key is not present");
            return iter->second;
        }

        using base_type::insert;
        insert_return_type insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_map::insert(node_type&& nodeHandle)");
            return base_type::template node_handle_insert<insert_return_type>(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_map::insert(const_iterator hint, node_type&& nodeHandle)");
            return base_type::node_handle_insert(hint, AZStd::move(nodeHandle));
        }

        //! C++17 insert_or_assign function assigns the element to the mapped_type if the key exist in the container
        //! Otherwise a new value is inserted into the container
        template <typename M>
        pair_iter_bool insert_or_assign(const key_type& key, M&& value)
        {
            return base_type::insert_or_assign_transparent(key, AZStd::forward<M>(value));
        }
        template <typename M>
        pair_iter_bool insert_or_assign(key_type&& key, M&& value)
        {
            return base_type::insert_or_assign_transparent(AZStd::move(key), AZStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, const key_type& key, M&& value)
        {
            return base_type::insert_or_assign_transparent(hint, key, AZStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, key_type&& key, M&& value)
        {
            return base_type::insert_or_assign_transparent(hint, AZStd::move(key), AZStd::forward<M>(value));
        }

        //! C++17 try_emplace function that does nothing to the arguments if the key exist in the container,
        //! otherwise it constructs the value type as if invoking
        //! value_type(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<KeyType>(key)),
        //!  AZStd::forward_as_tuple(AZStd::forward<Args>(args)...))
        template <typename... Args>
        pair_iter_bool try_emplace(const key_type& key, Args&&... arguments)
        {
            return base_type::try_emplace_transparent(key, AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        pair_iter_bool try_emplace(key_type&& key, Args&&... arguments)
        {
            return base_type::try_emplace_transparent(AZStd::move(key), AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, const key_type& key, Args&&... arguments)
        {
            return base_type::try_emplace_transparent(hint, key, AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, key_type&& key, Args&&... arguments)
        {
            return base_type::try_emplace_transparent(hint, AZStd::move(key), AZStd::forward<Args>(arguments)...);
        }

        node_type extract(const key_type& key)
        {
            return base_type::template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return base_type::template node_handle_extract<node_type>(it);
        }

        /**
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
            Internal::ConvertKeyType<key_type> converter;
            return base_type::insert_from(key, converter, base_type::m_hasher, base_type::m_keyEqual);
        }
        /// @}
    };

    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& left, unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }

    template <class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator==(const unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& a, const unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& b)
    {
        return (a.size() == b.size() && equal(a.begin(), a.end(), b.begin()));
    }

    template <class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator!=(const unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& a, const unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& b)
    {
        return !(a == b);
    }

    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator, class Predicate>
    decltype(auto) erase_if(unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>& container, Predicate predicate)
    {
        auto originalSize = container.size();

        for (auto iter = container.begin(); iter != container.end(); )
        {
            if (predicate(*iter))
            {
                iter = container.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return originalSize - container.size();
    }

    /**
     * Unordered multi map container is complaint with \ref CTR1 (6.2.4.6)
     * The only difference from the unordered_map is that we allow multiple entries with
     * the same key. You can iterate over them, by checking if the key is the same.
     *
     * Check the unordered_multimap \ref AZStdExamples.
     */
    template<class Key, class MappedType, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator >
    class unordered_multimap
        : public hash_table< Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, true> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator> this_type;
        typedef hash_table< Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, true> > base_type;
    public:
        using base_type::insert;
        typedef typename base_type::traits_type traits_type;

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

        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;

        AZ_FORCE_INLINE unordered_multimap()
            : base_type(hasher(), key_eq(), allocator_type()) {}
        explicit unordered_multimap(const allocator_type& alloc)
            : base_type(hasher(), key_eq(), alloc){}
        AZ_FORCE_INLINE unordered_multimap(const unordered_multimap& rhs)
            : base_type(rhs) {}
        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        AZ_FORCE_INLINE unordered_multimap(const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}
        AZ_FORCE_INLINE unordered_multimap(size_type numBuckets)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBuckets);
        }
        AZ_FORCE_INLINE unordered_multimap(size_type numBuckets, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBuckets);
        }
        AZ_FORCE_INLINE unordered_multimap(size_type numBuckets, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multimap(Iterator first, Iterator last)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multimap(Iterator first, Iterator last, size_type numBuckets)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multimap(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multimap(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        AZ_FORCE_INLINE unordered_multimap(const std::initializer_list<value_type>& list, const hasher& hash = hasher(), const key_eq& keyEqual = key_eq(), const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }

        AZ_FORCE_INLINE unordered_multimap(this_type&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            base_type::operator=(AZStd::move(rhs));
            return *this;
        }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            base_type::operator=(rhs);
            return *this;
        }

        AZ_FORCE_INLINE iterator insert(const value_type& value)
        {
            return base_type::insert_impl(value).first;
        }

        iterator insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_multimap::insert(node_type&& nodeHandle)");
            using insert_return_type = insert_return_type<iterator, node_type>;
            return base_type::template node_handle_insert<insert_return_type>(AZStd::move(nodeHandle)).position;
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_multimap::insert(const_iterator hint, node_type nodeHandle)");
            return base_type::node_handle_insert(hint, AZStd::move(nodeHandle));
        }

        node_type extract(const key_type& key)
        {
            return base_type::template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return base_type::template node_handle_extract<node_type>(it);
        }

        /**
        * Insert a pair with default value base on a key only (AKA lazy insert). This can be a speed up when
        * the object has complicated assignment function.
        */
        AZ_FORCE_INLINE pair_iter_bool insert_key(const key_type& key)
        {
            Internal::ConvertKeyType<key_type> converter;
            return base_type::insert_from(key, converter, base_type::m_hasher, base_type::m_keyEqual);
        }
    };

    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& left, unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }

    template <class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator==(const unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& a, const unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& b)
    {
        return (a.size() == b.size() && equal(a.begin(), a.end(), b.begin()));
    }

    template <class Key, class MappedType, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator!=(const unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& a, const unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& b)
    {
        return !(a == b);
    }

    template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator, class Predicate>
    decltype(auto) erase_if(unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>& container, Predicate predicate)
    {
        auto originalSize = container.size();

        for (auto iter = container.begin(); iter != container.end();)
        {
            if (predicate(*iter))
            {
                iter = container.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return originalSize - container.size();
    }
}

#endif // AZSTD_UNORDERED_MAP_H
#pragma once
