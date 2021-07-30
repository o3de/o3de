/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional_basic.h>

namespace AZStd
{
    /**
     * FIFO queue complaint with \ref CStd (23.2.3.1)
     * The only extension we have is that we allow access
     * to the underlying container via: get_container function.
     * Check the queue \ref AZStdExamples.
     */
    template<class T, class Container = AZStd::deque<T> >
    class queue
    {
    public:
        using this_type = queue;
        using container_type = Container;
        using value_type = typename Container::value_type;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;

        queue() = default;
        queue(const this_type& rhs) = default;
        queue(this_type && rhs) = default;

        explicit queue(const container_type& container)
            : m_container(container) {}
        explicit queue(Container&& container)
            : m_container(AZStd::move(container)) {}

        this_type& operator=(const this_type & rhs) = default;
        this_type& operator=(this_type && rhs) = default;

        [[nodiscard]] bool empty() const              { return m_container.empty(); }
        size_type size() const          { return m_container.size(); }
        reference front()               { return m_container.front(); }
        const_reference front() const   { return m_container.front(); }
        reference back()                    { return m_container.back(); }
        const_reference back() const        { return m_container.back(); }
        void push(const value_type& value)  { m_container.push_back(value); }
        void push(value_type&& value)       { m_container.push_back(AZStd::move(value)); }
        void pop()                      { m_container.pop_front(); }


        template<class... Args>
        void emplace(Args&&... args)           { m_container.emplace_back(AZStd::forward<Args>(args)...); }
        void swap(this_type& rhs)           { AZStd::swap(m_container, rhs.m_container); }

        Container& get_container()              { return m_container; }
        const Container& get_container() const  { return m_container; }

    protected:
        Container m_container;
    };

    // queue TEMPLATE FUNCTIONS
    template<class T, class Container>
    AZ_FORCE_INLINE bool operator==(const AZStd::queue<T, Container>& left, const AZStd::queue<T, Container>& right)
    {
        return left.get_container() == right.get_container();
    }

    template<class T, class Container>
    AZ_FORCE_INLINE bool operator!=(const AZStd::queue<T, Container>& left, const AZStd::queue<T, Container>& right)
    {
        return left.get_container() != right.get_container();
    }

    /**
    * Priority queue is complaint with \ref CStd (23.2.3.2)
    * The only extension we have is that we allow access
    * to the underlying container via: get_container function.
    * Check the priority_queue \ref AZStdExamples.
    */
    template<class T, class Container = AZStd::vector<T>, class Predicate = AZStd::less<typename Container::value_type> >
    class priority_queue
    {
    public:
        using this_type = priority_queue;
        using container_type = Container;
        using value_compare = Predicate;
        using value_type = typename Container::value_type;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;

        priority_queue() = default;
        priority_queue(const this_type& rhs) = default;
        priority_queue(this_type && rhs) = default;

        explicit priority_queue(const Predicate& comp)
            : m_comp(comp)   {}
        priority_queue(const Predicate& comp, const container_type& container)
            : m_container(container)
            , m_comp(comp)
        {
            // construct by copying specified container, comparator
            AZStd::make_heap(m_container.begin(), m_container.end(), comp);
        }
        template<class InputIterator>
        priority_queue(InputIterator first, InputIterator last)
            : m_container(first, last)
            , m_comp()
        {
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        template<class InputIterator>
        priority_queue(InputIterator first, InputIterator last, const Predicate& comp)
            : m_container(first, last)
            , m_comp(comp)
        {   // construct by copying [_First, _Last), specified comparator
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        template<class InputIterator>
        priority_queue(InputIterator first, InputIterator last, const Predicate& comp, const container_type& container)
            : m_container(container)
            , m_comp(comp)
        {   // construct by copying [_First, _Last), container, and comparator
            m_container.insert(m_container.end(), first, last);
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        this_type& operator=(const this_type & rhs) = default;
        this_type& operator=(this_type && rhs) = default;

        [[nodiscard]] bool empty() const { return m_container.empty(); }
        size_type size() const          { return m_container.size(); }
        const_reference top() const     { return m_container.front(); }
        reference top()                 { return m_container.front(); }
        void push(const value_type& value)
        {
            m_container.push_back(value);
            AZStd::push_heap(m_container.begin(), m_container.end(), m_comp);
        }
        void push(value_type&& value)
        {
            m_container.push_back(AZStd::move(value));
            AZStd::push_heap(m_container.begin(), m_container.end(), m_comp);
        }

        void pop()
        {
            AZStd::pop_heap(m_container.begin(), m_container.end(), m_comp);
            m_container.pop_back();
        }


        template<class... Args>
        void emplace(Args&&... args)        {   m_container.emplace_back(AZStd::forward<Args>(args)...); AZStd::push_heap(m_container.begin(), m_container.end(), m_comp); }
        void swap(this_type& rhs)           {   AZStd::swap(m_container, rhs.m_container); AZStd::swap(m_comp, rhs.m_comp); }

        Container& get_container()              { return m_container; }
        const Container& get_container() const  { return m_container; }

    protected:
        Container m_container;
        Predicate m_comp;
    };
}
