/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include <AzCore/std/base.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/containers_concepts.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/string/string_view.h>

namespace AZStd::Internal
{
    template<class Element, class Traits, class T>
    concept fixed_string_view_compatible =
        is_convertible_v<const T&, basic_string_view<Element, Traits>>
        && (!is_convertible_v<const T&, const Element*>);
}

namespace AZStd
{
    //! Implementation of string based class which stores an the characters in internal buffer that is determined at compile time
    //! The basic_fixed_string class performs no heap allocations.
    //! The API of the this class follows the same API as the C++20 std::string API with the exception that any API related to allocators have been removed
    //! This class performs no heap allocations and is constexpr with the exception of the basic_fixed_string::format functions which rely on the
    //! vs(w)nprintf function under the hood which are not constexpr.
    //! Due to being non-allocating this class is also safe to use in static memory unlike AZStd::string
    template<class Element, size_t MaxElementCount, class Traits = char_traits<Element>>
    class basic_fixed_string
    {
        static_assert(MaxElementCount != std::numeric_limits<size_t>::max(), "The Maximum element count must be less than the maximum value that can fit within a size_t");
    public:
        using pointer = Element*;
        using const_pointer = const Element*;

        using reference = Element&;
        using const_reference = const Element&;
        using difference_type = AZStd::ptrdiff_t;
        using size_type = AZStd::size_t;

        using iterator = pointer;
        using const_iterator = const_pointer;

        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;
        using value_type = Element;
        using traits_type = Traits;
        using allocator_type = void;

        inline static constexpr size_type npos = size_type(-1);

    private:
        // Optimize the stored size type based on the number of bytes needed to store up to MaxElementCount bytes
        using internal_size_type =
            AZStd::conditional_t<(MaxElementCount < std::numeric_limits<uint8_t>::max()), uint8_t,
            AZStd::conditional_t<(MaxElementCount < std::numeric_limits<uint16_t>::max()), uint16_t,
            AZStd::conditional_t<(MaxElementCount < std::numeric_limits<uint32_t>::max()), uint32_t, size_type
            >>>;

    public:
        constexpr basic_fixed_string() = default;

        // #2
        constexpr basic_fixed_string(size_type count, Element ch)
        {   // construct from count * ch
            assign(count, ch);
        }

        // #3
        constexpr basic_fixed_string(const basic_fixed_string& rhs,
            size_type rhsOffset) : basic_fixed_string(rhs, rhsOffset, npos)
        {   // construct from rhs [rhsOffset, npos)
        }

        // #3
        constexpr basic_fixed_string(const basic_fixed_string& rhs,
            size_type rhsOffset, size_type count)
        {   // construct from rhs [rhsOffset, rhsOffset + count)
            AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset");
            size_type num = AZStd::min(count, rhs.size() - rhsOffset);

            // make room and assign new stuff
            pointer data = m_buffer;
            const_pointer rhsData = rhs.m_buffer;
            Traits::copy(data, rhsData + rhsOffset, num);
            m_size = static_cast<internal_size_type>(num);
            Traits::assign(data[num], Element()); // terminate
        }

        // #4
        constexpr basic_fixed_string(const_pointer ptr, size_type count)
        {   // construct from [ptr, ptr + count)
            assign(ptr, count);
        }

        // #5
        constexpr basic_fixed_string(const_pointer ptr)
        {   // construct from [ptr, <null>)
            assign(ptr);
        }

        // #6
        template<input_iterator InputIt>
            requires (!is_convertible_v<InputIt, size_t>)
        constexpr basic_fixed_string(InputIt first, InputIt last)
        {   // construct from [first, last)
            assign(first, last);
        }

        // https://eel.is/c++draft/strings#string.cons-18
        template<Internal::container_compatible_range<Element> R>
        constexpr basic_fixed_string(from_range_t, R&& rg)
        {
            assign_range(AZStd::forward<R>(rg));
        }

        // #7
        constexpr basic_fixed_string(const basic_fixed_string& rhs)
          : basic_fixed_string(rhs, size_type(0), npos)
        {
        }

        // #8
        constexpr basic_fixed_string(basic_fixed_string&& rhs)
        {
            Traits::copy(m_buffer, rhs.m_buffer, rhs.size() + 1);
            m_size = rhs.m_size;
            rhs.m_size = 0;
            Traits::assign(rhs.m_buffer[0], Element{});
        }

        // #9
        constexpr basic_fixed_string(AZStd::initializer_list<Element> ilist)
        {
            assign(ilist.begin(), ilist.end());
        }

        // #10
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr explicit basic_fixed_string(const T& convertibleToView)
        {
            assign(convertibleToView);
        }

        // #11
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr basic_fixed_string(const T& convertibleToView,
            size_type rhsOffset, size_type count)
        {
            AZStd::basic_string_view<Element, Traits> view = convertibleToView;
            view = view.substr(rhsOffset, count);
            assign(view.begin(), view.end());
        }

        // #12
        constexpr basic_fixed_string(AZStd::nullptr_t) = delete;

        constexpr operator AZStd::basic_string_view<Element, Traits>() const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size());
        }

        constexpr auto begin() -> iterator
        {
            return iterator(m_buffer);
        }
        constexpr auto begin() const -> const_iterator
        {
            return const_iterator(m_buffer);
        }
        constexpr auto cbegin() const -> const_iterator
        {
            return const_iterator(m_buffer);
        }
        constexpr auto end() -> iterator
        {
            return iterator(m_buffer + m_size);
        }
        constexpr auto end() const -> const_iterator
        { return const_iterator(m_buffer + m_size);
        }
        constexpr auto cend() const -> const_iterator
        { return const_iterator(m_buffer + m_size);
        }
        constexpr auto rbegin() -> reverse_iterator
        {
            return reverse_iterator(end());
        }
        constexpr auto rbegin() const -> const_reverse_iterator
        {
            return const_reverse_iterator(end());
        }
        constexpr auto crbegin() const -> const_reverse_iterator
        {
            return const_reverse_iterator(end());
        }
        constexpr auto rend() -> reverse_iterator
        {
            return reverse_iterator(begin());
        }
        constexpr auto rend() const -> const_reverse_iterator
        {
            return const_reverse_iterator(begin());
        }
        constexpr auto crend() const -> const_reverse_iterator
        {
            return const_reverse_iterator(begin());
        }

        constexpr auto operator=(const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return assign(rhs);
        }
        constexpr auto operator=(basic_fixed_string&& rhs) -> basic_fixed_string&
        {
            return assign(AZStd::forward<basic_fixed_string>(rhs));
        }
        constexpr auto operator=(const_pointer ptr) -> basic_fixed_string&
        {
            return assign(ptr);
        }
        constexpr auto operator=(Element ch) -> basic_fixed_string&
        {
            return assign(1, ch);
        }
        constexpr auto operator=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
        {
            return assign(ilist);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto operator=(const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return assign(view);
        }

        constexpr auto operator=(AZStd::nullptr_t) -> basic_fixed_string& = delete;

        constexpr auto operator+=(const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return append(rhs);
        }
        constexpr auto operator+=(const_pointer ptr) -> basic_fixed_string&
        {
            return append(ptr);
        }
        constexpr auto operator+=(Element ch) -> basic_fixed_string&
        {
            return append(1, ch);
        }
        constexpr auto operator+=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
        {
            return append(ilist);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto operator+=(const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return append(view);
        }

        constexpr auto append(const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return append(rhs, size_type(0), npos);
        }
        constexpr auto append(const basic_fixed_string& rhs,
            size_type rhsOffset, size_type count) -> basic_fixed_string&
        {   // append rhs [rhsOffset, rhsOffset + count)
            AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset!");
            size_type num = rhs.size() - rhsOffset;
            if (num < count)
            {
                count = num;    // trim count to size
            }
            AZSTD_CONTAINER_ASSERT(capacity() - size() >= count,
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

        constexpr auto append(const_pointer ptr, size_type count) -> basic_fixed_string&
        {
            // append [ptr, ptr + count)
            pointer data = m_buffer;
            AZSTD_CONTAINER_ASSERT(capacity() - size() >= count,
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
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto append(const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return append(view.data(), view.size());
        }

        constexpr auto append(const_pointer ptr) -> basic_fixed_string&
        {
            return append(ptr, Traits::length(ptr));
        }
        constexpr auto append(size_type count, Element ch) -> basic_fixed_string&
        {
            // append count * ch
            AZSTD_CONTAINER_ASSERT(npos - m_size > count, "result is too long");
            size_type num = m_size + count;
            if (count > 0 && fits_in_capacity(num))
            {
                pointer data = m_buffer;
                Traits::assign(data + m_size, count, ch);
                m_size = static_cast<internal_size_type>(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        template<input_iterator InputIt>
            requires (!is_convertible_v<InputIt, size_t>)
        constexpr auto append(InputIt first, InputIt last)
            -> basic_fixed_string&
        {
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                return append(AZStd::to_address(first), ranges::distance(first, last));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // Input Iterator pointer type doesn't match the const_pointer type
                // So the elements need to be appended one by one into the buffer
                size_type newSize = m_size + ranges::distance(first, last);
                if (fits_in_capacity(newSize))
                {
                    for (size_t updateIndex = m_size; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(m_buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_size = static_cast<internal_size_type>(newSize);
                    Traits::assign(m_buffer[newSize], Element());  // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_fixed_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                return append(inputCopy.c_str(), inputCopy.size());
            }
        }

        template<Internal::container_compatible_range<Element> R>
        constexpr auto append_range(R&& rg)
            -> basic_fixed_string&
        {
            return append(basic_fixed_string(from_range, AZStd::forward<R>(rg)));
        }

        constexpr auto append(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
        {
            return append(ilist.begin(), ilist.size());
        }

        constexpr auto assign(const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return assign(rhs, size_type(0), npos);
        }

        constexpr auto assign(basic_fixed_string&& rhs) -> basic_fixed_string&
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

        constexpr auto assign(const basic_fixed_string& rhs,
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

        constexpr auto assign(const_pointer ptr, size_type count) -> basic_fixed_string&
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
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto assign(const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return assign(view.data(), view.size());
        }
        constexpr auto assign(const_pointer ptr) -> basic_fixed_string&
        {
            return assign(ptr, Traits::length(ptr));
        }
        constexpr auto assign(size_type count, Element ch) -> basic_fixed_string&
        {
            // assign count * ch
            if (fits_in_capacity(count))
            {   // make room and assign new stuff
                pointer data = m_buffer;
                Traits::assign(data, count, ch);
                m_size = static_cast<internal_size_type>(count);
                Traits::assign(data[count], Element());  // terminate
            }
            return *this;
        }

        template<input_iterator InputIt>
            requires (!is_convertible_v<InputIt, size_t>)
        constexpr auto assign(InputIt first, InputIt last)
            -> basic_fixed_string&
        {
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                return assign(AZStd::to_address(first), ranges::distance(first, last));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // Input Iterator pointer type doesn't match the const_pointer type
                // So the elements need to be assigned one by one into the buffer
                size_type newSize = ranges::distance(first, last);
                if (fits_in_capacity(newSize))
                {
                    for (size_t updateIndex = 0; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(m_buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_size = static_cast<internal_size_type>(newSize);
                    Traits::assign(m_buffer[newSize], Element());  // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_fixed_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                return assign(inputCopy.c_str(), inputCopy.size());
            }
        }

        template<Internal::container_compatible_range<Element> R>
        constexpr auto assign_range(R&& rg)
            -> basic_fixed_string&
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                return assign(ranges::begin(rangeView), ranges::end(rangeView));
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                return assign(ranges::begin(rangeView), ranges::end(rangeView));
            }
        }

        constexpr auto assign(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
        {
            return assign(ilist.begin(), ilist.size());
        }

        constexpr auto insert(size_type offset,
            const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return insert(offset, rhs, size_type(0), npos);
        }
        constexpr auto insert(size_type offset,
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

        constexpr auto insert(size_type offset, const_pointer ptr, size_type count) -> basic_fixed_string&
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

        constexpr auto insert(size_type offset, const_pointer ptr) -> basic_fixed_string&
        {
            return insert(offset, ptr, Traits::length(ptr));
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto insert(size_type offset, const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return insert(offset, view.data(), view.size());
        }

        constexpr auto insert(size_type offset, size_type count,
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
                Traits::assign(data + offset, count, ch);
                m_size = static_cast<internal_size_type>(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        constexpr auto insert(const_iterator insertPos) -> iterator
        {
            return insert(insertPos, Element());
        }

        constexpr auto insert(const_iterator insertPos, Element ch) -> iterator
        {
            const_pointer insertPosPtr = insertPos;

            size_type offset = insertPosPtr - data();
            insert(offset, 1, ch);
            return iterator(data() + offset);
        }

        constexpr auto insert(const_iterator insertPos,
            size_type count, Element ch) -> iterator
        {   // insert count * elem at insertPos
            const_pointer insertPosPtr = insertPos;

            size_type offset = insertPosPtr - data();
            insert(offset, count, ch);
            return data() + offset;
        }

        template<input_iterator InputIt>
            requires (!is_convertible_v<InputIt, size_t>)
        constexpr auto insert(const_iterator insertPos,
            InputIt first, InputIt last) -> iterator
        {   // insert [_First, _Last) at _Where
            size_type insertOffset = ranges::distance(cbegin(), insertPos);
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                insert(insertOffset, AZStd::to_address(first), ranges::distance(first, last));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // Input Iterator pointer type doesn't match the const_pointer type
                // So the elements need to be inserted one by one into the buffer
                size_type count = ranges::distance(first, last);
                size_type newSize = m_size + count;
                if (fits_in_capacity(newSize))
                {
                    Traits::copy_backward(m_buffer + insertOffset + count, m_buffer + insertOffset, m_size - insertOffset); // empty out hole
                    for (size_t updateIndex = insertOffset; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(m_buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_size = static_cast<internal_size_type>(newSize);
                    Traits::assign(m_buffer[newSize], Element()); // terminate
                }
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_fixed_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                insert(insertOffset, inputCopy.c_str(), inputCopy.size());
            }
            return begin() + insertOffset;
        }

        template<Internal::container_compatible_range<Element> R>
        constexpr auto insert_range(const_iterator insertPos, R&& rg)
            -> iterator
        {
            size_t offset = insertPos - begin();
            insert(insertPos - begin(), basic_fixed_string(from_range, AZStd::forward<R>(rg)));
            return begin() + offset;
        }

        constexpr auto insert(const_iterator insertPos,
            AZStd::initializer_list<Element> ilist) -> iterator
        {   // insert [_First, _Last) at _Where
            return insert(insertPos, ilist.begin(), ilist.end());
        }

        constexpr auto erase(size_type offset = 0, size_type count = npos) -> basic_fixed_string&
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
                Traits::move(data + offset, data + offset + count, m_size - offset - count);
                m_size = static_cast<internal_size_type>(m_size - count);
                Traits::assign(data[m_size], Element());  // terminate
            }
            return *this;
        }

        constexpr auto erase(const_iterator erasePos) -> iterator
        {
            const_pointer erasePtr = erasePos;

            // erase element at insertPos
            size_type count = erasePtr - data();
            erase(count, 1);
            return iterator(data() + count);
        }

        constexpr auto erase(const_iterator first, const_iterator last) -> iterator
        {   // erase substring [first, last)
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;

            size_type count = firstPtr - data();
            erase(count, lastPtr - firstPtr);
            return iterator(data() + count);
        }

        constexpr auto clear() -> void
        {
            erase(begin(), end());
        }
        constexpr auto replace(size_type offset, size_type count,
            const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            // replace [offset, offset + count) with rhs
            return replace(offset, count, rhs.c_str(), rhs.size());
        }

        constexpr auto replace(size_type offset, size_type count,
            const basic_fixed_string& rhs, size_type rhsOffset, size_type rhsCount) -> basic_fixed_string&
        {
            // replace [offset, offset + count) with rhs [rhsOffset, rhsOffset + rhsCount)
            return replace(offset, count, rhs.c_str() + rhsOffset, AZStd::min(rhsCount, rhs.size() - rhsOffset));
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto replace(size_type offset, size_type count,
            const T& convertibleToView, size_type rhsOffset, size_type rhsCount = npos)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            view = view.substr(rhsOffset, rhsCount);
            return replace(offset, count, view.data(), view.size());
        }

        constexpr auto replace(size_type offset, size_type count,
            const_pointer ptr, size_type ptrCount) -> basic_fixed_string&
        {
            pointer data = m_buffer;
            // replace [offset, offset + count) with [ptr, ptr + ptrCount)
            AZSTD_CONTAINER_ASSERT(m_size >= offset, "Invalid offset");
            // Make sure count is within is no larger than the distance from the offset
            // to the end of this string
            count = AZStd::min(count, m_size - offset);

            size_type newSize = m_size + ptrCount - count;
            if (fits_in_capacity(newSize))
            {
                // The code assumes that compile time evaluation will not need to deal with overlapping input
                size_type charsAfterCountToMove = m_size - count - offset;
                if (az_builtin_is_constant_evaluated() || !((ptr >= data + offset && ptr < data + offset + count)
                    || (ptr + ptrCount > data + offset && ptr + ptrCount <= data + offset + count)))
                {
                    // Ex1. this = "ABCDEFG", offset = 1, count = 4
                    // Input string is "CDE"
                    // First the text post offset + count is moved to right after the input string will be copied
                    //  "ABCDFG"
                    //    ^^^
                    // Next the input string is copied into the buffer
                    // "ACDEFG"
                    //
                    // Ex2. this = "ABCDEFG", offset = 1, count = 2
                    // Input string is "CDE"
                    // Performing the same two steps above, the string transform as follows
                    // "ABCDEFG" -> "ABCDDEFG" -> "ACDEDEFG"
                    //                ^^^
                    if (count != ptrCount)
                    {
                        Traits::move(data + offset + ptrCount, data + offset + count, charsAfterCountToMove);
                    }
                    if (ptrCount > 0)
                    {
                        // Copy bytes up to the minimum of this string count and input string count
                        Traits::copy(data + offset, ptr, ptrCount);
                    }
                }
                else
                {
                    // Overlap checks for fixed_string only needs to check between this string
                    // [offset, offset + count) due to fixed_string never moving memory
                    //
                    // Ex. this = "ABCDEFG", offset = 1, count=4
                    // substring is "CDE"
                    // The text from offset 1 for 4 chars "BCDE": should be replaced with "CDE"
                    // making a whole for the bytes results in output = "ABCDFG"
                    // Afterwards output = "ACDEFG"
                    // The input string overlaps with this string in this case
                    // So the string is copied piecewise
                    if (ptrCount <= count)
                    {   // hole doesn't get larger, just copy in substring
                        Traits::move(data + offset, ptr, ptrCount);    // fill hole
                        Traits::copy(data + offset + ptrCount, data + offset + count, charsAfterCountToMove);   // move tail down
                    }
                    else
                    {
                        if (ptr <= data + offset)
                        {   // hole gets larger, substring begins before hole
                            Traits::copy_backward(data + offset + ptrCount, data + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(data + offset, ptr, ptrCount);   // fill hole
                        }
                        else if (data + offset + count <= ptr)
                        {   // hole gets larger, substring begins after hole
                            Traits::copy_backward(data + offset + ptrCount, data + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(data + offset, ptr + (ptrCount - count), ptrCount);   // fill hole
                        }
                        else
                        {   // hole gets larger, substring begins in hole
                            Traits::copy(data + offset, ptr, count);   // fill old hole
                            Traits::copy_backward(data + offset + ptrCount, data + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(data + offset + count, ptr + ptrCount, ptrCount - count); // fill rest of new hole
                        }
                    }
                }
            }

            m_size = static_cast<internal_size_type>(newSize);
            Traits::assign(data[newSize], Element());  // terminate

            return *this;
        }

        constexpr auto replace(size_type offset, size_type count,
            const_pointer ptr) -> basic_fixed_string&
        {
            return replace(offset, count, ptr, Traits::length(ptr));
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto replace(size_type offset, size_type count,
            const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return replace(offset, count, view.data(), view.size());
        }
        constexpr auto replace(size_type offset, size_type count,
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
                Traits::assign(data + offset, num, ch);
                m_size = static_cast<internal_size_type>(numToGrow);
                Traits::assign(data[numToGrow], Element());  // terminate
            }
            return *this;
        }

        constexpr auto replace(const_iterator first, const_iterator last,
            const basic_fixed_string& rhs) -> basic_fixed_string&
        {
            return replace(ranges::distance(cbegin(), first), ranges::distance(first, last), rhs);
        }

        constexpr auto replace(const_iterator first, const_iterator last,
            const_pointer ptr, size_type count) -> basic_fixed_string&
        {   // replace [first, last) with [ptr, ptr + count)
            return replace(ranges::distance(cbegin(), first), ranges::distance(first, last), ptr, count);
        }

        constexpr auto replace(const_iterator first, const_iterator last,
            const_pointer ptr) -> basic_fixed_string&
        {   // replace [first, last) with [ptr, <null>)
            return replace(ranges::distance(cbegin(), first), ranges::distance(first, last), ptr);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto replace(const_iterator first, const_iterator last,
            const T& convertibleToView)
            -> basic_fixed_string&
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return replace(first, last, view.data(), view.size());
        }

        constexpr auto replace(const_iterator first, const_iterator last,
            size_type count, Element ch) -> basic_fixed_string&
        {   // replace [first, last) with count * ch
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;

            pointer data = m_buffer;
            return replace(firstPtr - data, lastPtr - firstPtr, count, ch);
        }

        template<input_iterator InputIt>
            requires (!is_convertible_v<InputIt, size_t>)
        constexpr auto replace(const_iterator first, const_iterator last,
            InputIt replaceFirst, InputIt replaceLast) -> basic_fixed_string&
        {   // replace [first, last) with [replaceFirst,replaceLast)
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                return replace(first, last, AZStd::to_address(replaceFirst), ranges::distance(replaceFirst, replaceLast));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // Input Iterator pointer type doesn't match the const_pointer type
                // So the elements need to be appended one by one into the buffer

                size_type insertOffset = ranges::distance(cbegin(), first);
                size_type postInsertOffset = ranges::distance(cbegin(), last);
                size_type count = ranges::distance(replaceFirst, replaceLast);
                size_type newSize = m_size + count - ranges::distance(first, last);
                if (fits_in_capacity(newSize))
                {
                    Traits::move(m_buffer + insertOffset + count, m_buffer + postInsertOffset, m_size - postInsertOffset); // empty out hole
                    for (size_t updateIndex = insertOffset; replaceFirst != replaceLast; ++replaceFirst, ++updateIndex)
                    {
                        Traits::assign(m_buffer[updateIndex], static_cast<Element>(*replaceFirst));
                    }
                    m_size = static_cast<internal_size_type>(newSize);
                    Traits::assign(m_buffer[newSize], Element()); // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_fixed_string inputCopy;
                for (; replaceFirst != replaceLast; ++replaceFirst)
                {
                    inputCopy.push_back(static_cast<Element>(*replaceFirst));
                }

                return replace(first, last, inputCopy.c_str(), inputCopy.size());
            }
        }

        template<Internal::container_compatible_range<Element> R>
        constexpr auto replace_with_range(
            const_iterator first, const_iterator last, R&& rg)
            -> basic_fixed_string&
        {
            return replace(first, last, basic_fixed_string(from_range, AZStd::forward<R>(rg)));
        }

        constexpr auto replace(const_iterator first, const_iterator last,
            AZStd::initializer_list<Element> ilist) -> basic_fixed_string&
        {
            return replace(first, last, ilist.begin(), ilist.end());
        }

        constexpr auto at(size_type offset) -> reference
        {
            // subscript mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
            pointer data = m_buffer;
            return data[offset];
        }

        constexpr auto at(size_type offset) const -> const_reference
        {
            // subscript non mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
            const_pointer data = m_buffer;
            return data[offset];
        }

        constexpr auto operator[](size_type offset) -> reference
        {
            // subscript mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
            pointer data = m_buffer;
            return data[offset];
        }

        constexpr auto operator[](size_type offset) const -> const_reference
        {
            // subscript non mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(m_size > offset, "Invalid offset");
            const_pointer data = m_buffer;
            return data[offset];
        }

        constexpr auto front() -> reference
        {
            AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::front - string is empty!");
            pointer data = m_buffer;
            return data[0];
        }

        constexpr auto front() const -> const_reference
        {
            AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::front - string is empty!");
            const_pointer data = m_buffer;
            return data[0];
        }

        constexpr auto back() -> reference
        {
            AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::back - string is empty!");
            pointer data = m_buffer;
            return data[m_size - 1];
        }

        constexpr auto back() const -> const_reference
        {
            AZSTD_CONTAINER_ASSERT(m_size != 0, "AZStd::fixed_string::back - string is empty!");
            const_pointer data = m_buffer;
            return data[m_size - 1];
        }

        constexpr auto push_back(Element ch) -> void
        {
            insert(end(), ch);
        }
        constexpr auto pop_back() -> void
        {
            if (m_size > 0)
            {
                pointer data = m_buffer;
                --m_size;
                Traits::assign(data[m_size], Element());  // terminate
            }
        }

        constexpr auto c_str() const -> const_pointer
        {
            return m_buffer;
        }
        constexpr auto data() const -> const_pointer
        {
            return m_buffer;
        }
        constexpr auto data() -> pointer
        {
            return m_buffer;
        }
        constexpr auto length() const -> size_type
        {
            return m_size;
        }
        constexpr auto size() const -> size_type
        {
            return m_size;
        }
        constexpr auto capacity() const -> size_type
        {
            return Capacity;
        }
        constexpr auto max_size() const -> size_type
        {
            // Maximum size of the fixed string is the MaxElementCount template argument
            return MaxElementCount;
        }

        constexpr auto resize(size_type newSize) -> void
        {
            resize(newSize, Element());
        }

        constexpr auto resize_no_construct(size_type newSize) -> void
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

        template<class Operation>
        constexpr auto resize_and_overwrite(size_type n, Operation op) -> void
        {
            resize_no_construct(n);
            const auto newSize = AZStd::move(op)(data(), n);
            AZSTD_CONTAINER_ASSERT(newSize >= 0 && newSize <= n,
                "resize_and_operation operation returned a new size that is outside the bounds of [0, %zu]", n);
            resize_no_construct(newSize);
        }

        constexpr auto resize(size_type newSize, Element ch) -> void
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

        constexpr auto reserve(size_type newCapacity = 0) -> void
        {
            // Validates that a new Capacity isn't being asked for fixed_string that is larger
            // than the fixed size
            fits_in_capacity(newCapacity);
        }

        constexpr auto empty() const -> bool
        {
            return m_size == 0;
        }
        constexpr auto copy(Element* dest, size_type count, size_type offset = 0) const -> size_type
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

        constexpr auto swap(basic_fixed_string& rhs) -> void
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
        constexpr auto contains(const basic_fixed_string& other) const -> bool
        {
            return find(other) != npos;
        }
        constexpr auto contains(value_type c) const -> bool
        {
            return find(c) != npos;
        }
        constexpr auto contains(const_pointer s) const -> bool
        {
            return find(s) != npos;
        }

        constexpr auto find(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type
        {
            return find(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto find(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {
            return StringInternal::find<traits_type>(data(), size(), ptr, offset, count, npos);
        }
        constexpr auto find(const_pointer ptr, size_type offset = 0) const -> size_type
        {
            return find(ptr, offset, Traits::length(ptr));
        }
        constexpr auto find(Element ch, size_type offset = 0) const -> size_type
        {
            return find(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto find(const T& convertibleToView, size_type offset = 0) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return find(view.data(), offset, view.size());
        }

        constexpr auto rfind(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type
        {
            return rfind(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto rfind(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {   // look for [ptr, ptr + count) beginning before offset
            return StringInternal::rfind<traits_type>(data(), size(), ptr, offset, count, npos);
        }
        constexpr auto rfind(const_pointer ptr, size_type offset = npos) const -> size_type
        {
            return rfind(ptr, offset, Traits::length(ptr));
        }
        constexpr auto rfind(Element ch, size_type offset = npos) const -> size_type
        {
            return rfind(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto rfind(const T& convertibleToView, size_type offset = npos) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return rfind(view.data(), offset, view.size());
        }

        constexpr auto find_first_of(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type
        {
            return find_first_of(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto find_first_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {
            return StringInternal::find_first_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }
        constexpr auto find_first_of(const_pointer ptr, size_type offset = 0) const -> size_type
        {
            return find_first_of(ptr, offset, Traits::length(ptr));
        }
        constexpr auto find_first_of(Element ch, size_type offset = 0) const -> size_type
        {
            return find(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto find_first_of(const T& convertibleToView, size_type offset = 0) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return find_first_of(view.data(), offset, view.size());
        }

        constexpr auto find_first_not_of(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type
        {   // look for none of rhs at or after offset
            return find_first_not_of(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto find_first_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {
            return StringInternal::find_first_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }
        constexpr auto find_first_not_of(const_pointer ptr, size_type offset = 0) const -> size_type
        {
            return find_first_not_of(ptr, offset, Traits::length(ptr));
        }
        constexpr auto find_first_not_of(Element ch, size_type offset = 0) const -> size_type
        {
            return find_first_not_of(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto find_first_not_of(const T& convertibleToView, size_type offset = 0) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return find_first_not_of(view.data(), offset, view.size());
        }

        constexpr auto find_last_of(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type
        {
            return find_last_of(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto find_last_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {
            return StringInternal::find_last_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        constexpr auto find_last_of(const_pointer ptr, size_type offset = npos) const -> size_type
        {
            return find_last_of(ptr, offset, Traits::length(ptr));
        }
        constexpr auto find_last_of(Element ch, size_type offset = npos) const -> size_type
        {
            return rfind(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto find_last_of(const T& convertibleToView, size_type offset = npos) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return find_last_of(view.data(), offset, view.size());
        }

        constexpr auto find_last_not_of(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type
        {
            return find_last_not_of(rhs.m_buffer, offset, rhs.m_size);
        }
        constexpr auto find_last_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type
        {
            return StringInternal::find_last_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }
        constexpr auto find_last_not_of(const_pointer ptr, size_type offset = npos) const -> size_type
        {
            return find_last_not_of(ptr, offset, Traits::length(ptr));
        }
        constexpr auto find_last_not_of(Element ch, size_type offset = npos) const -> size_type
        {
            return find_last_not_of(&ch, offset, 1);
        }
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto find_last_not_of(const T& convertibleToView, size_type offset = npos) const
            -> size_type
        {
            basic_string_view<Element, Traits> view = convertibleToView;
            return find_last_not_of(view.data(), offset, view.size());
        }

        constexpr auto substr(size_type offset = 0, size_type count = npos) const -> basic_fixed_string
        {
            // return [offset, offset + count) as new string
            return basic_fixed_string(*this, offset, count);
        }

        constexpr auto compare(const basic_fixed_string& rhs) const -> int
        {
            const_pointer rhsData = rhs.m_buffer;
            return compare(0, m_size, rhsData, rhs.m_size);
        }

        constexpr auto compare(size_type offset, size_type count, const basic_fixed_string& rhs) const -> int
        {   // compare [offset, off + count) with rhs
            return compare(offset, count, rhs, 0, npos);
        }

        constexpr auto compare(size_type offset, size_type count,
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

        constexpr auto compare(const_pointer ptr) const -> int
        {
            return compare(0, m_size, ptr, Traits::length(ptr));
        }
        constexpr auto compare(size_type offset, size_type count, const_pointer ptr) const -> int
        {
            return compare(offset, count, ptr, Traits::length(ptr));
        }
        constexpr auto compare(size_type offset, size_type count, const_pointer ptr, size_type ptrCount) const -> int
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
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto starts_with(const T& prefix) const
            -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        constexpr auto starts_with(value_type prefix) const -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        constexpr auto starts_with(const_pointer prefix) const -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        // ends_with
        template<typename T>
            requires Internal::fixed_string_view_compatible<Element, Traits, T>
        constexpr auto ends_with(const T& suffix) const
            -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        constexpr auto ends_with(value_type suffix) const -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        constexpr auto ends_with(const_pointer suffix) const -> bool
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        constexpr auto leak_and_reset() -> void
        {
            m_size = 0;
            Traits::assign(m_buffer[0], Element());
        }

        static decltype(auto) format_arg(const char* format, va_list argList)
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

        static decltype(auto) format(const char* format, ...) AZ_FORMAT_ATTRIBUTE(1, 2)
        {
            va_list mark;
            va_start(mark, format);
            auto result = format_arg(format, mark);
            va_end(mark);
            return result;
        }

        static decltype(auto) format_arg(const wchar_t* format, va_list argList);

        static decltype(auto) format(const wchar_t* format, ...);

    protected:
        constexpr auto fits_in_capacity(size_type newSize)-> bool
        {
            AZSTD_CONTAINER_ASSERT(newSize <= Capacity, "New string size of %zu cannot fit in a fixed_string of size %zu", newSize, Capacity);
            return newSize <= Capacity;
        }

        inline static constexpr size_type Capacity = MaxElementCount; // current storage reserved for string not including null-terminator
        Element m_buffer[Capacity + 1]{};     // buffer for string
        internal_size_type m_size{}; // current length of string
    };

    // Deduction guides for basic_fixed_string
    template<class T, class... U>
    basic_fixed_string(T, U...)-> basic_fixed_string<T, 1 + sizeof...(U), AZStd::char_traits<T>>;

    // Guide for making a basic_fixed_string out of a string literal
    // i.e basic_fixed_string elem = "Test"; // basic_fixed_string<char, 4, AZStd::char_traits<char>>
    template<class T, auto N>
    basic_fixed_string(const T (&)[N])-> basic_fixed_string<T, N - 1, AZStd::char_traits<T>>; // subtract 1 to account for null-terminator

    template<size_t MaxElementCount>
    using fixed_string = basic_fixed_string<char, MaxElementCount, char_traits<char>>;
    template<size_t MaxElementCount>
    using fixed_wstring = basic_fixed_string<wchar_t, MaxElementCount, char_traits<wchar_t>>;

    template<class Element, size_t MaxElementCount, class Traits>
    constexpr void swap(basic_fixed_string<Element, MaxElementCount, Traits>& left, basic_fixed_string<Element, MaxElementCount, Traits>& right);

    // 21.4.8
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        Element rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const Element* lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(Element lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);

    // rvalue operator+ overload - Optimized to avoid copies of basic_fixed_string objects
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(basic_fixed_string<Element, MaxElementCount, Traits>&& lhs,
        Element rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(const Element* lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr basic_fixed_string<Element, MaxElementCount, Traits> operator+(Element lhs,
        basic_fixed_string<Element, MaxElementCount, Traits>&& rhs);

    // Relational Operator Overloads
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator==(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator==(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator==(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator!=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator!=(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator!=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator<=(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>=(const basic_fixed_string<Element, MaxElementCount, Traits>& lhs, const Element* rhs);
    template<class Element, size_t MaxElementCount, class Traits>
    constexpr bool operator>=(const Element* lhs, const basic_fixed_string<Element, MaxElementCount, Traits>& rhs);

    // C++20 erase helpers
    template<class Element, size_t MaxElementCount, class Traits, class U>
    constexpr auto erase(basic_fixed_string<Element, MaxElementCount, Traits>& container, const U& element)
        -> typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;

    template<class Element, size_t MaxElementCount, class Traits, class Predicate>
    constexpr auto erase_if(basic_fixed_string<Element, MaxElementCount, Traits>& container, Predicate predicate)
        -> typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type;

    template<class T>
    struct hash;

    template<class Element, size_t MaxElementCount, class Traits>
    struct hash<basic_fixed_string<Element, MaxElementCount, Traits>>;

    // Extern common fixed_string types
    extern template class AZCORE_API basic_fixed_string<char, 1024>;

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
        auto removedCount = ranges::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }

    template<class Element, size_t MaxElementCount, class Traits, class Predicate>
    inline constexpr auto erase_if(basic_fixed_string<Element, MaxElementCount, Traits>& container, Predicate predicate)
        -> typename basic_fixed_string<Element, MaxElementCount, Traits>::size_type
    {
        auto iter = AZStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = ranges::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }

    template<class Element, size_t MaxElementCount, class Traits>
    struct hash<basic_fixed_string<Element, MaxElementCount, Traits>>
    {
        using is_transparent = void;
        inline constexpr size_t operator()(const basic_string_view<Element, Traits>& value) const
        {
            return hash_string(value.begin(), value.length());
        }
    };

} // namespace AZStd

#include <AzCore/std/string/fixed_string_Platform.inl>

namespace AZStd
{
    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format(const wchar_t* format, ...)
    {
        va_list mark;
        va_start(mark, format);
        auto result = format_arg(format, mark);
        va_end(mark);
        return result;
    }
}
