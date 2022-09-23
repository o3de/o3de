/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/containers_concepts.h>
#include <AzCore/std/containers/node_handle.h>
#include <AzCore/std/containers/rbtree.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/typetraits/type_identity.h>

namespace AZStd
{
    namespace Internal
    {
        template<class T, class KeyEq, class Allocator >
        struct OrderedSetRbTreeTraits
        {
            using key_type = T;
            using key_equal = KeyEq;
            using value_type = T;
            using allocator_type = Allocator;
            static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    }

    /**
    * Ordered set container is complaint with \ref C++0x (23.4.3)
    * This is an associative container, all keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class set
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = set<Key, Compare, Allocator>;
        using tree_type = rbtree<Internal::OrderedSetRbTreeTraits<Key, Compare, Allocator>>;
    public:
        using traits_type = typename tree_type::traits_type;

        using key_type = typename tree_type::key_type;
        using value_type = typename tree_type::value_type;
        using key_compare = typename tree_type::key_equal;
        using value_compare = typename tree_type::key_equal;

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

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;
        using insert_return_type = AssociativeInternal::insert_return_type<iterator, node_type>;

        set() = default;
        explicit set(const Compare& comp, const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}

        template <class InputIterator>
        set(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        set(from_range_t, R&& rg, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
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
        set(const set& rhs)
            : m_tree(rhs.m_tree) {}
        set(set&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}

        explicit set(const Allocator& alloc)
            : m_tree(alloc) {}
        set(const set& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(rhs.m_tree, alloc) {}
        set(set&& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(AZStd::move(rhs.m_tree), alloc) {}

        set(initializer_list<value_type> list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : set(list.begin(), list.end(), comp, alloc) {}

        template <class InputIterator>
        set(InputIterator first, InputIterator last, const Allocator& alloc)
            : set(first, last, Compare(), alloc)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        set(from_range_t, R&& rg, const Allocator& a)
            : set(from_range, AZStd::forward<R>(rg), Compare(), a)
        {
        }
        set(initializer_list<value_type> list, const Allocator& alloc)
            : set(list, Compare(), alloc) {}

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const { return m_tree.key_comp(); }

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
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value) { return m_tree.insert_unique(value); }
        AZ_FORCE_INLINE iterator insert(iterator insertPos, const value_type& value) { return m_tree.insert_unique(insertPos, value); }
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
                "node_type with incompatible allocator passed to set::insert(node_type&& nodeHandle)");
            return m_tree.template node_handle_insert_unique<insert_return_type>(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to set::insert(const_iterator hint, node_type&& nodeHandle)");
            return m_tree.node_handle_insert_unique(hint, AZStd::move(nodeHandle));
        }

        node_type extract(const key_type& key)
        {
            return m_tree.template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return m_tree.template node_handle_extract<node_type>(it);
        }

        AZ_FORCE_INLINE iterator erase(const_iterator erasePos) { return m_tree.erase(erasePos); }
        AZ_FORCE_INLINE size_type erase(const key_type& key) { return m_tree.erase_unique(key); }
        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last) { return m_tree.erase(first, last); }
        AZ_FORCE_INLINE void clear() { m_tree.clear(); }

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
        auto find(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.count(key); }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range_unique(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range_unique(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
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

    template<class Key, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(set<Key, Compare, Allocator>& left, set<Key, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator==(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
            && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator!=(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator<(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator>(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator<=(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator>=(const set<Key, Compare, Allocator>& left, const set<Key, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(set<Key, Compare, Allocator>& container, Predicate predicate)
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
        class Compare = less<iter_value_type<InputIterator>>,
        class Allocator = allocator>
    set(InputIterator, InputIterator,
        Compare = Compare(), Allocator = Allocator())
        ->set<iter_value_type<InputIterator>, Compare, Allocator>;

    template<class R, class Compare = less<ranges::range_value_t<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    set(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
        ->set<ranges::range_value_t<R>, Compare, Allocator>;

    template<class Key, class Compare = less<Key>, class Allocator = allocator>
    set(initializer_list<Key>, Compare = Compare(), Allocator = Allocator())
        ->set<Key, Compare, Allocator>;

    template<class InputIterator, class Allocator>
    set(InputIterator, InputIterator, Allocator)
        ->set<iter_value_type<InputIterator>,
        less<iter_value_type<InputIterator>>, Allocator>;

    template<class R, class Allocator, class = enable_if_t<ranges::input_range<R>>>
    set(from_range_t, R&&, Allocator)
        ->set<ranges::range_value_t<R>, less<ranges::range_value_t<R>>, Allocator>;

    template<class Key, class Allocator>
    set(initializer_list<Key>, Allocator)->set<Key, less<Key>, Allocator>;

    /**
    * Ordered multiset container is complaint with \ref C++0x (23.4.4)
    * This is an associative container, key can be equivalent (multiple copies of the same key value).
    * You can iterate over them, by checking if the key stays the same, after finding the first.
    *
    * Check the set \ref AZStdExamples.
    */
    template<class Key, class Compare = AZStd::less<Key>, class Allocator = AZStd::allocator>
    class multiset
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = multiset<Key, Compare, Allocator>;
        using tree_type = rbtree<Internal::OrderedSetRbTreeTraits<Key, Compare, Allocator>>;
    public:
        using traits_type = typename tree_type::traits_type;

        using key_type = typename tree_type::key_type;
        using value_type = typename tree_type::value_type;
        using key_compare = typename tree_type::key_equal;
        using value_compare = typename tree_type::key_equal;

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

        using node_type = set_node_handle<set_node_traits<value_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;

        multiset() = default;
        explicit multiset(const Compare& comp, const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}

        template <class InputIterator>
        multiset(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        multiset(from_range_t, R&& rg, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
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
        multiset(const multiset& rhs)
            : m_tree(rhs.m_tree) {}
        multiset(multiset&& rhs)
            : m_tree(AZStd::move(rhs.m_tree)) {}

        explicit multiset(const Allocator& alloc)
            : m_tree(alloc) {}
        multiset(const multiset& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(rhs.m_tree, alloc) {}
        multiset(multiset&& rhs, const type_identity_t<Allocator>& alloc)
            : m_tree(AZStd::move(rhs.m_tree), alloc) {}

        multiset(initializer_list<value_type> list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : multiset(list.begin(), list.end(), comp, alloc) {}

        template <class InputIterator>
        multiset(InputIterator first, InputIterator last, const Allocator& alloc)
            : multiset(first, last, Compare(), alloc)
        {
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        multiset(from_range_t, R&& rg, const Allocator& a)
            : multiset(from_range, AZStd::forward<R>(rg), Compare(), a)
        {
        }
        multiset(initializer_list<value_type> list, const Allocator& alloc)
            : multiset(list, Compare(), alloc) {}

        // Add move semantics...
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const        { return m_tree.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const    { return m_tree.key_comp(); }

        AZ_FORCE_INLINE iterator begin()                    { return m_tree.begin(); }
        AZ_FORCE_INLINE iterator end()                      { return m_tree.end(); }
        AZ_FORCE_INLINE const_iterator begin() const        { return m_tree.begin(); }
        AZ_FORCE_INLINE const_iterator end() const          { return m_tree.end(); }
        AZ_FORCE_INLINE reverse_iterator rbegin()           { return m_tree.rbegin(); }
        AZ_FORCE_INLINE reverse_iterator rend()             { return m_tree.rend(); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        AZ_FORCE_INLINE bool empty() const                  { return m_tree.empty(); }
        AZ_FORCE_INLINE size_type size() const              { return m_tree.size(); }
        AZ_FORCE_INLINE size_type max_size() const          { return m_tree.max_size(); }
        AZ_FORCE_INLINE void swap(this_type& rhs)           { m_tree.swap(rhs.m_tree); }

        // insert/erase
        AZ_FORCE_INLINE pair<iterator, bool> insert(const value_type& value)             { return m_tree.insert_equal(value); }
        AZ_FORCE_INLINE iterator insert(iterator insertPos, const value_type& value)    { return m_tree.insert_equal(insertPos, value); }
        template<class InputIterator>
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
        iterator insert(value_type&& value)
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
                "node_type with incompatible allocator passed to multiset::insert(node_type&& nodeHandle)");
            return m_tree.node_handle_insert_equal(AZStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            AZSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to multiset::insert(const_iterator hint, node_type&& nodeHandle)");
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
        auto find(const ComparableToKey& key) const ->enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) ->  enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.count(key); }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<::AZStd::Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
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

    template<class Key, class Compare, class Allocator>
    AZ_FORCE_INLINE void swap(multiset<Key, Compare, Allocator>& left, multiset<Key, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator==(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator!=(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator<(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator>(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator<=(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class Compare, class Allocator>
    inline bool operator>=(const multiset<Key, Compare, Allocator>& left, const multiset<Key, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(multiset<Key, Compare, Allocator>& container, Predicate predicate)
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
        class Compare = less<iter_value_type<InputIterator>>,
        class Allocator = allocator>
    multiset(InputIterator, InputIterator,
            Compare = Compare(), Allocator = Allocator())
        ->multiset<iter_value_type<InputIterator>, Compare, Allocator>;

    template<class R, class Compare = less<ranges::range_value_t<R>>,
        class Allocator = allocator,
        class = enable_if_t<ranges::input_range<R>>>
    multiset(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
        ->multiset<ranges::range_value_t<R>, Compare, Allocator>;

    template<class Key, class Compare = less<Key>, class Allocator = allocator>
    multiset(initializer_list<Key>, Compare = Compare(), Allocator = Allocator())
        ->multiset<Key, Compare, Allocator>;

    template<class InputIterator, class Allocator>
    multiset(InputIterator, InputIterator, Allocator)
        ->multiset<iter_value_type<InputIterator>,
        less<iter_value_type<InputIterator>>, Allocator>;

    template<class R, class Allocator, class = enable_if_t<ranges::input_range<R>>>
    multiset(from_range_t, R&&, Allocator)
        ->multiset<ranges::range_value_t<R>, less<ranges::range_value_t<R>>, Allocator>;

    template<class Key, class Allocator>
    multiset(initializer_list<Key>, Allocator)->multiset<Key, less<Key>, Allocator>;
}
