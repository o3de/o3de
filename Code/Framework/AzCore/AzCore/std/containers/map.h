/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/containers_concepts.h>
#include <AzCore/std/containers/rbtree.h>
#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/typetraits/type_identity.h>

namespace AZStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class KeyEq, class Allocator >
        struct OrderedMapRbTreeTraits
        {
            using key_type = Key;
            using key_equal = KeyEq;
            using value_type = AZStd::pair<Key, MappedType>;
            using allocator_type = Allocator;
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };
    }

    /**
    * Ordered map container is complaint with \ref C++0x (23.4.1)
    * This is an associative container, all keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class MappedType, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class map
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = map<Key, MappedType, Compare, Allocator>;
        using tree_type = rbtree<Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator>>;

    public:
        using traits_type = typename tree_type::traits_type;

        using key_type = typename tree_type::key_type;
        using value_type = typename tree_type::value_type;
        using key_compare = typename tree_type::key_equal;
        using mapped_type = MappedType;
        class value_compare
        {
        protected:
            Compare m_comp;
            AZ_FORCE_INLINE value_compare(Compare c)
                : m_comp(AZStd::move(c)) {}
        public:
            AZ_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        using allocator_type = typename tree_type::allocator_type;
        using size_type = typename tree_type::size_type;
        using difference_type = typename tree_type::difference_type;
        using pointer = typename tree_type::pointer;
        using const_pointer = typename tree_type::const_pointer;
        using reference = typename tree_type::reference;
        using const_reference = typename tree_type::const_reference;

        using iterator = typename tree_type::iterator;
        using const_iterator = typename tree_type::const_iterator;

        using reverse_iterator = typename tree_type::reverse_iterator;
        using const_reverse_iterator = typename tree_type::const_reverse_iterator;
        
        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;
        using insert_return_type = AssociativeInternal::insert_return_type<iterator, node_type>;

        map() = default;
        explicit map(const Compare& comp, const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        template <class InputIterator>
        map(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        map(from_range_t, R&& rg, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                m_tree.insert_unique(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                m_tree.insert_unique(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }

        map(const map& rhs)
            : m_tree(rhs.m_tree) {}
        map(map&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}

        explicit map(const Allocator& alloc)
            : m_tree(alloc) {}
        map(const map& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(rhs.m_tree, alloc) {}
        map(map&& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(AZStd::move(rhs.m_tree), alloc) {}

        map(initializer_list<value_type> list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : map(list.begin(), list.end(), comp, alloc)
        {
        }

        template <class InputIterator>
        map(InputIterator first, InputIterator last, const Allocator& alloc)
            : map(first, last, Compare(), alloc)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        map(from_range_t, R&& rg, const Allocator& a)
            : map(from_range, AZStd::forward<R>(rg), Compare(), a)
        {
        }
        map(initializer_list<value_type> il, const Allocator& a)
            : map(il, Compare(), a)
        {
        }

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const { return value_comp(m_tree.key_comp()); }

        AZ_FORCE_INLINE iterator begin() { return m_tree.begin(); }
        AZ_FORCE_INLINE iterator end() { return m_tree.end(); }
        AZ_FORCE_INLINE const_iterator begin() const { return m_tree.begin(); }
        AZ_FORCE_INLINE const_iterator end() const { return m_tree.end(); }
        AZ_FORCE_INLINE reverse_iterator rbegin() { return m_tree.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator rend() { return m_tree.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        AZ_FORCE_INLINE bool empty() const { return m_tree.empty(); }
        AZ_FORCE_INLINE size_type size() const { return m_tree.size(); }
        AZ_FORCE_INLINE size_type max_size() const { return m_tree.max_size(); }
        MappedType& operator[](const key_type& key)
        {
            iterator iter = m_tree.lower_bound(key);
            if (iter == end() || key_comp()(key, (*iter).first))
            {
                iter = insert(iter, value_type(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::move(key)), AZStd::tuple<>{}));
            }
            return (*iter).second;
        }
        MappedType& operator[](key_type&& key)
        {
            iterator iter = m_tree.lower_bound(key);
            if (iter == end() || key_comp()(key, (*iter).first))
            {
                iter = insert(iter, value_type(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::move(key)), AZStd::tuple<>{}));
            }
            return (*iter).second;
        }
        AZ_FORCE_INLINE void swap(this_type& rhs) { m_tree.swap(rhs.m_tree); }

        // insert/erase
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value) { return m_tree.insert_unique(value); }
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value) { return m_tree.insert_unique(insertPos, value); }
        template <class InputIterator>
        void insert(InputIterator first, InputIterator last)
        {
            m_tree.insert_unique(first, last);
        }
        template<class R>
        auto insert_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>>
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                m_tree.insert_unique(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                m_tree.insert_unique(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }
        AZ_FORCE_INLINE iterator erase(const_iterator erasePos) { return m_tree.erase(erasePos); }
        AZ_FORCE_INLINE size_type erase(const key_type& key) { return m_tree.erase_unique(key); }
        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last) { return m_tree.erase(first, last); }
        AZ_FORCE_INLINE void clear() { m_tree.clear(); }

        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        AZStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_unique(AZStd::forward<value_type>(value));
        }
        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_unique(insertPos, AZStd::forward<value_type>(value));
        }
        template<class ... InputArguments>
        AZStd::pair<iterator, bool> emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(AZStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(insertPos, AZStd::forward<InputArguments>(arguments) ...);
        }

        insert_return_type insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to map::insert(node_type&& nodeHandle)");
            return m_tree.template node_handle_insert_unique<insert_return_type>(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to map::insert(const_iterator hint, node_type&& node_handle)");
            return m_tree.node_handle_insert_unique(hint, AZStd::move(nodeHandle));
        }

        //! C++17 insert_or_assign function assigns the element to the mapped_type if the key exist in the container
        //! Otherwise a new value is inserted into the container
        template <typename M>
        pair<iterator, bool> insert_or_assign(const key_type& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(key, AZStd::forward<M>(value));
        }
        template <typename M>
        pair<iterator, bool> insert_or_assign(key_type&& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(AZStd::move(key), AZStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, const key_type& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(hint, key, AZStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, key_type&& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(hint, AZStd::move(key), AZStd::forward<M>(value));
        }

        //! C++17 try_emplace function that does nothing to the arguments if the key exist in the container,
        //! otherwise it constructs the value type as if invoking
        //! value_type(AZStd::piecewise_construct, AZStd::forward_as_tuple(AZStd::forward<KeyType>(key)),
        //!  AZStd::forward_as_tuple(AZStd::forward<Args>(args)...))
        template <typename... Args>
        pair<iterator, bool> try_emplace(const key_type& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(key, AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        pair<iterator, bool> try_emplace(key_type&& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(AZStd::move(key), AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, const key_type& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(hint, key, AZStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, key_type&& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(hint, AZStd::move(key), AZStd::forward<Args>(arguments)...);
        }

        node_type extract(const key_type& key)
        {
            return m_tree.template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return m_tree.template node_handle_extract<node_type>(it);
        }

        // set operations:
        const_iterator find(const key_type& key) const { return m_tree.find(key); }
        iterator find(const key_type& key) { return m_tree.find(key); }
        bool contains(const key_type& key) const { return m_tree.contains(key); }
        size_type count(const key_type& key) const { return m_tree.find(key) == m_tree.end() ? 0 : 1; }
        iterator lower_bound(const key_type& key) { return m_tree.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const { return m_tree.lower_bound(key); }
        iterator upper_bound(const key_type& key) { return m_tree.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const { return m_tree.upper_bound(key); }
        pair<iterator, iterator> equal_range(const key_type& key) { return m_tree.equal_range_unique(key); }
        pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range_unique(key); }

        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.find(key) == m_tree.end() ? 0 : 1; }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range_unique(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range_unique(key); }

        /**
        * \anchor MapExtensions
        * \name Extensions
        * @{
        */
        /**
         * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
         * this is here only to make sure it's available to the end user.
         */
        AZ_FORCE_INLINE pair<iterator, bool> insert(const key_type& key)
        {
            if (m_tree.find(key) != m_tree.end())
            {
                return AZStd::make_pair(m_tree.end(), false);
            }
            return m_tree.insert_unique(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type& get_allocator() { return m_tree.get_allocator(); }
        AZ_FORCE_INLINE const allocator_type& get_allocator() const { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        AZ_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        AZ_INLINE bool      validate() const { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        AZ_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        AZ_INLINE int       validate_iterator(const iterator& iter) const { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        AZ_FORCE_INLINE void                        leak_and_reset() { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(map<Key, MappedType, Compare, Allocator>& left, map<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
            && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class MappedType, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(map<Key, MappedType, Compare, Allocator>& container, Predicate predicate)
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
    template<class InputIterator, class Compare = less<iter_key_type<InputIterator>>,
        class Allocator = allocator>
    map(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
        ->map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>, Compare, Allocator>;

    template<class R, class Compare = less<range_key_type<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    map(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
        ->map<range_key_type<R>, range_mapped_type<R>, Compare, Allocator>;

    template<class Key, class T, class Compare = less<Key>,
        class Allocator = allocator>
    map(initializer_list<pair<Key, T>>, Compare = Compare(), Allocator = Allocator())
        ->map<Key, T, Compare, Allocator>;

    template<class InputIterator, class Allocator>
    map(InputIterator, InputIterator, Allocator)
        ->map<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        less<iter_key_type<InputIterator>>, Allocator>;

    template<class R, class Allocator, class = enable_if_t<ranges::input_range<R>>>
    map(from_range_t, R&&, Allocator)
        ->map<range_key_type<R>, range_mapped_type<R>, less<range_key_type<R>>, Allocator>;

    template<class Key, class T, class Allocator>
    map(initializer_list<pair<Key, T>>, Allocator)->map<Key, T, less<Key>, Allocator>;

    /**
    * Ordered multimap container is complaint with \ref C++0x (23.4.2)
    * This is an associative container, key can be equivalent (multiple copies of the same key value).
    * You can iterate over them, by checking if the key stays the same, after finding the first.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class MappedType, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class multimap
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = multimap<Key, MappedType, Compare, Allocator>;
        using tree_type = rbtree<Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator>>;
    public:
        using traits_type = typename tree_type::traits_type;

        using key_type = typename tree_type::key_type;
        using value_type = typename tree_type::value_type;
        using key_compare = typename tree_type::key_equal;
        using mapped_type = MappedType;
        class value_compare
        {
        protected:
            Compare m_comp;
            AZ_FORCE_INLINE value_compare(Compare c)
                : m_comp(AZStd::move(c)) {}
        public:
            AZ_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        using allocator_type = typename tree_type::allocator_type;
        using size_type = typename tree_type::size_type;
        using difference_type = typename tree_type::difference_type;
        using pointer = typename tree_type::pointer;
        using const_pointer = typename tree_type::const_pointer;
        using reference = typename tree_type::reference;
        using const_reference = typename tree_type::const_reference;

        using iterator = typename tree_type::iterator;
        using const_iterator = typename tree_type::const_iterator;

        using reverse_iterator = typename tree_type::reverse_iterator;
        using const_reverse_iterator = typename tree_type::const_reverse_iterator;

        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;

        multimap() = default;
        explicit multimap(const Compare& comp, const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        template <class InputIterator>
        multimap(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        multimap(from_range_t, R&& rg, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                m_tree.insert_equal(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                m_tree.insert_equal(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }

        multimap(const multimap& rhs)
            : m_tree(rhs.m_tree) {}
        multimap(multimap&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}

        explicit multimap(const Allocator& alloc)
            : m_tree(alloc) {}
        multimap(const multimap& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(rhs.m_tree, alloc) {}
        multimap(multimap&& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(AZStd::move(rhs.m_tree), alloc) {}

        multimap(initializer_list<value_type> list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : multimap(list.begin(), list.end(), comp, alloc)
        {
        }

        template <class InputIterator>
        multimap(InputIterator first, InputIterator last, const Allocator& alloc)
            : multimap(first, last, Compare(), alloc)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        multimap(from_range_t, R&& rg, const Allocator& a)
            : multimap(from_range, AZStd::forward<R>(rg), Compare(), a)
        {
        }
        multimap(initializer_list<value_type> il, const Allocator& a)
            : multimap(il, Compare(), a)
        {
        }

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const { return value_compare(m_tree.key_comp()); }

        AZ_FORCE_INLINE iterator begin() { return m_tree.begin(); }
        AZ_FORCE_INLINE iterator end() { return m_tree.end(); }
        AZ_FORCE_INLINE const_iterator begin() const { return m_tree.begin(); }
        AZ_FORCE_INLINE const_iterator end() const { return m_tree.end(); }
        AZ_FORCE_INLINE reverse_iterator rbegin() { return m_tree.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator rend() { return m_tree.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        AZ_FORCE_INLINE bool empty() const { return m_tree.empty(); }
        AZ_FORCE_INLINE size_type size() const { return m_tree.size(); }
        AZ_FORCE_INLINE size_type max_size() const { return m_tree.max_size(); }
        AZ_FORCE_INLINE void swap(this_type& rhs) { m_tree.swap(rhs.m_tree); }

        // insert/erase
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value) { return m_tree.insert_equal(value); }
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value) { return m_tree.insert_equal(insertPos, value); }
        template <class InputIterator>
        void insert(InputIterator first, InputIterator last)
        {
            m_tree.insert_equal(first, last);
        }
        template<class R>
        auto insert_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>>
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                m_tree.insert_equal(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                m_tree.insert_equal(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }
        AZ_FORCE_INLINE iterator erase(const_iterator erasePos)                         { return m_tree.erase(erasePos); }
        AZ_FORCE_INLINE size_type erase(const key_type& key)                            { return m_tree.erase(key); }
        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)       { return m_tree.erase(first, last); }
        AZ_FORCE_INLINE void clear() { m_tree.clear(); }

        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        AZStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_equal(AZStd::forward<value_type>(value));
        }

        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_equal(insertPos, AZStd::forward<value_type>(value));
        }

        template<class ... InputArguments>
        iterator emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(AZStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(insertPos, AZStd::forward<InputArguments>(arguments) ...);
        }

        iterator insert(node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to multimap::insert(node_type&& nodeHandle)");
            return m_tree.node_handle_insert_equal(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to multimap::insert(const_iterator hint, node_type&& nodeHandle)");
            return m_tree.node_handle_insert_equal(hint, AZStd::move(nodeHandle));
        }

        node_type extract(const key_type& key)
        {
            return m_tree.template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return m_tree.template node_handle_extract<node_type>(it);
        }

        // set operations:
        const_iterator find(const key_type& key) const          { return m_tree.find(key); }
        iterator find(const key_type& key)                      { return m_tree.find(key); }
        bool contains(const key_type& key) const                { return m_tree.contains(key); }
        size_type count(const key_type& key) const              { return m_tree.count(key); }
        iterator lower_bound(const key_type& key)               { return m_tree.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const   { return m_tree.lower_bound(key); }
        iterator upper_bound(const key_type& key)               { return m_tree.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const   { return m_tree.upper_bound(key); }
        pair<iterator, iterator> equal_range(const key_type& key)                   { return m_tree.equal_range(key); }
        pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range(key); }

        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.count(key); }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range(key); }

        /**
        * \anchor MultimapExtensions
        * \name Extensions
        * @{
        */
        /**
        * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
        * this is here only to make sure it's available to the end user.
        */
        AZ_FORCE_INLINE iterator  insert(const key_type& key)
        {
            return m_tree.insert_equal(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_tree.get_allocator(); }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        AZ_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        AZ_INLINE bool      validate() const    { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        AZ_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        AZ_INLINE int       validate_iterator(const iterator& iter) const       { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        AZ_FORCE_INLINE void                        leak_and_reset()    { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(multimap<Key, MappedType, Compare, Allocator>& left, multimap<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class MappedType, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(multimap<Key, MappedType, Compare, Allocator>& container, Predicate predicate)
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
    template<class InputIterator, class Compare = less<iter_key_type<InputIterator>>,
        class Allocator = allocator>
        multimap(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
        ->multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        Compare, Allocator>;

    template<class R, class Compare = less<range_key_type<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    multimap(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
        ->multimap<range_key_type<R>, range_mapped_type<R>, Compare, Allocator>;

    template<class Key, class T, class Compare = less<Key>,
        class Allocator = allocator>
        multimap(initializer_list<pair<Key, T>>, Compare = Compare(), Allocator = Allocator())
        ->multimap<Key, T, Compare, Allocator>;

    template<class InputIterator, class Allocator>
    multimap(InputIterator, InputIterator, Allocator)
        ->multimap<iter_key_type<InputIterator>, iter_mapped_type<InputIterator>,
        less<iter_key_type<InputIterator>>, Allocator>;

    template<class R, class Allocator, class = enable_if_t<ranges::input_range<R>>>
    multimap(from_range_t, R&&, Allocator)
        ->multimap<range_key_type<R>, range_mapped_type<R>, less<range_key_type<R>>, Allocator>;

    template<class Key, class T, class Allocator>
    multimap(initializer_list<pair<Key, T>>, Allocator)
        ->multimap<Key, T, less<Key>, Allocator>;

}
