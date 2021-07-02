/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_UNORDERED_SET_H
#define AZSTD_UNORDERED_SET_H 1

#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/hash_table.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class Hasher, class EqualKey, class Allocator, bool isMultiSet>
        struct UnorderedSetTableTraits
        {
            typedef Key         key_type;
            typedef EqualKey    key_eq;
            typedef Hasher      hasher;
            typedef Key         value_type;
            typedef Allocator   allocator_type;
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

        typedef unordered_set<Key, Hasher, EqualKey, Allocator> this_type;
        typedef hash_table< Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, false> > base_type;
    public:
        typedef typename base_type::traits_type traits_type;

        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;

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

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;
        using insert_return_type = insert_return_type<iterator, node_type>;

        AZ_FORCE_INLINE unordered_set()
            : base_type(hasher(), key_eq(), allocator_type()) {}
        explicit unordered_set(const allocator_type& alloc)
            : base_type(hasher(), key_eq(), alloc) {}
        AZ_FORCE_INLINE unordered_set(const unordered_set& rhs)
            : base_type(rhs) {}
        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        AZ_FORCE_INLINE unordered_set(const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}
        AZ_FORCE_INLINE unordered_set(size_type numBucketsHint)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE unordered_set(size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBucketsHint);
        }
        AZ_FORCE_INLINE unordered_set(size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_set(Iterator first, Iterator last)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_set(Iterator first, Iterator last, size_type numBucketsHint)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_set(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_set(Iterator first, Iterator last, size_type numBucketsHint, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBucketsHint);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        AZ_FORCE_INLINE unordered_set(const std::initializer_list<value_type>& list, const hasher& hash = hasher(), const key_eq& keyEqual = key_eq(), const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }

        AZ_FORCE_INLINE unordered_set(this_type&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }

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

        typedef unordered_multiset<Key, Hasher, EqualKey, Allocator> this_type;
        typedef hash_table< Internal::UnorderedSetTableTraits<Key, Hasher, EqualKey, Allocator, true> > base_type;
    public:
        typedef typename base_type::traits_type traits_type;

        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;

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

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename base_type::list_node_type, typename base_type::node_deleter>>;

        AZ_FORCE_INLINE unordered_multiset()
            : base_type(hasher(), key_eq(), allocator_type()) {}
        explicit unordered_multiset(const allocator_type& alloc)
            : base_type(hasher(), key_eq(), alloc) {}
        AZ_FORCE_INLINE unordered_multiset(const unordered_multiset& rhs)
            : base_type(rhs) {}
        /// This constructor is AZStd extension (so we don't rehash/allocate memory)
        AZ_FORCE_INLINE unordered_multiset(const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator) {}
        AZ_FORCE_INLINE unordered_multiset(size_type numBuckets)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBuckets);
        }
        AZ_FORCE_INLINE unordered_multiset(size_type numBuckets, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBuckets);
        }
        AZ_FORCE_INLINE unordered_multiset(size_type numBuckets, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBuckets);
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multiset(Iterator first, Iterator last)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multiset(Iterator first, Iterator last, size_type numBuckets)
            : base_type(hasher(), key_eq(), allocator_type())
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multiset(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual, allocator_type())
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        AZ_FORCE_INLINE unordered_multiset(Iterator first, Iterator last, size_type numBuckets, const hasher& hash, const key_eq& keyEqual, const allocator_type& allocator)
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(numBuckets);
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        AZ_FORCE_INLINE unordered_multiset(const std::initializer_list<value_type>& list, const hasher& hash = hasher(), const key_eq& keyEqual = key_eq(), const allocator_type& allocator = allocator_type())
            : base_type(hash, keyEqual, allocator)
        {
            base_type::rehash(list.size());
            for (const value_type& i : list)
            {
                base_type::insert(i);
            }
        }
        AZ_FORCE_INLINE unordered_multiset(this_type&& rhs)
            : base_type(AZStd::move(rhs))
        {
        }

        this_type& operator=(this_type&& rhs)
        {
            base_type::operator=(AZStd::move(rhs));
            return *this;
        }

        using base_type::insert;
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
        return (a.size() == b.size() && equal(a.begin(), a.end(), b.begin()));
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

}

#endif // AZSTD_UNORDERED_SET_H
#pragma once
