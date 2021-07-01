/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_STACK_H
#define AZSTD_STACK_H 1

#include <AzCore/std/containers/deque.h>

namespace AZStd
{
    /**
     * Stack container is complaint with \ref CStd (23.2.3.3)
     * The only extension we have is that we allow access
     * to the underlying container via: get_container function.
     * Check the stack \ref AZStdExamples.
     */
    template<class T, class Container = AZStd::deque<T> >
    class stack
    {
        enum
        {
            CONTAINER_VERSION = 1
        };
    public:
        typedef stack<T, Container>                  this_type;
        typedef Container container_type;
        typedef typename Container::value_type      value_type;
        typedef typename Container::size_type       size_type;
        typedef typename Container::reference       reference;
        typedef typename Container::const_reference const_reference;

        AZ_FORCE_INLINE stack() {}
        AZ_FORCE_INLINE explicit stack(const container_type& container)
            : m_container(container) {}
        AZ_FORCE_INLINE bool empty() const              { return m_container.empty(); }
        AZ_FORCE_INLINE size_type size() const          { return m_container.size(); }
        AZ_FORCE_INLINE reference top()                 { return m_container.back(); }
        AZ_FORCE_INLINE const_reference top() const     { return m_container.back(); }
        AZ_FORCE_INLINE reference back()                    { return m_container.back(); }
        AZ_FORCE_INLINE const_reference back() const        { return m_container.back(); }
        AZ_FORCE_INLINE void push(const value_type& value)  { m_container.push_back(value); }
        AZ_FORCE_INLINE void pop()                      { m_container.pop_back(); }
        AZ_FORCE_INLINE void push()                     { m_container.push_back(); }

        AZ_FORCE_INLINE stack(this_type&& rhs)
            : m_container(AZStd::move(rhs.m_container)) {}
        AZ_FORCE_INLINE explicit stack(Container&& container)
            : m_container(AZStd::move(container)) {}
        this_type& operator=(this_type&& rhs)       { m_container = AZStd::move(rhs.m_container); return *this; }
        void push(value_type&& value)               { m_container.push_back(AZStd::move(value)); }
        template<class Args>
        void emplace(Args&& args)                   { m_container.emplace_back(AZStd::forward<Args>(args)); }
        void swap(this_type&& rhs)                  { m_container.swap(AZStd::move(rhs.m_container)); }

        void swap(this_type& rhs)                   { AZStd::swap(m_container, rhs.m_container); }

        AZ_FORCE_INLINE Container& get_container()              { return m_container; }
        AZ_FORCE_INLINE const Container& get_container() const  { return m_container; }

    protected:
        Container m_container;
    };

    // queue TEMPLATE FUNCTIONS
    template<class T, class Container>
    AZ_FORCE_INLINE bool operator==(const AZStd::stack<T, Container>& left, const AZStd::stack<T, Container>& right)
    {
        return left.get_container() == right.get_container();
    }

    template<class T, class Container>
    AZ_FORCE_INLINE bool operator!=(const AZStd::stack<T, Container>& left, const AZStd::stack<T, Container>& right)
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
}

#endif // AZSTD_STACK_H
#pragma once
