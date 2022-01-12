/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZStd
{
    template <class Element>
    inline constexpr span<Element>::span()
        : m_begin(nullptr)
        , m_end(nullptr)
    { }

    template <class Element>
    inline constexpr span<Element>::span(pointer s, size_type length)
        : m_begin(s)
        , m_end(m_begin + length)
    {
        if (length == 0) erase();
    }

    template <class Element>
    inline constexpr span<Element>::span(pointer first, const_pointer last)
        : m_begin(first)
        , m_end(last)
    { }

    template <class Element>
    template<AZStd::size_t N>
    inline constexpr span<Element>::span(AZStd::array<Element, N>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    inline constexpr span<Element>::span(AZStd::vector<Element>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    template<AZStd::size_t N>
    inline constexpr span<Element>::span(AZStd::fixed_vector<Element, N>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    template<AZStd::size_t N>
    inline constexpr span<Element>::span(const AZStd::array<Element, N>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    inline constexpr span<Element>::span(const AZStd::vector<Element>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    template<AZStd::size_t N>
    inline constexpr span<Element>::span(const AZStd::fixed_vector<Element, N>& data)
        : m_begin(data.data())
        , m_end(m_begin + data.size())
    { }

    template <class Element>
    inline constexpr span<Element>::span(span&& other)
        : span(other.m_begin, other.m_end)
    {
#if AZ_DEBUG_BUILD // Clearing the original pointers isn't necessary, but is good for debugging
        other.m_begin = nullptr;
        other.m_end = nullptr;
#endif
    }

    template <class Element>
    inline constexpr AZStd::size_t span<Element>::size() const { return m_end - m_begin; }
    
    template <class Element>
    inline constexpr bool span<Element>::empty() const { return m_end == m_begin; }
    
    template <class Element>
    inline constexpr Element* span<Element>::data() { return m_begin; }
    
    template <class Element>
    inline constexpr const Element* span<Element>::data() const { return m_begin; }

    template <class Element>
    inline constexpr span<Element>& span<Element>::operator=(span<Element>&& other)
    {
        m_begin = other.m_begin;
        m_end = other.m_end;
#if AZ_DEBUG_BUILD // Clearing the original pointers isn't necessary, but is good for debugging
        other.m_begin = nullptr;
        other.m_end = nullptr;
#endif
        return *this;
    }
    
    template <class Element>
    inline constexpr const Element& span<Element>::operator[](AZStd::size_t index) const 
    {
        AZ_Assert(index < size(), "index value is out of range");
        return m_begin[index]; 
    }

    template <class Element>
    inline constexpr Element& span<Element>::operator[](AZStd::size_t index) 
    {
        AZ_Assert(index < size(), "index value is out of range");
        return m_begin[index]; 
    }

    template <class Element>
    inline constexpr void span<Element>::erase() { m_begin = m_end = nullptr; }
    
    template <class Element>
    inline constexpr Element*         span<Element>::begin() { return m_begin; }
    template <class Element>
    inline constexpr Element*         span<Element>::end() { return m_end; }
    template <class Element>
    inline constexpr const Element*   span<Element>::begin() const { return m_begin; }
    template <class Element>
    inline constexpr const Element*   span<Element>::end() const { return m_end; }

    template <class Element>
    inline constexpr const Element*   span<Element>::cbegin() const { return m_begin; }
    template <class Element>
    inline constexpr const Element*   span<Element>::cend() const { return m_end; }

    template <class Element>
    inline constexpr AZStd::reverse_iterator<Element*> span<Element>::rbegin() { return AZStd::reverse_iterator<Element*>(m_end); }
    template <class Element>
    inline constexpr AZStd::reverse_iterator<Element*> span<Element>::rend() { return AZStd::reverse_iterator<Element*>(m_begin); }
    template <class Element>
    inline constexpr AZStd::reverse_iterator<const Element*> span<Element>::rbegin() const { return AZStd::reverse_iterator<const Element*>(m_end); }
    template <class Element>
    inline constexpr AZStd::reverse_iterator<const Element*> span<Element>::rend() const { return AZStd::reverse_iterator<const Element*>(m_begin); }

    template <class Element>
    inline constexpr AZStd::reverse_iterator<const Element*> span<Element>::crbegin() const { return AZStd::reverse_iterator<const Element*>(cend()); }
    template <class Element>
    inline constexpr AZStd::reverse_iterator<const Element*> span<Element>::crend() const { return AZStd::reverse_iterator<const Element*>(cbegin()); }
} // namespace AZStd
