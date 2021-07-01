/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_QUEUE_H
#define AZSTD_QUEUE_H 1

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
        enum
        {
            CONTAINER_VERSION = 1
        };
    public:
        typedef queue<T, Container>  this_type;
        typedef Container container_type;
        typedef typename Container::value_type      value_type;
        typedef typename Container::size_type       size_type;
        typedef typename Container::reference       reference;
        typedef typename Container::const_reference const_reference;

        AZ_FORCE_INLINE queue() {}

        AZ_FORCE_INLINE explicit queue(const container_type& container)
            : m_container(container) {}
        AZ_FORCE_INLINE bool empty() const              { return m_container.empty(); }
        AZ_FORCE_INLINE size_type size() const          { return m_container.size(); }
        AZ_FORCE_INLINE reference front()               { return m_container.front(); }
        AZ_FORCE_INLINE const_reference front() const   { return m_container.front(); }
        AZ_FORCE_INLINE reference back()                    { return m_container.back(); }
        AZ_FORCE_INLINE const_reference back() const        { return m_container.back(); }
        AZ_FORCE_INLINE void push(const value_type& value)  { m_container.push_back(value); }
        AZ_FORCE_INLINE void pop()                      { m_container.pop_front(); }

        AZ_FORCE_INLINE void push()                     { m_container.push_back(); }

        AZ_FORCE_INLINE queue(this_type&& rhs)
            : m_container(AZStd::move(rhs.m_container))  {}
        AZ_FORCE_INLINE explicit queue(Container&& container)
            : m_container(AZStd::move(container)) {}
        this_type& operator=(this_type&& rhs)
        {
            m_container = AZStd::move(rhs.m_container);
            return (*this);
        }
        void push(value_type&& value)       { m_container.push_back(AZStd::move(value)); }
        template<class... Args>
        void emplace(Args&&... args)           { m_container.emplace_back(AZStd::forward<Args>(args)...); }
        void swap(this_type& rhs)           { AZStd::swap(m_container, rhs.m_container); }

        AZ_FORCE_INLINE Container& get_container()              { return m_container; }
        AZ_FORCE_INLINE const Container& get_container() const  { return m_container; }

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

    /*  template<class T, class Container>
        AZ_FORCE_INLINE bool operator<(const queue<T,Container>& left, const queue<T,Container>& right)
        {
            return left.get_container() < right.get_container();
        }

        template<class T, class Container>
        AZ_FORCE_INLINE bool operator>(const queue<T,Container>& left, const queue<T,Container>& right)
        {
            return left.get_container() > right.get_container();
        }

        template<class T, class Container>
        AZ_FORCE_INLINE operator<=(const queue<T,Container>& left, const queue<T,Container>& right)
        {
            return left.get_container() <= right.get_container();
        }

        template<class T, class Container>
        AZ_FORCE_INLINE bool operator>=(const queue<T,Container>& left, const queue<T,Container>& right)
        {
            return left.get_container() >= right.get_container();
        }*/

    /**
    * Priority queue is complaint with \ref CStd (23.2.3.2)
    * The only extension we have is that we allow access
    * to the underlying container via: get_container function.
    * Check the priority_queue \ref AZStdExamples.
    */
    template<class T, class Container = AZStd::vector<T>, class Predicate = AZStd::less<typename Container::value_type> >
    class priority_queue
    {
        enum
        {
            CONTAINER_VERSION = 1
        };
    public:
        typedef priority_queue<T, Container, Predicate>   this_type;
        typedef Container container_type;
        typedef typename Container::value_type value_type;
        typedef typename Container::size_type size_type;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;

        AZ_FORCE_INLINE priority_queue() {}
        AZ_FORCE_INLINE explicit priority_queue(const Predicate& comp)
            : m_comp(comp)   {}
        AZ_FORCE_INLINE priority_queue(const Predicate& comp, const container_type& container)
            : m_container(container)
            , m_comp(comp)
        {
            // construct by copying specified container, comparator
            AZStd::make_heap(m_container.begin(), m_container.end(), comp);
        }
        template<class InputIterator>
        AZ_FORCE_INLINE priority_queue(InputIterator first, InputIterator last)
            : m_container(first, last)
            , m_comp()
        {
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        template<class InputIterator>
        AZ_FORCE_INLINE priority_queue(InputIterator first, InputIterator last, const Predicate& comp)
            : m_container(first, last)
            , m_comp(comp)
        {   // construct by copying [_First, _Last), specified comparator
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        template<class InputIterator>
        AZ_FORCE_INLINE priority_queue(InputIterator first, InputIterator last, const Predicate& comp, const container_type& container)
            : m_container(container)
            , m_comp(comp)
        {   // construct by copying [_First, _Last), container, and comparator
            m_container.insert(m_container.end(), first, last);
            AZStd::make_heap(m_container.begin(), m_container.end(), m_comp);
        }

        AZ_FORCE_INLINE bool empty() const              { return m_container.empty(); }
        AZ_FORCE_INLINE size_type size() const          { return m_container.size(); }
        AZ_FORCE_INLINE const_reference top() const     { return m_container.front(); }
        AZ_FORCE_INLINE reference top()                 { return m_container.front(); }
        AZ_FORCE_INLINE void push(const value_type& value)
        {
            m_container.push_back(value);
            AZStd::push_heap(m_container.begin(), m_container.end(), m_comp);
        }

        AZ_FORCE_INLINE void pop()
        {
            AZStd::pop_heap(m_container.begin(), m_container.end(), m_comp);
            m_container.pop_back();
        }

        AZ_FORCE_INLINE priority_queue(this_type&& rhs)
            : m_container(AZStd::move(rhs.m_container))
            , m_comp(AZStd::move(rhs.m_comp))    {}
        AZ_FORCE_INLINE explicit priority_queue(const Predicate& pred, Container&& container)
            : m_container(AZStd::move(container))
            , m_comp(pred)    {}
        this_type& operator=(this_type&& rhs)
        {
            m_container = AZStd::move(rhs.m_container);
            m_comp = AZStd::move(rhs.m_comp);
            return (*this);
        }
        void push(value_type&& value)       {   m_container.push_back(AZStd::move(value)); AZStd::push_heap(m_container.begin(), m_container.end(), m_comp); }
        template<class Args>
        void emplace(Args&& args)           {   m_container.emplace_back(AZStd::forward<Args>(args)); AZStd::push_heap(m_container.begin(), m_container.end(), m_comp); }
        void swap(this_type& rhs)           {   AZStd::swap(m_container, rhs.m_container); AZStd::swap(m_comp, rhs.m_comp); }

        AZ_FORCE_INLINE Container& get_container()              { return m_container; }
        AZ_FORCE_INLINE const Container& get_container() const  { return m_container; }

    protected:
        Container m_container;
        Predicate m_comp;
    };
}

#endif // AZSTD_QUEUE_H
#pragma once
