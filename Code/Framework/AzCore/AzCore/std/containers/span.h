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
     * First pass partial implementation of span copied over from array_view. It
     * returns non-const iterator/pointers. first(), last(), and subspan() 
     * are yet to be implemented. It does not maintain storage for the data, 
     * but just holds pointers to mark the beginning and end of the array. 
     * It can be conveniently constructed from a variety of other container 
     * types like array, vector, and fixed_vector. 
     *
     * Example:
     *    Given "void Func(AZStd::span<int> a) {...}" you can call...
     *    - Func({1,2,3});
     *    - AZStd::array<int,3> a = {1,2,3};
     *      Func(a);
     *    - AZStd::vector<int> v = {1,2,3};
     *      Func(v);
     *    - AZStd::fixed_vector<int,10> fv = {1,2,3};
     *      Func(fv);
     *
     * Since the span does not copy and store any data, it is only valid as long as the data used to create it is valid.
     */
    template <class T>
    class span final
    {
    public:
        using element_type = T;
        using value_type = AZStd::remove_cv_t<T>;

        using pointer = T*;
        using const_pointer = const T*;

        using reference = T&;
        using const_reference = const T&;

        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        constexpr span();

        ~span() = default;

        constexpr span(pointer s, size_type length);

        constexpr span(pointer first, pointer last);

        // We explicitly delete this constructor because it's too easy to accidentally 
        // create a span to just the first element instead of an entire array.
        constexpr span(const_pointer s) = delete;

        template<typename Container>
        constexpr span(Container& data);

        template<typename Container>
        constexpr span(const Container& data);

        constexpr span(const span&) = default;

        constexpr span(span&& other);

        constexpr span& operator=(const span& other) = default;

        constexpr span& operator=(span&& other);

        constexpr size_type size() const;

        constexpr bool empty() const;

        constexpr pointer data();
        constexpr const_pointer data() const;

        constexpr const_reference operator[](size_type index) const;
        constexpr reference operator[](size_type index);

        constexpr void erase();

        constexpr iterator         begin();
        constexpr iterator         end();
        constexpr const_iterator   begin() const;
        constexpr const_iterator   end() const;

        constexpr const_iterator   cbegin() const;
        constexpr const_iterator   cend() const;
        
        constexpr reverse_iterator rbegin();
        constexpr reverse_iterator rend();
        constexpr const_reverse_iterator rbegin() const;
        constexpr const_reverse_iterator rend() const;
        
        constexpr const_reverse_iterator crbegin() const;
        constexpr const_reverse_iterator crend() const;

        friend bool operator==(span lhs, span rhs)
        {
            return lhs.m_begin == rhs.m_begin && lhs.m_end == rhs.m_end;
        }

        friend bool operator!=(span lhs, span rhs) { return !(lhs == rhs); }
        friend bool operator< (span lhs, span rhs) { return lhs.m_begin < rhs.m_begin || lhs.m_begin == rhs.m_begin && lhs.m_end < rhs.m_end; }
        friend bool operator> (span lhs, span rhs) { return lhs.m_begin > rhs.m_begin || lhs.m_begin == rhs.m_begin && lhs.m_end > rhs.m_end; }
        friend bool operator<=(span lhs, span rhs) { return lhs == rhs || lhs < rhs; }
        friend bool operator>=(span lhs, span rhs) { return lhs == rhs || lhs > rhs; }

    private:
        pointer m_begin;
        pointer m_end;
    };

} // namespace AZStd

#include <AzCore/std/containers/span.inl>
