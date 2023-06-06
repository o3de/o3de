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
        template<class Key, class Hasher, class EqualKey, class Allocator, bool isMultiSet>
        struct UnorderedSetTableTraits
        {
            using key_type = Key;
            using key_equal = EqualKey;
            using hasher = Hasher;
            using value_type = Key;
            using allocator_type = Allocator;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiSet,

                // This values are NOT used for dynamic maps
                is_dynamic = true,
                fixed_num_buckets = 1,
                fixed_num_elements = 4
            };
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    }

    /**
    * Unordered set container is complaint with \ref CTR1 (6.2.4.3)
    * This is an associative container, all Keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    * It has the extensions in the \ref hash_table class.
    *
    * Check the unordered_set \ref AZStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator>
    class unordered_set
        : public hash_table< Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, false> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = unordered_set<Key, Hasher, EqualKey, Allocator>;
        using base_type = hash_table<Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, false>>;
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
        using insert_return_type = AssociativeInternal::insert_return_type<iterator, node_type>;

        unordered_set()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        explicit unordered_set(size_type numBuckets,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        unordered_set(Iterator first, Iterator last, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_set(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }
        unordered_set(const unordered_set& rhs)
            : base_type(rhs) {}
        unordered_set(unordered_set&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }
        explicit unordered_set(const allocator_type& alloc)
            : base_type(hasher(), key_equal(), alloc) {}
        unordered_set(const unordered_set& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(rhs, alloc) {}
        unordered_set(unordered_set&& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(AZStd::move(rhs), alloc) {}


        unordered_set(const initializer_list<value_type>& list, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }
        unordered_set(size_type numBucketsHint, const allocator_type& alloc)
            : unordered_set(numBucketsHint, hasher(), key_equal(), alloc)
        {
        }
        unordered_set(size_type numBucketsHint, const hasher& hash, const allocator_type& alloc)
            : unordered_set(numBucketsHint, hash, key_equal(), alloc)
        {
        }
        template<class InputIterator>
        unordered_set(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
            : unordered_set(f, l, n, hasher(), key_equal(), a)
        {
        }
        template<class InputIterator>
        unordered_set(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_set(f, l, n, hf, key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_set(from_range_t, R&& rg, size_type n, const allocator_type& a)
            : unordered_set(from_range, AZStd::forward<R>(rg), n, hasher(), key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_set(from_range_t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_set(from_range, AZStd::forward<R>(rg), n, hf, key_equal(), a)
        {
        }
        unordered_set(initializer_list<value_type> il, size_type n, const allocator_type& a)
            : unordered_set(il, n, hasher(), key_equal(), a)
        {
        }
        unordered_set(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_set(il, n, hf, key_equal(), a)
        {
        }

        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        unordered_set(const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}

        this_type& operator=(this_type&& rhs)
        {
            base_type::operator=(AZStd::move(rhs));
            return *this;
        }

        this_type& operator=(const this_type& rhs)
        {
            base_type::operator=(rhs);
            return *this;
        }

        using base_type::insert;
        // Bring hash_table::insert_range function into scope
        using base_type::insert_range;
        insert_return_type insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_set::insert(node_type&& nodeHandle)");
            return base_type::template node_handle_insert<insert_return_type>(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_set::insert(const_iterator hint, node_type&& nodeHandle)");
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
    };

    template<class Key, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(unordered_set<Key, Hasher, EqualKey, Allocator>& left, unordered_set<Key, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }

    template <class Key, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator==(const unordered_set<Key, Hasher, EqualKey, Allocator>& a, const unordered_set<Key, Hasher, EqualKey, Allocator>& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }

        for (decltype(a.begin()) ait = a.begin(); ait != a.end(); ++ait)
        {
            if (b.find(*ait) == b.end())
            {
                return false;
            }
        }
        return true;
    }

    template <class Key, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator!=(const unordered_set<Key, Hasher, EqualKey, Allocator>& a, const unordered_set<Key, Hasher, EqualKey, Allocator>& b)
    {
        return !(a == b);
    }

    template<class Key, class Hasher, class EqualKey, class Allocator, class Predicate>
    decltype(auto) erase_if(unordered_set<Key, Hasher, EqualKey, Allocator>& container, Predicate predicate)
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
        class Hash = hash<iter_value_type<InputIterator>>,
        class Pred = equal_to<iter_value_type<InputIterator>>,
        class Allocator = allocator>
        unordered_set(InputIterator, InputIterator,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_set<iter_value_type<InputIterator>,
        Hash, Pred, Allocator>;

    template<class R,
        class Hash = hash<ranges::range_value_t<R>>,
        class Pred = equal_to<ranges::range_value_t<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_set(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        -> unordered_set<ranges::range_value_t<R>, Hash, Pred, Allocator>;

    template<class T, class Hash = hash<T>,
        class Pred = equal_to<T>, class Allocator = allocator>
        unordered_set(initializer_list<T>,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_set<T, Hash, Pred, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_set(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        ->unordered_set<iter_value_type<InputIterator>,
        hash<iter_value_type<InputIterator>>,
        equal_to<iter_value_type<InputIterator>>,
        Allocator>;

    template<class InputIterator, class Hash, class Allocator>
    unordered_set(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_set<iter_value_type<InputIterator>, Hash,
        equal_to<iter_value_type<InputIterator>>,
        Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_set(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        -> unordered_set<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_set(from_range_t, R&&, Allocator)
        -> unordered_set<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>, Allocator>;

    template<class R,
        class Hash,
        class Allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_set(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator = Allocator())
        -> unordered_set<ranges::range_value_t<R>, Hash, equal_to<ranges::range_value_t<R>>, Allocator>;

    template<class T, class Allocator>
    unordered_set(initializer_list<T>,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        ->unordered_set<T, hash<T>, equal_to<T>, Allocator>;

    template<class T, class Hash, class Allocator>
    unordered_set(initializer_list<T>,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_set<T, Hash, equal_to<T>, Allocator>;

    /**
    * Unordered multi set container is complaint with \ref CTR1 (6.2.4.5)
    * The only difference from the unordered_set is that we allow multiple entries with
    * the same key. You can iterate over them, by checking if the key stays the same.
    *
    * Check the unordered_multiset \ref AZStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = AZStd::allocator>
    class unordered_multiset
        : public hash_table< Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, true> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = unordered_multiset<Key, Hasher, EqualKey, Allocator>;
        using base_type = hash_table<Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, true>>;
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

        unordered_multiset()
            : base_type(hasher(), key_equal(), allocator_type()) {}
        explicit unordered_multiset(size_type numBuckets,
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        unordered_multiset(Iterator first, Iterator last, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
            base_type::insert(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multiset(from_range_t, R&& rg, size_type numBucketsHint = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBucketsHint);
            base_type::insert_range(AZStd::forward<R>(rg));
        }
        unordered_multiset(const unordered_multiset& rhs)
            : base_type(rhs) {}
        unordered_multiset(unordered_multiset&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }
        explicit unordered_multiset(const allocator_type& alloc)
            : base_type(hasher(), key_equal(), alloc) {}
        unordered_multiset(const unordered_multiset& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(rhs, alloc) {}
        unordered_multiset(unordered_multiset&& rhs, const type_identity_t<allocator_type>& alloc)
            : base_type(AZStd::move(rhs), alloc) {}


        unordered_multiset(const initializer_list<value_type>& list, size_type numBuckets = {},
            const hasher& hash = hasher(), const key_equal& keyEqual = key_equal(),
            const allocator_type& alloc = allocator_type())
            : base_type(hash, keyEqual, alloc)
        {
            base_type::rehash(numBuckets);
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }
        unordered_multiset(size_type numBucketsHint, const allocator_type& alloc)
            : unordered_multiset(numBucketsHint, hasher(), key_equal(), alloc)
        {
        }
        unordered_multiset(size_type numBucketsHint, const hasher& hash, const allocator_type& alloc)
            : unordered_multiset(numBucketsHint, hash, key_equal(), alloc)
        {
        }
        template<class InputIterator>
        unordered_multiset(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
            : unordered_multiset(f, l, n, hasher(), key_equal(), a)
        {
        }
        template<class InputIterator>
        unordered_multiset(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multiset(f, l, n, hf, key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multiset(from_range_t, R&& rg, size_type n, const allocator_type& a)
            : unordered_multiset(from_range, AZStd::forward<R>(rg), n, hasher(), key_equal(), a)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        unordered_multiset(from_range_t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multiset(from_range, AZStd::forward<R>(rg), n, hf, key_equal(), a)
        {
        }
        unordered_multiset(initializer_list<value_type> il, size_type n, const allocator_type& a)
            : unordered_multiset(il, n, hasher(), key_equal(), a)
        {
        }
        unordered_multiset(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
            : unordered_multiset(il, n, hf, key_equal(), a)
        {
        }

        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        unordered_multiset(const hasher& hash, const key_equal& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}

        this_type& operator=(this_type&& rhs)
        {
            base_type::operator=(AZStd::move(rhs));
            return *this;
        }

        using base_type::insert;
        // Bring hash_table::insert_range function into scope
        using base_type::insert_range;
        iterator insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_multiset::insert(node_type&& nodeHandle)");
            using insert_return_type = insert_return_type<iterator, node_type>;
            return base_type::template node_handle_insert<insert_return_type>(AZStd::move(nodeHandle)).position;
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == base_type::get_allocator(),
                "node_type with incompatible allocator passed to unordered_multiset::insert(const_iterator hint, node_type&& nodeHandle)");
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
    };

    template<class Key, class Hasher, class EqualKey, class Allocator >
    AZ_FORCE_INLINE void swap(unordered_multiset<Key, Hasher, EqualKey, Allocator>& left, unordered_multiset<Key, Hasher, EqualKey, Allocator>& right)
    {
        left.swap(right);
    }

    template <class Key, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator==(const unordered_multiset<Key, Hasher, EqualKey, Allocator>& a, const unordered_multiset<Key, Hasher, EqualKey, Allocator>& b)
    {
        return (a.size() == b.size() && AZStd::equal(a.begin(), a.end(), b.begin(), b.end()));
    }

    template <class Key, class Hasher, class EqualKey, class Allocator>
    AZ_FORCE_INLINE bool operator!=(const unordered_multiset<Key, Hasher, EqualKey, Allocator>& a, const unordered_multiset<Key, Hasher, EqualKey, Allocator>& b)
    {
        return !(a == b);
    }

    template<class Key, class Hasher, class EqualKey, class Allocator, class Predicate>
    decltype(auto) erase_if(unordered_multiset<Key, Hasher, EqualKey, Allocator>& container, Predicate predicate)
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
        class Hash = hash<iter_value_type<InputIterator>>,
        class Pred = equal_to<iter_value_type<InputIterator>>,
        class Allocator = allocator>
        unordered_multiset(InputIterator, InputIterator,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_multiset<iter_value_type<InputIterator>,
        Hash, Pred, Allocator>;

    template<class R,
        class Hash = hash<ranges::range_value_t<R>>,
        class Pred = equal_to<ranges::range_value_t<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multiset(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type = {},
        Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        -> unordered_multiset<ranges::range_value_t<R>, Hash, Pred, Allocator>;

    template<class T, class Hash = hash<T>,
        class Pred = equal_to<T>, class Allocator = allocator>
        unordered_multiset(initializer_list<T>,
            typename allocator_traits<Allocator>::size_type = {},
            Hash = Hash(), Pred = Pred(), Allocator = Allocator())
        ->unordered_multiset<T, Hash, Pred, Allocator>;

    template<class InputIterator, class Allocator>
    unordered_multiset(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type, Allocator)
        ->unordered_multiset<iter_value_type<InputIterator>,
        hash<iter_value_type<InputIterator>>,
        equal_to<iter_value_type<InputIterator>>,
        Allocator>;

    template<class InputIterator, class Hash, class Allocator>
    unordered_multiset(InputIterator, InputIterator,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_multiset<iter_value_type<InputIterator>, Hash,
        equal_to<iter_value_type<InputIterator>>,
        Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multiset(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        -> unordered_multiset<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>, Allocator>;

    template<class R,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multiset(from_range_t, R&&, Allocator)
        -> unordered_multiset<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>, Allocator>;

    template<class R,
        class Hash,
        class Allocator,
        class = enable_if_t<ranges::input_range<R>>>
    unordered_multiset(from_range_t, R&&,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator = Allocator())
        ->unordered_multiset<ranges::range_value_t<R>, Hash, equal_to<ranges::range_value_t<R>>, Allocator>;
    template<class T, class Allocator>
    unordered_multiset(initializer_list<T>,
        typename allocator_traits<Allocator>::size_type,
        Allocator)
        -> unordered_multiset<T, hash<T>, equal_to<T>, Allocator>;

    template<class T, class Hash, class Allocator>
    unordered_multiset(initializer_list<T>,
        typename allocator_traits<Allocator>::size_type,
        Hash, Allocator)
        ->unordered_multiset<T, Hash, equal_to<T>, Allocator>;
}
