/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/array.h>

namespace AZStd
{
    /**
     * Immutable wrapper for an array of data. It does not maintain storage for the data,
     * but just holds pointers to mark the beginning and end of the array. It can be 
     * conveniently constructed from a variety of other container types like array,
     * vector, and fixed_vector. 
     *
     * Example:
     *    Given "void Func(AZStd::array_view<int> a) {...}" you can call...
     *    - Func({1,2,3});
     *    - AZStd::array<int,3> a = {1,2,3};
     *      Func(a);
     *    - AZStd::vector<int> v = {1,2,3};
     *      Func(v);
     *    - AZStd::fixed_vector<int,10> fv = {1,2,3};
     *      Func(fv);
     *
     * Since the array_view does not copy and store any data, it is only valid as long as the data used to create it is valid.
     */
    template <class Element>
    class array_view final
    {
    public:
        using value_type = Element;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        using reference = value_type&;
        using const_reference = const value_type&;

        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        using iterator = const value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        array_view()
            : m_begin(nullptr)
            , m_end(nullptr)
        { }

        ~array_view() = default;

        array_view(const_pointer s, size_type length)
            : m_begin(s)
            , m_end(m_begin + length)
        {
            if (length == 0) erase();
        }

        array_view(const_pointer first, const_pointer last)
            : m_begin(first)
            , m_end(last)
        { }

        // We explicitly delete this constructor because it's too easy to accidentally 
        // create an array_view to just the first element instead of an entire array.
        array_view(const_pointer s) = delete;

        template<AZStd::size_t N>
        array_view(const AZStd::array<value_type, N>& data)
            : m_begin(data.data())
            , m_end(m_begin + data.size())
        { }

        array_view(const AZStd::vector<value_type>& data)
            : m_begin(data.data())
            , m_end(m_begin + data.size())
        { }

        template<AZStd::size_t N>
        array_view(const AZStd::fixed_vector<value_type, N>& data)
            : m_begin(data.data())
            , m_end(m_begin + data.size())
        { }

        array_view(const array_view&) = default;

        array_view(array_view&& other)
            : array_view(other.m_begin, other.m_end)
        {
#if AZ_DEBUG_BUILD // Clearing the original pointers isn't necessary, but is good for debugging
            other.m_begin = nullptr;
            other.m_end = nullptr;
#endif
        }

        array_view& operator=(const array_view& other) = default;

        array_view& operator=(array_view&& other)
        {
            m_begin = other.m_begin;
            m_end = other.m_end;
#if AZ_DEBUG_BUILD // Clearing the original pointers isn't necessary, but is good for debugging
            other.m_begin = nullptr;
            other.m_end = nullptr;
#endif
            return *this;
        }

        size_type size() const { return m_end - m_begin; }

        bool empty() const { return m_end == m_begin; }

        const_pointer data() const { return m_begin; }

        const_reference operator[](size_type index) const 
        {
            AZ_Assert(index < size(), "index value is out of range");
            return m_begin[index]; 
        }

        void erase() { m_begin = m_end = nullptr; }

        iterator         begin() const { return m_begin; }
        iterator         end() const { return m_end; }
        const_iterator   cbegin() const { return m_begin; }
        const_iterator   cend() const { return m_end; }
        reverse_iterator rbegin() const { return reverse_iterator(m_end); }
        reverse_iterator rend() const { return reverse_iterator(m_begin); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

        friend bool operator==(array_view lhs, array_view rhs)
        {
            return lhs.m_begin == rhs.m_begin && lhs.m_end == rhs.m_end;
        }

        friend bool operator!=(array_view lhs, array_view rhs) { return !(lhs == rhs); }
        friend bool operator< (array_view lhs, array_view rhs) { return lhs.m_begin < rhs.m_begin || lhs.m_begin == rhs.m_begin && lhs.m_end < rhs.m_end; }
        friend bool operator> (array_view lhs, array_view rhs) { return lhs.m_begin > rhs.m_begin || lhs.m_begin == rhs.m_begin && lhs.m_end > rhs.m_end; }
        friend bool operator<=(array_view lhs, array_view rhs) { return lhs == rhs || lhs < rhs; }
        friend bool operator>=(array_view lhs, array_view rhs) { return lhs == rhs || lhs > rhs; }

    private:
        const_pointer m_begin;
        const_pointer m_end;
    };
} // namespace AZStd
