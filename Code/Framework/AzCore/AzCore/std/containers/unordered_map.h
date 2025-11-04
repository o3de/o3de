/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/hash_table.h>
#include <AzCore/std/typetraits/type_identity.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class Hasher, class EqualKey, class Allocator, bool isMultiMap>
        struct UnorderedMapTableTraits
        {
            using key_type = Key;
            using key_equal = EqualKey;
            using hasher = Hasher;
            using value_type = AZStd::pair<Key, MappedType>;
            using allocator_type = Allocator;
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
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value) { return value.first; }
        };

        /**
         * Used when we want to insert entry with a key only, default construct for the
         * value. This rely on that AZStd::pair (map value type) can be constructed with a key only (first element).
         */
        template<class KeyType>
        struct ConvertKeyType
        {
            using key_type = KeyType;

            AZ_FORCE_INLINE const KeyType& to_key(const KeyType& key) const { return key; }
            // We return key as the value so the pair is constructed using Pair(first) ctor.
            AZ_FORCE_INLINE const KeyType& to_value(const KeyType& key) const { return key; }
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

        using this_type = unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>;
        using base_type = hash_table<Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, false>>;
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

        unordered_map()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        explicit unordered_map(size_type numBucketsHint,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class InputIterator>
        unordered_map(InputIterator first, InputIterator last, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_map(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }

        unordered_map(const unordered_map& rhs)
            : base_type(rhs) {}
        unordered_map(unordered_map&& rhs)
            : base_type(AZStd::move(rhs)) {}

        explicit unordered_map(const allocator_type& alloc)
            : base_type(hasher(), key_equal(), alloc) {}
        unordered_map(const unordered_map& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(rhs, alloc) {}
        unordered_map(unordered_map&& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(AZStd::move(rhs), alloc) {}

        unordered_map(initializer_list<value_type> list, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(list);
        }
        unordered_map(size_type numBucketsHint, const allocator_type& alloc)
            : unordered_map(numBucketsHint, hasher(), key_equal(), alloc)
        {
        }
        unordered_map(size_type numBucketsHint, const hasher& hash, const allocator_type& alloc)
            : unordered_map(numBucketsHint, hash, key_equal(), alloc)
        {
        }
        template<class InputIterator>
        unordered_map(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
            : unordered_map(f, l, n, hasher(), key_equal(), a)
        {
        }
        template<class InputIterator>
        unordered_map(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_map(f, l, n, hf, key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_map(from_range_t, R&& rg, size_type n, const allocator_type& a)
            : unordered_map(from_range, AZStd::forward<R>(rg), n, hasher(), key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_map(from_range_t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_map(from_range, AZStd::forward<R>(rg), n, hf, key_equal(), a)
        {
        }
        unordered_map(initializer_list<value_type> il, size_type n, const allocator_type& a)
            : unordered_map(il, n, hasher(), key_equal(), a)
        {
        }
        unordered_map(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_map(il, n, hf, key_equal(), a)
        {
        }

        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        unordered_map(const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}

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
        // Bring hash_table::insert_range function into scope
        using base_type::insert_range;
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
        return (a.size() == b.size() && AZStd::equal(a.begin(), a.end(), b.begin(), b.end()));
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

    // deduction guides
    template<class InputIterator,
        class Hash = hash<iter_key_type<InputIterator>>,
        class Pred = equal_to<iter_key_type<InputIterator>>,
        class Allocator = allocator>
        unordered_map(InputIterator, InputIterator,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>, Hash, Pred,
        Allocator>;

    template<class R,
        class Hash = hash<range_key_type<R>>,
        class Pred = equal_to<range_key_type<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_map(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_map<range_key_type<R>, range_mapped_type<R>, Hash, Pred, Allocator>;

    template<class Key, class T, class Hash = hash<Key>,
        class Pred = equal_to<Key>, class Allocator = allocator>
        unordered_map(initializer_list<pair<Key, T>>,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(),
            Pred = Pred(), Allocator = Allocator())
        ->unordered_map<Key, T, Hash, Pred, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_map(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        ->unordered_map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        hash<iter_key_type<InputIterator>>,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_map(InputIterator, InputIterator, Allocator)
        ->unordered_map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        hash<iter_key_type<InputIterator>>,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class InputIterator, class Hash, class Allocator>
    unordered_map(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>, Hash,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_map(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        -> unordered_map<range_key_type<R>, range_mapped_type<R>, hash<range_key_type<R>>, equal_to<range_key_type<R>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_map(from_range_t, R&&, Allocator)
        -> unordered_map<range_key_type<R>, range_mapped_type<R>, hash<range_key_type<R>>, equal_to<range_key_type<R>>, Allocator>;

    template<class R,
        class Hash,
        class Allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_map(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator = Allocator())
        -> unordered_map<range_key_type<R>, range_mapped_type<R>, Hash, equal_to<range_key_type<R>>, Allocator>;

    template<class Key, class T, class Allocator>
    unordered_map(initializer_list<pair<Key, T>>, typename allocator_traits<Allocator>::size_type, Allocator)
        ->unordered_map<Key, T, hash<Key>, equal_to<Key>, Allocator>;

    template<class Key, class T, class Allocator>
    unordered_map(initializer_list<pair<Key, T>>, Allocator)
        ->unordered_map<Key, T, hash<Key>, equal_to<Key>, Allocator>;

    template<class Key, class T, class Hash, class Allocator>
    unordered_map(initializer_list<pair<Key, T>>, typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_map<Key, T, Hash, equal_to<Key>, Allocator>;


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

        using this_type = unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>;
        using base_type = hash_table<Internal::UnorderedMapTableTraits<Key, MappedType, Hasher, EqualKey, Allocator, true>>;
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

        unordered_multimap()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        explicit unordered_multimap(size_type numBucketsHint,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class InputIterator>
        unordered_multimap(InputIterator first, InputIterator last, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multimap(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }

        unordered_multimap(const unordered_multimap& rhs)
            : base_type(rhs) {}
        unordered_multimap(unordered_multimap&& rhs)
            : base_type(AZStd::move(rhs)) {}

        explicit unordered_multimap(const allocator_type& alloc)
            : base_type(hasher(), key_equal(), alloc) {}
        unordered_multimap(const unordered_multimap& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(rhs, alloc) {}
        unordered_multimap(unordered_multimap&& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(AZStd::move(rhs), alloc) {}

        unordered_multimap(initializer_list<value_type> list, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert(list);
        }
        unordered_multimap(size_type numBucketsHint, const allocator_type& alloc)
            : unordered_multimap(numBucketsHint, hasher(), key_equal(), alloc)
        {
        }
        unordered_multimap(size_type numBucketsHint, const hasher& hash, const allocator_type& alloc)
            : unordered_multimap(numBucketsHint, hash, key_equal(), alloc)
        {
        }
        template<class InputIterator>
        unordered_multimap(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
            : unordered_multimap(f, l, n, hasher(), key_equal(), a)
        {
        }
        template<class InputIterator>
        unordered_multimap(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multimap(f, l, n, hf, key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multimap(from_range_t, R&& rg, size_type n, const allocator_type& a)
            : unordered_multimap(from_range, AZStd::forward<R>(rg), n, hasher(), key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multimap(from_range_t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multimap(from_range, AZStd::forward<R>(rg), n, hf, key_equal(), a)
        {
        }
        unordered_multimap(initializer_list<value_type> il, size_type n, const allocator_type& a)
            : unordered_multimap(il, n, hasher(), key_equal(), a)
        {
        }
        unordered_multimap(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multimap(il, n, hf, key_equal(), a)
        {
        }

        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        unordered_multimap(const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}

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

        using base_type::insert;
        // Bring hash_table::insert_range function into scope
        using base_type::insert_range;
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
        return (a.size() == b.size() && AZStd::equal(a.begin(), a.end(), b.begin(), b.end()));
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

    // deduction guides
    template<class InputIterator,
        class Hash = hash<iter_key_type<InputIterator>>,
        class Pred = equal_to<iter_key_type<InputIterator>>,
        class Allocator = allocator>
    unordered_multimap(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        Hash, Pred, Allocator>;

    template<class R,
        class Hash = hash<range_key_type<R>>,
        class Pred = equal_to<range_key_type<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multimap(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_multimap<range_key_type<R>, range_mapped_type<R>, Hash, Pred, Allocator>;

    template<class Key, class T, class Hash = hash<Key>,
        class Pred = equal_to<Key>, class Allocator = allocator>
    unordered_multimap(initializer_list<pair<Key, T>>,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_multimap<Key, T, Hash, Pred, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_multimap(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        ->unordered_multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        hash<iter_key_type<InputIterator>>,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_multimap(InputIterator, InputIterator, Allocator)
        ->unordered_multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        hash<iter_key_type<InputIterator>>,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class InputIterator, class Hash, class Allocator>
    unordered_multimap(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>, Hash,
        equal_to<iter_key_type<InputIterator>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multimap(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        ->unordered_multimap<range_key_type<R>, range_mapped_type<R>, hash<range_key_type<R>>, equal_to<range_key_type<R>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multimap(from_range_t, R&&, Allocator)
        ->unordered_multimap<range_key_type<R>, range_mapped_type<R>, hash<range_key_type<R>>, equal_to<range_key_type<R>>, Allocator>;

    template<class R,
        class Hash,
        class Allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multimap(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator = Allocator())
        ->unordered_multimap<range_key_type<R>, range_mapped_type<R>, Hash, equal_to<range_key_type<R>>, Allocator>;

    template<class Key, class T, class Allocator>
    unordered_multimap(initializer_list<pair<Key, T>>, typename allocator_traits<Allocator>::size_type, Allocator)
        ->unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Allocator>;

    template<class Key, class T, class Allocator>
    unordered_multimap(initializer_list<pair<Key, T>>, Allocator)
        ->unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Allocator>;

    template<class Key, class T, class Hash, class Allocator>
    unordered_multimap(initializer_list<pair<Key, T>>, typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_multimap<Key, T, Hash, equal_to<Key>, Allocator>;
}

