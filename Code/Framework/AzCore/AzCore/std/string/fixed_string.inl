/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <cstring>

#include <AzCore/std/algorithm.h>

#include <AzCore/std/string/fixed_string_Platform.inl>

namespace AZStd
{
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string() = default;

    // #2
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(size_type count, Element ch)
    {   // construct from count * ch
        assign(count, ch);
    }

    // #3
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const basic_fixed_string& rhs,
        size_type rhsOffset)
    {   // construct from rhs [rhsOffset, npos)
        assign(rhs, rhsOffset, npos);
    }

    // #3
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const basic_fixed_string& rhs,
        size_type rhsOffset, size_type count)
    {   // construct from rhs [rhsOffset, rhsOffset + count)
        assign(rhs, rhsOffset, count);
    }

    // #4
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const_pointer ptr, size_type count)
    {   // construct from [ptr, ptr + count)
        assign(ptr, count);
    }

    // #5
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const_pointer ptr)
    {   // construct from [ptr, <null>)
        assign(ptr);
    }

    // #6
    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt, typename>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(InputIt first, InputIt last)
    {   // construct from [first, last)
        if (first == last)
        {
            Traits::assign(m_buffer[0], Element()); // terminate
        }
        else
        {
            construct_iter(first, last);
        }
    }

    // #7
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const basic_fixed_string& rhs)
    {
        assign(rhs, size_type(0), npos);
    }

    // #8
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(basic_fixed_string&& rhs)
    {
        assign(AZStd::move(rhs));
    }

    // #9
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(AZStd::initializer_list<Element> ilist)
    {
        assign(ilist.begin(), ilist.end());
    }

    // #10
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T, typename>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const T& convertibleToView)
    {
        AZStd::basic_string_view<Element, Traits> view = convertibleToView;
        assign(view.begin(), view.end());
    }

    // #11
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T, typename>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::basic_fixed_string(const T& convertibleToView,
        size_type rhsOffset, size_type count)
    {
        AZStd::basic_string_view<Element, Traits> view = convertibleToView;
        view = view.substr(rhsOffset, count);
        assign(view.begin(), view.end());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits>::operator AZStd::basic_string_view<Element, Traits>() const
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::begin() -> iterator
    {
        return iterator(m_buffer);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::begin() const -> const_iterator
    {
        return const_iterator(m_buffer);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::cbegin() const -> const_iterator
    {
        return const_iterator(m_buffer);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::end() -> iterator
    {
        return iterator(m_buffer + m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::end() const -> const_iterator
    { return const_iterator(m_buffer + m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::cend() const -> const_iterator
    { return const_iterator(m_buffer + m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return assign(rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(basic_fixed_string&& rhs) -> basic_fixed_string&
    {
        return assign(AZStd::forward<basic_fixed_string>(rhs));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(const_pointer ptr) -> basic_fixed_string&
    {
        return assign(ptr);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(Element ch) -> basic_fixed_string&
    {
        return assign(1, ch);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
    {
        return assign(ilist);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator=(const T& convertibleToView)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return assign(view);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator+=(const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return append(rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator+=(const_pointer ptr) -> basic_fixed_string&
    {
        return append(ptr);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator+=(Element ch) -> basic_fixed_string&
    {
        return append(1, ch);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator+=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
    {
        return append(ilist);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator+=(const T& convertibleToView)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return append(view);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return append(rhs, size_type(0), npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(const basic_fixed_string& rhs,
        size_type rhsOffset, size_type count) -> basic_fixed_string&
    {   // append rhs [rhsOffset, rhsOffset + count)
        AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset!");
        size_type num = rhs.size() - rhsOffset;
        if (num < count)
        {
            count = num;    // trim count to size
        }
        AZSTD_CONTAINER_ASSERT(Capacity - m_size > count && m_size + count >= m_size,
            "New size of fixed_string would be larger than can be represented in internal size_type with size %d",
            std::numeric_limits<internal_size_type>::digits);
        num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {
            pointer data = m_buffer;
            const_pointer rhsData = rhs.m_buffer;
            // make room and append new stuff
            Traits::copy(data + m_size, rhsData + rhsOffset, count);
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element()); // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(const_pointer ptr, size_type count) -> basic_fixed_string&
    {
        // append [ptr, ptr + count)
        pointer data = m_buffer;
        AZSTD_CONTAINER_ASSERT(Capacity - m_size > count && m_size + count >= m_size,
            "New size of fixed_string would be larger than can be represented in internal size_type with size %d",
            std::numeric_limits<internal_size_type>::digits);
        size_type num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {
            // make room and append new stuff
            data = m_buffer;
            Traits::copy(data + m_size, ptr, count);
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return *this;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(const T& convertibleToView)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return append(view.data(), view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(const_pointer ptr) -> basic_fixed_string&
    {
        return append(ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(size_type count, Element ch) -> basic_fixed_string&
    {
        // append count * ch
        AZSTD_CONTAINER_ASSERT(npos - m_size > count, "result is too long");
        size_type num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {
            pointer data = m_buffer;
            // make room and append new stuff using assign
            if (count == 1)
            {
                Traits::assign(*(data + m_size), ch);
            }
            else
            {
                Traits::assign(data + m_size, count, ch);
            }
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {   // append [first, last)
        return append_iter(first, last);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
    {   // append [first, last)
        return append_iter(ilist.begin(), ilist.end());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return assign(rhs, size_type(0), npos);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(basic_fixed_string&& rhs) -> basic_fixed_string&
    {
        if (this != &rhs)
        {
            Traits::copy(m_buffer, rhs.m_buffer, sizeof(m_buffer));
            m_size = rhs.m_size;
            rhs.m_size = 0;
            Traits::assign(rhs.m_buffer[0], Element{});
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(const basic_fixed_string& rhs,
        size_type rhsOffset, size_type count) -> basic_fixed_string&
    {   // assign rhs [rhsOffset, rhsOffset + count)
        AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset");
        size_type num = rhs.size() - rhsOffset;
        if (count < num)
        {
            num = count;    // trim num to size
        }
        if (this == &rhs)
        {
            erase((size_type)(rhsOffset + num));
            erase(size_type(0), rhsOffset);    // substring
        }
        else if (fits_in_capacity(num))
        {   // make room and assign new stuff
            pointer data = m_buffer;
            const_pointer rhsData = rhs.m_buffer;
            Traits::copy(data, rhsData + rhsOffset, num);
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element()); // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(const_pointer ptr, size_type count) -> basic_fixed_string&
    {   // assign [ptr, ptr + count)
        pointer data = m_buffer;
        if (fits_in_capacity(count))
        {
            // make room and assign new stuff
            data = m_buffer;
            if (count > 0)
            {
                Traits::copy(data, ptr, count);
            }
            m_size = static_cast<internal_size_type>(count);
            Traits::assign(data[count], Element());  // terminate
        }
        return *this;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(const T& convertibleToView)
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return assign(view.data(), view.size());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(const_pointer ptr) -> basic_fixed_string&
    {
        return assign(ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(size_type count, Element ch) -> basic_fixed_string&
    {
        // assign count * ch
        AZSTD_CONTAINER_ASSERT(count != npos, "result is too long!");
        if (fits_in_capacity(count))
        {   // make room and assign new stuff
            pointer data = m_buffer;
            if (count == 1)
            {
                Traits::assign(*(data), ch);
            }
            else
            {
                Traits::assign(data, count, ch);
            }
            m_size = static_cast<internal_size_type>(count);
            Traits::assign(data[count], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {
        return assign_iter(first, last);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
    {
        return assign_iter(ilist.begin(), ilist.end());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset,
        const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return insert(offset, rhs, size_type(0), npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset,
        const basic_fixed_string& rhs, size_type rhsOffset, size_type count) -> basic_fixed_string&
    {
        // insert rhs [rhsOffset, rhsOffset + count) at offset
        AZSTD_CONTAINER_ASSERT(m_size >= offset && rhs.m_size >= rhsOffset, "Invalid offset(s)");
        size_type num = rhs.m_size - rhsOffset;
        if (num < count)
        {
            count = num;    // trim _Count to size
        }
        AZSTD_CONTAINER_ASSERT(npos - m_size > count, "Result is too long");
        num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {
            pointer data = m_buffer;
            // make room and insert new stuff
            Traits::copy_backward(data + offset + count, data + offset, m_size - offset);   // empty out hole
            if (this == &rhs)
            {
                offset < rhsOffset ? Traits::copy(data + offset, data + rhsOffset + count, count) : Traits::copy_backward(data + offset, data + rhsOffset, count); // substring
            }
            else
            {
                const_pointer rhsData = rhs.m_buffer;
                Traits::copy(data + offset, rhsData + rhsOffset, count);   // fill hole
            }
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return (*this);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset, const_pointer ptr, size_type count) -> basic_fixed_string&
    {
        // insert [ptr, ptr + count) at offset
        pointer data = m_buffer;
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        AZSTD_CONTAINER_ASSERT(npos - m_size > count, "Result is too long");
        size_type num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {   // make room and insert new stuff
            data = m_buffer;
            Traits::copy_backward(data + offset + count, data + offset, m_size - offset);   // empty out hole
            Traits::copy(data + offset, ptr, count);   // fill hole
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset, const_pointer ptr) -> basic_fixed_string&
    {
        return insert(offset, ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset, const T& convertibleToView)
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return insert(offset, view.data(), view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(size_type offset, size_type count,
        Element ch) -> basic_fixed_string&
    {
        // insert count * ch at offset
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        AZSTD_CONTAINER_ASSERT(npos - m_size > count, "Result is too long");
        size_type num = m_size + count;
        if (count > 0 && fits_in_capacity(num))
        {
            pointer data = m_buffer;
            // make room and insert new stuff
            Traits::copy_backward(data + offset + count, data + offset, m_size - offset);   // empty out hole
            if (count == 1)
            {
                Traits::assign(*(data + offset), ch);
            }
            else
            {
                Traits::assign(data + offset, count, ch);
            }
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(const_iterator insertPos) -> iterator
    {
        return insert(insertPos, Element());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(const_iterator insertPos, Element ch) -> iterator
    {
        const_pointer insertPosPtr = insertPos;

        size_type offset = insertPosPtr - data();
        insert(offset, 1, ch);
        return iterator(data() + offset);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(const_iterator insertPos,
        size_type count, Element ch) -> iterator
    {   // insert count * elem at insertPos
        const_pointer insertPosPtr = insertPos;

        pointer data = m_buffer;
        size_type offset = insertPosPtr - data();
        return insert(offset, count, ch);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(const_iterator insertPos,
        InputIt first, InputIt last)-> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, iterator>
    {   // insert [_First, _Last) at _Where
        return insert_iter(insertPos, first, last);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert(const_iterator insertPos,
        AZStd::initializer_list<Element> ilist) -> iterator
    {   // insert [_First, _Last) at _Where
        return insert_iter(insertPos, ilist.begin(), ilist.end());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::erase(size_type offset, size_type count) -> basic_fixed_string&
    {   // erase elements [offset, offset + count)
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        if (m_size - offset < count)
        {
            count = m_size - offset;    // trim count
        }
        if (count > 0)
        {
            // move elements down
            pointer data = m_buffer;
            Traits::copy(data + offset, data + offset + count, m_size - offset - count);
            m_size = static_cast<internal_size_type>(m_size - count);
            Traits::assign(data[m_size], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::erase(const_iterator erasePos) -> iterator
    {
        const_pointer erasePtr = erasePos;

        // erase element at insertPos
        size_type count = erasePtr - data();
        erase(count, 1);
        return iterator(data() + count);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::erase(const_iterator first, const_iterator last) -> iterator
    {   // erase substring [first, last)
        const_pointer firstPtr = first;
        const_pointer lastPtr = last;

        size_type count = firstPtr - data();
        erase(count, lastPtr - firstPtr);
        return iterator(data() + count);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::clear() -> void
    {
        erase(begin(), end());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        // replace [offset, offset + count) with rhs
        return replace(offset, count, rhs, size_type(0), npos);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const basic_fixed_string& rhs, size_type rhsOffset, size_type rhsCount) -> basic_fixed_string&
    {
        // replace [offset, offset + count) with rhs [rhsOffset, rhsOffset + rhsCount)
        AZSTD_CONTAINER_ASSERT(m_size >= offset && rhs.m_size >= rhsOffset, "Invalid offsets");
        if (m_size - offset < count)
        {
            count = m_size - offset;    // trim count to size
        }
        size_type num = rhs.m_size - rhsOffset;
        if (num < rhsCount)
        {
            rhsCount = num; // trim rhsCount to size
        }
        AZSTD_CONTAINER_ASSERT(npos - rhsCount > m_size - count, "Result is too long");

        size_type nm = m_size - count - offset; // length of preserved tail
        size_type newSize = m_size + rhsCount - count;
        if (fits_in_capacity(newSize))
        {
            pointer data = m_buffer;
            const_pointer rhsData = rhs.m_buffer;

            if (this != &rhs)
            {   // no overlap, just move down and copy in new stuff
                Traits::copy_backward(data + offset + rhsCount, data + offset + count, nm);   // empty hole
                Traits::copy(data + offset, rhsData + rhsOffset, rhsCount); // fill hole
            }
            else if (rhsCount <= count)
            {   // hole doesn't get larger, just copy in substring
                Traits::copy(data + offset, data + rhsOffset, rhsCount);    // fill hole
                Traits::copy_backward(data + offset + rhsCount, data + offset + count, nm);   // move tail down
            }
            else if (rhsOffset <= offset)
            {   // hole gets larger, substring begins before hole
                Traits::copy_backward(data + offset + rhsCount, data + offset + count, nm);   // move tail down
                Traits::copy(data + offset, data + rhsOffset, rhsCount);   // fill hole
            }
            else if (offset + count <= rhsOffset)
            {   // hole gets larger, substring begins after hole
                Traits::copy_backward(data + offset + rhsCount, data + offset + count, nm);   // move tail down
                Traits::copy(data + offset, data + (rhsOffset + rhsCount - count), rhsCount);   // fill hole
            }
            else
            {   // hole gets larger, substring begins in hole
                Traits::copy(data + offset, data + rhsOffset, count);   // fill old hole
                Traits::copy_backward(data + offset + rhsCount, data + offset + count, nm);   // move tail down
                Traits::copy(data + offset + count, data + rhsOffset + rhsCount, rhsCount - count); // fill rest of new hole
            }

            m_size = static_cast<internal_size_type>(newSize);
            Traits::assign(data[newSize], Element());  // terminate
        }
        return *this;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const T& convertibleToView, size_type rhsOffset, size_type rhsCount)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        view = view.substr(rhsOffset, rhsCount);
        return replace(offset, count, view.data(), view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const_pointer ptr, size_type ptrCount) -> basic_fixed_string&
    {
        pointer data = m_buffer;
        // replace [offset, offset + count) with [ptr, ptr + ptrCount)
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        if (m_size - offset < count)
        {
            count = m_size - offset;    // trim _N0 to size
        }
        AZSTD_CONTAINER_ASSERT(npos - ptrCount > m_size - count, "Result too long");

        size_type nm = m_size - count - offset;
        if (ptrCount < count)
        {
            Traits::copy(data + offset + ptrCount, data + offset + count, nm);  // smaller hole, move tail up
        }
        size_type num = m_size + ptrCount - count;
        if ((0 != ptrCount || 0 != count) && fits_in_capacity(num))
        {
            data = m_buffer;
            // make room and rearrange
            if (count < ptrCount)
            {
                Traits::copy_backward(data + offset + ptrCount, data + offset + count, nm);   // move tail down
            }

            if (ptrCount > 0)
            {
                Traits::copy(data + offset, ptr, ptrCount);    // fill hole
            }

            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const_pointer ptr) -> basic_fixed_string&
    {
        return replace(offset, count, ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        const T& convertibleToView)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return replace(offset, count, view.data(), view.size());
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(size_type offset, size_type count,
        size_type num, Element ch) -> basic_fixed_string&
    {   // replace [offset, offset + count) with num * ch
        AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
        if (m_size - offset < count)
        {
            count = m_size - offset;    // trim count to size
        }
        AZSTD_CONTAINER_ASSERT(npos - num > m_size - count, "Result is too long");
        size_type nm = m_size - count - offset;

        pointer data = m_buffer;

        if (num < count)
        {
            Traits::copy(data + offset + num, data + offset + count, nm); // smaller hole, move tail up
        }
        size_type numToGrow = m_size + num - count;
        if ((0 < num || 0 < count) && fits_in_capacity(numToGrow))
        {   // make room and rearrange
            data = m_buffer;
            if (count < num)
            {
                Traits::copy_backward(data + offset + num, data + offset + count, nm); // move tail down
            }
            if (count == 1)
            {
                Traits::assign(*(data + offset), ch);
            }
            else
            {
                Traits::assign(data + offset, num, ch);
            }
            m_size = static_cast<internal_size_type>(numToGrow);
            Traits::assign(data[numToGrow], Element());  // terminate
        }
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        const basic_fixed_string& rhs) -> basic_fixed_string&
    {
        return replace(AZStd::distance(cbegin(), first), AZStd::distance(first, last), rhs);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        const_pointer ptr, size_type count) -> basic_fixed_string&
    {   // replace [first, last) with [ptr, ptr + count)
        return replace(AZStd::distance(cbegin(), first), AZStd::distance(first, last), ptr, count);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        const_pointer ptr) -> basic_fixed_string&
    {   // replace [first, last) with [ptr, <null>)
        return replace(AZStd::distance(cbegin(), first), AZStd::distance(first, last), ptr);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        const T& convertibleToView)
        ->AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return replace(first, last, view.data(), view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        size_type count, Element ch) -> basic_fixed_string&
    {   // replace [first, last) with count * ch
        const_pointer firstPtr = first;
        const_pointer lastPtr = last;

        pointer data = m_buffer;
        return replace(firstPtr - data, lastPtr - firstPtr, count, ch);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        InputIt first2, InputIt last2) -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {   // replace [first, last) with [first2,last2)
        return replace_iter(first, last, first2, last2);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace(const_iterator first, const_iterator last,
        AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
    {   // replace [first, last) with [first2,last2)
        return replace_iter(first, last, ilist.begin(), ilist.end());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::at(size_type offset) -> reference
    {
        // subscript mutable sequence with checking
        AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
        pointer data = m_buffer;
        return data[offset];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::at(size_type offset) const -> const_reference
    {
        // subscript non mutable sequence with checking
        AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
        const_pointer data = m_buffer;
        return data[offset];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator[](size_type offset) -> reference
    {
        // subscript mutable sequence with checking
        AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
        pointer data = m_buffer;
        return data[offset];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::operator[](size_type offset) const -> const_reference
    {
        // subscript non mutable sequence with checking
        AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
        const_pointer data = m_buffer;
        return data[offset];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::front() -> reference
    {
        AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::front - string is empty!");
        pointer data = m_buffer;
        return data[0];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::front() const -> const_reference
    {
        AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::front - string is empty!");
        const_pointer data = m_buffer;
        return data[0];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::back() -> reference
    {
        AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::back - string is empty!");
        pointer data = m_buffer;
        return data[m_size - 1];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::back() const -> const_reference
    {
        AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::back - string is empty!");
        const_pointer data = m_buffer;
        return data[m_size - 1];
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::push_back(Element ch) -> void
    {
        insert(end(), ch);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::pop_back() -> void
    {
        if (m_size > 0)
        {
            pointer data = m_buffer;
            --m_size;
            Traits::assign(data[m_size], Element());  // terminate
        }
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::c_str() const -> const_pointer
    {
        return m_buffer;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::data() const -> const_pointer
    {
        return m_buffer;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::data() -> pointer
    {
        return m_buffer;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::length() const -> size_type
    {
        return m_size;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::size() const -> size_type
    {
        return m_size;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::capacity() const -> size_type
    {
        return Capacity;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::max_size() const -> size_type
    {
        // Maximum size of the fixed string is the MaxElementCount template argument
        return MaxElementCount;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::resize(size_type newSize) -> void
    {
        resize(newSize, Element());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::resize_no_construct(size_type newSize) -> void
    {
        if (newSize <= m_size)
        {
            erase(newSize);
        }
        else
        {
            reserve(newSize);
            pointer data = m_buffer;
            m_size = static_cast<internal_size_type>(newSize);
            Traits::assign(data[m_size], Element());  // terminate
        }
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::resize(size_type newSize, Element ch) -> void
    {   // determine new length, padding with ch elements as needed
        if (newSize <= m_size)
        {
            erase(newSize);
        }
        else
        {
            append(newSize - m_size, ch);
        }
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::reserve(size_type newCapacity) -> void
    {
        // Validates that a new Capacity isn't being asked for fixed_string that is larger
        // than the fixed size
        fits_in_capacity(newCapacity);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::empty() const -> bool
    {
        return m_size == 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::copy(Element* dest, size_type count, size_type offset) const -> size_type
    {
        // copy [offset, offset + count) to [dest, dest + count)
        // assume there is enough space in _Ptr
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        if (m_size - offset < count)
        {
            count = m_size - offset;
        }
        const_pointer data = m_buffer;
        Traits::copy(dest /*, destSize*/, data + offset, count);
        return count;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::swap(basic_fixed_string& rhs) -> void
    {   // exchange contents with _Right
        if (this == &rhs)
        {
            return;
        }

        Element temp[Capacity + 1]{};
        internal_size_type temp_size = rhs.m_size;
        Traits::copy(temp, rhs.m_buffer, rhs.m_size);
        Traits::copy(rhs.m_buffer, m_buffer, m_size);
        Traits::copy(m_buffer, temp, temp_size);

        rhs.m_size = m_size;
        m_size = temp_size;

        // Null-Terminate the swapped buffers
        Traits::assign(m_buffer[m_size], Element{ 0 });
        Traits::assign(rhs.m_buffer[rhs.m_size], Element{ 0 });
    }

    // C++23 contains
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::contains(const basic_fixed_string& other) const -> bool
    {
        return find(other) != npos;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::contains(value_type c) const -> bool
    {
        return find(c) != npos;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::contains(const_pointer s) const -> bool
    {
        return find(s) != npos;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {
        return find(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {
        return StringInternal::find<traits_type>(data(), size(), ptr, offset, count, npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find(const_pointer ptr, size_type offset) const -> size_type
    {
        return find(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find(Element ch, size_type offset) const -> size_type
    {
        return find(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return find(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rfind(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {
        return rfind(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rfind(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {   // look for [ptr, ptr + count) beginning before offset
        return StringInternal::rfind<traits_type>(data(), size(), ptr, offset, count, npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rfind(const_pointer ptr, size_type offset) const -> size_type
    {
        return rfind(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rfind(Element ch, size_type offset) const -> size_type
    {
        return rfind(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::rfind(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return rfind(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_of(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {
        return find_first_of(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {
        return StringInternal::find_first_of<traits_type>(data(), size(), ptr, offset, count, npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_of(const_pointer ptr, size_type offset) const -> size_type
    {
        return find_first_of(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_of(Element ch, size_type offset) const -> size_type
    {
        return find(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_of(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return find_first_of(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_not_of(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {   // look for none of rhs at or after offset
        return find_first_not_of(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {
        return StringInternal::find_first_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_not_of(const_pointer ptr, size_type offset) const -> size_type
    {
        return find_first_not_of(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_not_of(Element ch, size_type offset) const -> size_type
    {
        return find_first_not_of(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_first_not_of(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return find_first_not_of(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_of(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {
        return find_last_of(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {
        return StringInternal::find_last_of<traits_type>(data(), size(), ptr, offset, count, npos);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_of(const_pointer ptr, size_type offset) const -> size_type
    {
        return find_last_of(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_of(Element ch, size_type offset) const -> size_type
    {
        return rfind(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_of(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return find_last_of(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_not_of(const basic_fixed_string& rhs, size_type offset) const -> size_type
    {
        return find_last_not_of(rhs.m_buffer, offset, rhs.m_size);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
    {
        return StringInternal::find_last_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_not_of(const_pointer ptr, size_type offset) const -> size_type
    {
        return find_last_not_of(ptr, offset, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_not_of(Element ch, size_type offset) const -> size_type
    {
        return find_last_not_of(&ch, offset, 1);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::find_last_not_of(const T& convertibleToView, size_type offset) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>
    {
        basic_string_view<Element, Traits> view = convertibleToView;
        return find_last_not_of(view.data(), offset, view.size());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::substr(size_type offset, size_type count) const -> basic_fixed_string
    {
        // return [offset, offset + count) as new string
        return basic_fixed_string(*this, offset, count);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(const basic_fixed_string& rhs) const -> int
    {
        const_pointer rhsData = rhs.m_buffer;
        return compare(0, m_size, rhsData, rhs.m_size);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(size_type offset, size_type count, const basic_fixed_string& rhs) const -> int
    {   // compare [offset, off + count) with rhs
        return compare(offset, count, rhs, 0, npos);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(size_type offset, size_type count,
        const basic_fixed_string& rhs, size_type rhsOffset, size_type rhsCount) const -> int
    {
        // compare [offset, offset + count) with rhs [rhsOffset, rhsOffset + rhsCount)
        AZSTD_CONTAINER_ASSERT(rhs.m_size >= rhsOffset, "Invalid offset");
        if (rhs.m_size - rhsOffset < rhsCount)
        {
            rhsCount = rhs.m_size - rhsOffset;  // trim rhsCount to size
        }
        const_pointer rhsData = rhs.m_buffer;
        return compare(offset, count, rhsData + rhsOffset, rhsCount);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(const_pointer ptr) const -> int
    {
        return compare(0, m_size, ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(size_type offset, size_type count, const_pointer ptr) const -> int
    {
        return compare(offset, count, ptr, Traits::length(ptr));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::compare(size_type offset, size_type count, const_pointer ptr, size_type ptrCount) const -> int
    {
        // compare [offset, offset + _N0) with [_Ptr, _Ptr + _Count)
        AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
        if (m_size - offset < count)
        {
            count = m_size - offset;    // trim count to size
        }
        const_pointer data = m_buffer;
        int ans = Traits::compare(data + offset, ptr, count < ptrCount ? count : ptrCount);
        return ans != 0 ? ans : count < ptrCount ? -1 : count == ptrCount ? 0 : 1;
    }

    // starts_with
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::starts_with(const T& prefix) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, bool>
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::starts_with(value_type prefix) const -> bool
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::starts_with(const_pointer prefix) const -> bool
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
    }

    // ends_with
    template<class Element, size_t MaxElementCount, class Traits>
    template<typename T>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::ends_with(const T& suffix) const
        -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, bool>
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::ends_with(value_type suffix) const -> bool
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::ends_with(const_pointer suffix) const -> bool
    {
        return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::leak_and_reset() -> void
    {
        m_size = 0;
        Traits::assign(m_buffer[0], Element());
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format_arg(const char* format, va_list argList)
    {
        basic_fixed_string<char, MaxElementCount, char_traits<char>> result;
        // On Windows, AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS is set, so azvsnprintf calls _vsnprintf_s.
        // _vsnprintf_s, unlike vsnprintf, will fail and return -1 when the size argument is 0.
        // azvscprintf uses the proper function(_vscprintf or vsnprintf) on the given platform
        va_list argListCopy; //<- Depending on vprintf implementation va_list may be consumed, so send a copy
        va_copy(argListCopy, argList);
        const int len = azvscprintf(format, argListCopy);
        va_end(argListCopy);
        if (len > 0)
        {
            result.resize(len);
            va_copy(argListCopy, argList);
            [[maybe_unused]] const int bytesPrinted = azvsnprintf(result.data(), result.size() + 1, format, argListCopy);
            va_end(argListCopy);
            AZ_Assert(bytesPrinted >= 0, "azvsnprintf error! Format string: \"%s\"", format);
            AZ_Assert(static_cast<size_t>(bytesPrinted) == result.size(), "azvsnprintf failed to print all bytes! Format string: \"%s\", bytesPrinted=%i/%i",
                format, bytesPrinted, len);
        }
        return result;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format(const char* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        basic_fixed_string<char, MaxElementCount, char_traits<char>> result = format_arg(format, mark);
        va_end(mark);
        return result;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format(const wchar_t* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        basic_fixed_string<wchar_t, MaxElementCount, char_traits<wchar_t>> result = format_arg(format, mark);
        va_end(mark);
        return result;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::construct_iter(InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>>
    {
        // initialize from [first, last), input iterators
        for (; first != last; ++first)
        {
            append((size_type)1, (Element)* first);
        }
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::append_iter(InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {   // append [first, last), input iterators
        return replace(end(), end(), first, last);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::assign_iter(InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {
        return replace(begin(), end(), first, last);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::insert_iter(const_iterator insertPos, InputIt first,
        InputIt last) -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, iterator>
    {   // insert [first, last) at insertPos, input iterators
        difference_type offset = insertPos - cbegin();
        replace(insertPos, insertPos, first, last);
        return iterator(m_buffer + offset);
    }

    template<class Element, size_t MaxElementCount, class Traits>
    template<class InputIt>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::replace_iter(const_iterator first, const_iterator last,
        InputIt first2, InputIt last2) -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>
    {   // replace [first, last) with [first2, last2), input iterators
        basic_fixed_string rhs(first2, last2);
        replace(first, last, rhs);
        return *this;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr auto basic_fixed_string<Element, MaxElementCount, Traits>::fits_in_capacity(size_type newSize)-> bool
    {
        AZSTD_CONTAINER_ASSERT(newSize <= Capacity, "New string size of %zu cannot fit in a fixed_string of size %zu", newSize, Capacity);
        return newSize <= Capacity;
    }

    // Free functions
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr void swap(basic_fixed_string<Element, MaxElementCount, Traits>& left,
        basic_fixed_string<Element, MaxElementCount, Traits>& right)
    {
        left.swap(right);
    }

    // 21.4.8
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        basic_fixed_string<Element, MaxElementCount, Traits> result(lhs);
        result.append(rhs);
        return result;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        basic_fixed_string<Element, MaxElementCount, Traits> result(lhs);
        result.append(rhs);
        return result;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        Element rhs)
    {
        basic_fixed_string<Element, MaxElementCount, Traits> result(lhs);
        result.push_back(rhs);
        return result;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        basic_fixed_string<Element, MaxElementCount, Traits> result(lhs);
        result.append(rhs);
        return result;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(Element lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        basic_fixed_string<Element, MaxElementCount, Traits> result(1, lhs);
        result.append(rhs);
        return result;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs)
    {
        if (rhs.size() < lhs.capacity() - lhs.size() || lhs.size() > rhs.capacity() - rhs.size())
        {
            return AZStd::move(lhs.append(rhs)); // No need to create copy since lhs is an r-value
        }
        else
        {
            using size_type = typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;
            return AZStd::move(rhs.insert(size_type(0), AZStd::move(lhs)));
        }
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return AZStd::move(lhs.append(rhs));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        const Element* rhs)
    {
        return AZStd::move(lhs.append(rhs));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        Element rhs)
    {
        lhs.push_back(rhs);
        return AZStd::move(lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs)
    {
        using size_type = typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;
        return AZStd::move(rhs.insert(size_type(0), lhs));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const Element* lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs)
    {
        using size_type = typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;
        return AZStd::move(rhs.insert(size_type(0), lhs));
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(Element lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs)
    {
        using size_type = typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;
        return AZStd::move(rhs.insert(size_type(0), 1, lhs));
    }

    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator==(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return lhs.compare(rhs) == 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator==(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return rhs.compare(lhs) == 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator==(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return lhs.compare(rhs) == 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator!=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator==(lhs, rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator!=(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator==(lhs, rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator!=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return !operator==(lhs, rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator< (const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return lhs.compare(rhs) < 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator< (const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return lhs.compare(rhs) < 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator< (const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return rhs.compare(lhs) > 0;
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator> (const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator> (const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator> (const Element* lhs, const basic_fixed_string<Element, MaxElementCount,
        Traits>& rhs)
    {
        return operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator<=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator<=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return !operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator<=(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator<(rhs, lhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator>=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator<(lhs, rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator>=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs)
    {
        return !operator<(lhs, rhs);
    }
    template<class Element, size_t MaxElementCount, class Traits>
    inline constexpr bool operator>=(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs)
    {
        return !operator<(lhs, rhs);
    }

    template<class Element, size_t MaxElementCount, class Traits, class U>
    inline constexpr auto erase(basic_fixed_string<Element, MaxElementCount, Traits>& container, const U& element)
        -> typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type
    {
        auto iter = AZStd::remove(container.begin(), container.end(), element);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }

    template<class Element, size_t MaxElementCount, class Traits, class Predicate>
    inline constexpr auto erase_if(basic_fixed_string<Element, MaxElementCount, Traits>& container, Predicate predicate)
        -> typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type
    {
        auto iter = AZStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    struct hash<basic_fixed_string<Element, MaxElementCount, Traits>>
    {
        inline constexpr size_t operator()(const basic_fixed_string<Element, MaxElementCount, Traits>& value) const
        {
            return hash_string(value.begin(), value.length());
        }
    };
} // namespace AZStd
