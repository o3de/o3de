/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Interval.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/optional.h>

#include <map>

namespace AZ::RHI
{
    //! Simple implementation of an interval map. An interval map creates disjoint intervals where each
    //! interval stores an object T. Every insertion of a new interval overrides whatever value was before
    //! in every interval that overlaps with the new one. Adjacent intervals with the same object are merge together
    //! after insertions and removals. All intervals are not inclusive with the end of the interval [a,b)
    //!
    //! Implementation:
    //! It uses an ordered map to stores the beginning and end of all intervals. For example, if the
    //! interval [4, 8) with object T is inserted, this will translate to two insertions in the map. The first will be
    //! (4, T), and the second one (8, invalid). To store the T object in the map we use an "optional" object so we can mark the end
    //! of an interval. If later, another interval [6, 10) with object X is inserted, the
    //! map will contains [(4, T), (6, X), (10, invalid)]. Then, if the interval [2, 5) with value T is inserted, the map
    //! will contain [(2, T), (6, X), (10, invalid)], and the two intervals with element T will be merge into one.
    //! Finally, if the range [5, 7) is queried, the interal_map would return two intervals:
    //! - [5, 6) -> T
    //! - [6, 7) -> X
    //! Object T must be comparable and copyable.
    template <class Key, class T, class Compare = AZStd::less<Key>>
    class interval_map
    {
        using this_type = interval_map<Key, T, Compare>;

    public:
        template<class T1, class T2>
        using pair = AZStd::pair<T1, T2>;
        using mapped_type = AZStd::optional<T>;

        using Allocator = std::allocator<std::pair<const Key, mapped_type>>;
        // Using std::map for now because AZStd::map has a memory leak when inserting with a hint (LY-115607)
        using container_type = std::map<Key, mapped_type, Compare, Allocator>;
        using container_iterator = typename container_type::iterator;
        using container_const_iterator = typename container_type::const_iterator;
        using key_compare = typename container_type::key_compare;
        using value_compare = typename container_type::value_compare;
        using key_type = typename container_type::key_type;

        using size_type = std::size_t;

        class const_iterator
        {
            using this_type = const_iterator;
            friend class interval_map<Key, T, Compare>;
        public:

            using reference_value = const T&;
            using interval_type = pair<key_type, key_type>;

            const_iterator() {}
            const_iterator(const container_type& container, container_const_iterator iter)
                : m_container(&container), m_iterator(iter) {}

            bool operator==(const const_iterator& other) const
            {
                return m_container == other.m_container && m_iterator == other.m_iterator;
            }

            bool operator!=(const const_iterator& other) const
            {
                return m_container != other.m_container || m_iterator != other.m_iterator;
            }

            AZ_FORCE_INLINE key_type interval_begin() const
            {
                AZ_Assert(m_iterator != m_container->end(), "Invalid begin interval");
                return m_iterator->first;
            }

            AZ_FORCE_INLINE key_type interval_end() const
            {
                AZ_Assert(m_iterator != m_container->end(), "Invalid begin interval");
                auto next = AZStd::next(m_iterator);
                AZ_Assert(next != m_container->end(), "Invalid end interval");
                return next->first;
            }

            AZ_FORCE_INLINE reference_value value() const
            {
                AZ_Assert(m_iterator != m_container->end() && m_iterator->second.has_value(), "Invalid begin interval");
                return m_iterator->second.value();
            }

            AZ_FORCE_INLINE interval_type interval() const
            {
                return AZStd::make_pair(interval_begin(), interval_end());
            }

            pair<interval_type, reference_value> operator*() const
            {
                return AZStd::pair<interval_type, reference_value>(interval(), value());
            }

            AZ_FORCE_INLINE this_type& operator++()
            {
                AZ_Assert(m_iterator != m_container->end(), "Invalid begin interval");
                // Move the m_iterator forward until we find an iterator with a valid entry (i.e. the
                // beginning of a new interval).
                while (++m_iterator != m_container->end() && !m_iterator->second.has_value()) {};
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                this_type tmp = *this;
                ++(*this);
                return tmp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZ_Assert(m_iterator != m_container->begin(), "Interval is at the beginning");
                // Move backwards until we find an iterator that points to a valid entry (i.e. the
                // beginning of a new interval).
                while (--m_iterator != m_container->begin() && !m_iterator->second.has_value()) {};
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                this_type tmp = *this;
                --(*this);
                return tmp;
            }

        protected:
            const container_type* m_container = nullptr;
            container_const_iterator m_iterator;
        };

        class iterator : public const_iterator
        {
            using this_type = iterator;
            using base_type = const_iterator;
        public:

            using reference_value = T&;

            iterator() {}
            iterator(const container_type& container, container_iterator iter)
                : const_iterator(container, iter){}

            AZ_FORCE_INLINE this_type& operator++() { base_type::operator++(); return *this; }
            AZ_FORCE_INLINE this_type operator++(int)
            {
                this_type temp = *this;
                ++*this;
                return temp;
            }
            AZ_FORCE_INLINE this_type& operator--() { base_type::operator--(); return *this; }
            AZ_FORCE_INLINE this_type operator--(int)
            {
                this_type temp = *this;
                --*this;
                return temp;
            }
            AZ_FORCE_INLINE reference_value value() const
            {
                AZ_Assert(this->m_iterator != this->m_container->end() && this->m_iterator->second.has_value(), "Invalid begin interval");
                return const_cast<reference_value>(this->m_iterator->second.value());
            }
        };

        AZ_FORCE_INLINE explicit interval_map(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_container(comp, alloc) {}

        template <class InputIterator>
        AZ_FORCE_INLINE interval_map(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_container(first, last, comp, alloc) {}
        AZ_FORCE_INLINE interval_map(const this_type& rhs)
            : m_container(rhs.m_container) {}
        AZ_FORCE_INLINE interval_map(const this_type& rhs, const Allocator& alloc)
            : m_container(rhs.m_container, alloc) {}

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_container = rhs.m_container; return *this; }
        AZ_FORCE_INLINE key_compare key_comp() const { return m_container.key_comp(); }
        AZ_FORCE_INLINE value_compare value_comp() const { return m_container.value_comp(); }

        AZ_FORCE_INLINE iterator begin() { return iterator(m_container, m_container.begin()); }
        AZ_FORCE_INLINE iterator end() { return iterator(m_container, m_container.end()); }
        AZ_FORCE_INLINE const_iterator begin() const { return const_iterator(m_container, m_container.begin()); }
        AZ_FORCE_INLINE const_iterator end() const { return const_iterator(m_container, m_container.end()); }
        AZ_FORCE_INLINE bool empty() const { return m_container.empty(); }
        AZ_FORCE_INLINE void swap(this_type& rhs) { m_container.swap(rhs.m_container); }

        AZ_FORCE_INLINE const_iterator at(const key_type& key) const
        {
            auto upper_bound = m_container.upper_bound(key);
            if (upper_bound == m_container.begin() || !has_value(AZStd::prev(upper_bound)))
            {
                return end();
            }

            return const_iterator(m_container, AZStd::prev(upper_bound));
        }

        // Returns a begin and end iterator for the intervals in the map that overlaps with the [lower, upper) interval
        // that was passed.
        AZ_FORCE_INLINE pair<const_iterator, const_iterator> overlap(const key_type& lower, const key_type& upper) const
        {
            if (!key_comp()(lower, upper))
            {
                return pair<const_iterator, const_iterator>(end(), end());
            }

            auto lower_limit = m_container.upper_bound(lower);
            auto upper_limit = m_container.lower_bound(upper);
            if (lower_limit == m_container.end() || upper_limit == m_container.begin())
            {
                return pair<const_iterator, const_iterator>(end(), end());
            }

            const_iterator begin_range;
            const_iterator end_range;
            if (lower_limit == m_container.begin())
            {
                begin_range = const_iterator(m_container, lower_limit);
            }
            else
            {
                auto prev = AZStd::prev(lower_limit);
                begin_range = const_iterator(m_container, has_value(prev) ? prev : lower_limit);
            }

            if (upper_limit == m_container.end())
            {
                end_range = const_iterator(m_container, upper_limit);
            }
            else
            {
                auto next = AZStd::next(upper_limit);
                end_range = const_iterator(m_container, has_value(upper_limit) ? upper_limit : next);
            }

            return pair<const_iterator, const_iterator>(begin_range, end_range);
        }

        // Assign the interval [lower, upper) with a new value. All overlapping intervals will
        // have their value overwritten with the new value. All adjacent intervals with the same value
        // will be merge together.
        AZ_FORCE_INLINE iterator assign(const key_type& lower, const key_type& upper, const T& value)
        {
            if (!key_comp()(lower, upper))
            {
                return end();
            }

            auto last = insert_upper_bound(upper);
            auto first = m_container.lower_bound(lower);
            m_container.erase(first, last);
            first = m_container.insert(last, std::make_pair(lower, value));

            // Merge intervals with the same value.
            if (first != m_container.begin())
            {
                auto prev = AZStd::prev(first);
                if (has_value(prev) && prev->second == value)
                {
                    m_container.erase(first);
                    first = prev;
                }
            }

            if (has_value(last) &&
                last->second == value)
            {
                last = m_container.erase(last);
            }

            return iterator(m_container, first);
        }

        AZ_FORCE_INLINE const_iterator erase(const_iterator erasePos)
        {
            if (erasePos == end())
            {
                return end();
            }

            container_const_iterator eraseIt;
            auto lower = erasePos.m_iterator;
            auto upper = AZStd::next(erasePos.m_iterator);
            AZ_Assert(lower != m_container.end() && upper != m_container.end(), "Invalid erase iterator");

            if (!has_value(upper))
            {
                m_container.erase(upper);
            }

            if (lower == m_container.begin() || !has_value(AZStd::prev(lower)))
            {
                eraseIt = m_container.erase(lower);
            }
            else
            {
                const_cast<mapped_type&>(lower->second).reset();
                eraseIt = AZStd::next(lower);
            }

            return const_iterator(m_container, eraseIt);
        }

        AZ_FORCE_INLINE void clear() { m_container.clear(); }

    private:
        // Returns true if the iterator contains a valid object.
        bool has_value(container_const_iterator iterator) const
        {
            return iterator->second.has_value();
        }

        // Inserts the upper bound of an interval
        auto insert_upper_bound(const key_type& upper)
        {
            auto last = m_container.upper_bound(upper);
            if (last == m_container.begin())
            {
                return m_container.insert(last, { upper, mapped_type() });
            }

            auto&& value_before = AZStd::prev(last)->second;
            return m_container.insert(last, std::make_pair(upper, value_before));
        }

        container_type m_container;
    };
}
