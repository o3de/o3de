/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
    public:
        using this_type = stack;
        using container_type = Container;
        using value_type = typename Container::value_type;
        using size_type = typename Container::size_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;

        stack() = default;
        stack(const this_type& rhs) = default;
        stack(this_type&& rhs) = default;

        explicit stack(const container_type& container)
            : m_container(container) {}
        explicit stack(container_type&& container)
            : m_container(AZStd::move(container)) {}

        this_type& operator=(const this_type& rhs) = default;
        this_type& operator=(this_type && rhs) = default;

        [[nodiscard]] bool empty() const              { return m_container.empty(); }
        size_type size() const          { return m_container.size(); }
        reference top()                 { return m_container.back(); }
        const_reference top() const     { return m_container.back(); }
        void push(const value_type& value) { m_container.push_back(value); }
        void push(value_type&& value) { m_container.push_back(AZStd::move(value)); }
        void pop()                      { m_container.pop_back(); }


        template<class... Args>
        decltype(auto) emplace(Args&&... args)                   { return m_container.emplace_back(AZStd::forward<Args>(args)...); }

        void swap(this_type& rhs)                   { AZStd::swap(m_container, rhs.m_container); }

        Container& get_container()              { return m_container; }
        const Container& get_container() const  { return m_container; }

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
}
