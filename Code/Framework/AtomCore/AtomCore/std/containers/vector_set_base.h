/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace AZStd
{
    /**
     * This class is an ordered set implementation which uses a sorted vector. Insertions / removals
     * are slower, but searches and iteration are very cache friendly. This container wraps an
     * AZStd::vector by default, but can wrap any random access container adhering to the interface.
     * See @ref fixed_vector_set for a version that does not perform any allocations. The iterator
     * invalidation behavior is directly inherited from the underlying container.
     */
    template <typename Key, typename Compare, typename RandomAccessContainer>
    class vector_set_base
    {
        using this_type = vector_set_base<Key, Compare, RandomAccessContainer>;
    public:
        using pointer = typename RandomAccessContainer::pointer;
        using reference = typename RandomAccessContainer::reference;
        using iterator = typename RandomAccessContainer::iterator;
        using reverse_iterator = typename RandomAccessContainer::reverse_iterator;

        using const_pointer = typename RandomAccessContainer::const_pointer;
        using const_reference = typename RandomAccessContainer::const_reference;
        using const_iterator = typename RandomAccessContainer::const_iterator;
        using const_reverse_iterator = typename RandomAccessContainer::const_reverse_iterator;

        using size_type = typename RandomAccessContainer::size_type;
        using difference_type = typename RandomAccessContainer::difference_type;
        using key_type = Key;

        using pair_iter_bool = AZStd::pair<iterator, bool>;

        template <typename ... Args>
        AZ_FORCE_INLINE vector_set_base(Args&&... arguments)
            : m_container{AZStd::forward<Args>(arguments)...}
        {}

        iterator                begin()             { return m_container.begin(); }
        const_iterator          begin() const       { return m_container.begin(); }
        iterator                end()               { return m_container.end(); }
        const_iterator          end() const         { return m_container.end(); }
        reverse_iterator        rbegin()            { return m_container.rbegin(); }
        const_reverse_iterator  rbegin() const      { return m_container.rbegin(); }
        reverse_iterator        rend()              { return m_container.rend(); }
        const_reverse_iterator  rend() const        { return m_container.rend(); }

        reference               front()             { return m_container.front(); }
        const_reference         front() const       { return m_container.front(); }
        reference               back()              { return m_container.back(); }
        const_reference         back() const        { return m_container.back(); }

        size_type               size() const        { return m_container.size(); }
        size_type               capacity() const    { return m_container.capacity(); }
        bool                    empty() const       { return m_container.empty(); }

        pointer                 data()              { return m_container.data(); }
        const_pointer           data() const        { return m_container.data(); }

        template <typename ... Args>
        pair_iter_bool emplace(Args&&... arguments)
        {
            return insert(Key(AZStd::forward<Args>(arguments) ...));
        }

        pair_iter_bool insert(key_type&& key)
        {
            Compare comp;
            iterator first = lower_bound(key);
            if (first != m_container.end())
            {
                if (comp(key, *first))
                {
                    return pair_iter_bool(m_container.insert(first, AZStd::move(key)), true);
                }
                else
                {
                    return pair_iter_bool(first, false);
                }
            }
            return pair_iter_bool(m_container.insert(first, AZStd::move(key)), true);
        }

        pair_iter_bool insert(const_reference key)
        {
            Compare comp;
            iterator first = lower_bound(key);
            if (first != m_container.end())
            {
                if (comp(key, *first))
                {
                    return pair_iter_bool(m_container.insert(first, key), true);
                }
                else
                {
                    return pair_iter_bool(first, false);
                }
            }
            return pair_iter_bool(m_container.insert(first, key), true);
        }

        template <typename InputIterator>
        void assign(InputIterator first, InputIterator last)
        {
            m_container.assign(first, last);

            // Sort the whole container.
            AZStd::sort(m_container.begin(), m_container.end(), Compare());

            // De-duplicate entries and resize.
            iterator newEnd = AZStd::unique(m_container.begin(), m_container.end());
            m_container.erase(newEnd, m_container.end());
        }

        template <typename InputIterator>
        void insert(InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }

        iterator lower_bound(const key_type& key)
        {
            return AZStd::lower_bound(m_container.begin(), m_container.end(), key, Compare());
        }

        const_iterator lower_bound(const key_type& key) const
        {
            return AZStd::lower_bound(m_container.begin(), m_container.end(), key, Compare());
        }

        iterator upper_bound(const key_type& key)
        {
            return AZStd::upper_bound(m_container.begin(), m_container.end(), key, Compare());
        }

        const_iterator upper_bound(const key_type& key) const
        {
            return AZStd::upper_bound(m_container.begin(), m_container.end(), key, Compare());
        }

        iterator find(const key_type& key)
        {
            Compare comp;
            iterator first = lower_bound(key);
            if (first != m_container.end() && !comp(key, *first))
            {
                return first;
            }
            return m_container.end();
        }

        const_iterator find(const key_type& key) const
        {
            Compare comp;
            const_iterator first = lower_bound(key);
            if (first != m_container.end() && !comp(key, *first))
            {
                return first;
            }
            return m_container.end();
        }

        size_t erase(const key_type& key)
        {
            Compare comp;
            iterator first = lower_bound(key);
            if (first != m_container.end() && !comp(key, *first))
            {
                m_container.erase(first);
                return 1;
            }
            return 0;
        }

        reference at(size_type position)
        {
            return m_container.at(position);
        }

        const_reference at(size_type position) const
        {
            return m_container.at(position);
        }

        reference operator[](size_type position)
        {
            return m_container[position];
        }

        const_reference operator[](size_type position) const
        {
            return m_container[position];
        }

        void swap(this_type& rhs)
        {
            m_container.swap(rhs.m_container);
        }

        void clear()
        {
            m_container.clear();
        }

        friend bool operator==(const this_type& lhs, const this_type& rhs)
        {
            return lhs.m_container == rhs.m_container;
        }

        friend bool operator!=(const this_type& lhs, const this_type& rhs)
        {
            return lhs.m_container != rhs.m_container;
        }

    protected:
        RandomAccessContainer m_container;
    };
}
