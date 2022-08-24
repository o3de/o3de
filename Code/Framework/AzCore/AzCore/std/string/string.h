/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <cstring>

#include <AzCore/std/containers/compressed_pair.h>
#include <AzCore/std/containers/containers_concepts.h>
#include <AzCore/std/base.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_integral.h>

#include <AzCore/std/string/string_view.h>

namespace AZStd::StringInternal
{
    template<class Element, size_t ElementSize = sizeof(Element)>
    struct Padding
    {
        AZ::u8 m_packed[ElementSize - 1];
    };

    template<class Element>
    struct Padding<Element, 1>
    {};
}

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class StringBenchmarkFixture;
}
#endif

namespace AZStd
{
    /**
     *
     * Based on dinkumware (VC9) implementation. There are many improvements and extensions.
     */
    template<class Element, class Traits = char_traits<Element>, class Allocator = AZStd::allocator >
    class basic_string
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        using this_type = basic_string<Element, Traits, Allocator>;
    public:
        using pointer = Element*;
        using const_pointer = const Element*;

        using reference = Element&;
        using const_reference = const Element&;

        using iterator_impl = pointer;
        using const_iterator_impl = const_pointer;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        using iterator = Debug::checked_randomaccess_iterator<iterator_impl, this_type>;
        using const_iterator = Debug::checked_randomaccess_iterator<const_iterator_impl, this_type>;
#else
        using iterator = iterator_impl;
        using const_iterator = const_iterator_impl;
#endif
        using difference_type = iter_difference_t<iterator>;
        using size_type = make_unsigned_t<difference_type>;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;
        using value_type = Element;
        using traits_type = Traits;
        using allocator_type = Allocator;

        // AZSTD extension.
        /**
        * \brief Allocation node type. Common for all AZStd containers.
        * In vectors case we allocate always "sizeof(node_type)*capacity" block.
        */
        using node_type = value_type;

        inline static constexpr size_type npos = size_type(-1);

        // Constructors and Assignment operators
        // https://eel.is/c++draft/strings#string.cons
        inline constexpr basic_string(const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {
            Traits::assign(m_storage.first().GetData()[0], Element());
        }

        inline basic_string(const_pointer ptr, size_type count, const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {   // construct from [ptr, ptr + count)
            assign(ptr, count);
        }

        inline basic_string(const_pointer ptr, const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {   // construct from [ptr, <null>)
            assign(ptr);
        }

        inline basic_string(size_type count, Element ch, const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {   // construct from count * ch
            assign(count, ch);
        }

        template<class InputIt, typename = enable_if_t<input_iterator<InputIt> && !is_convertible_v<InputIt, size_t>>>
        inline basic_string(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {   // construct from [first, last)
            assign(first, last);
        }
        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        basic_string(from_range_t, R&& rg, const Allocator& alloc = Allocator())
            : m_storage{ skip_element_tag{}, alloc }
        {
            assign_range(AZStd::forward<R>(rg));
        }
        basic_string(initializer_list<Element> ilist, const Allocator& alloc = Allocator())
            : basic_string(ilist.begin(), ilist.end(), alloc)
        {
        }

        inline basic_string(const this_type& rhs)
            : m_storage{ skip_element_tag{}, rhs.m_storage.second() }
        {
            assign(rhs);
        }

        inline basic_string(this_type&& rhs)
            : m_storage{ skip_element_tag{}, rhs.m_storage.second() }
        {
            assign(AZStd::move(rhs));
        }

        inline basic_string(const this_type& rhs, size_type rhsOffset, size_type count = npos)
        {   // construct from rhs [rhsOffset, rhsOffset + count)
            assign(rhs, rhsOffset, count);
        }

        inline basic_string(const this_type& rhs, size_type rhsOffset, size_type count, const Allocator& alloc)
            : m_storage{ skip_element_tag{}, alloc }
        {   // construct from rhs [rhsOffset, rhsOffset + count) with allocator
            assign(rhs, rhsOffset, count);
        }

        basic_string(AZStd::basic_string_view<Element, Traits> view, const Allocator& alloc = Allocator())
            : basic_string(view.begin(), view.end(), alloc)
        {
        }

        basic_string(AZStd::basic_string_view<Element, Traits> view, size_type pos, size_type n, const Allocator& alloc = Allocator())
            : basic_string(view.substr(pos, n), alloc)
        {
        }

        // C++23 overload to prevent initializing a string_view via a nullptr or integer type
        constexpr basic_string(AZStd::nullptr_t) = delete;

        inline ~basic_string()
        {
            // destroy the string
            deallocate_memory(m_storage.first().GetData(), 0);
        }

        constexpr operator AZStd::basic_string_view<Element, Traits>() const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size());
        }

        inline iterator begin()             { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_storage.first().GetData())); }
        inline const_iterator begin() const { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, m_storage.first().GetData())); }
        inline const_iterator cbegin() const    { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, m_storage.first().GetData())); }
        inline iterator end()                   { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, (m_storage.first().GetData()) + m_storage.first().GetSize())); }
        inline const_iterator end() const       { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, (m_storage.first().GetData()) + m_storage.first().GetSize())); }
        inline const_iterator cend() const      { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, (m_storage.first().GetData()) + m_storage.first().GetSize())); }
        inline reverse_iterator     rbegin()            { return reverse_iterator(end()); }
        inline const_reverse_iterator   rbegin() const      { return const_reverse_iterator(end()); }
        inline const_reverse_iterator   crbegin() const     { return const_reverse_iterator(end()); }
        inline reverse_iterator     rend()              { return reverse_iterator(begin()); }
        inline const_reverse_iterator   rend() const        { return const_reverse_iterator(begin()); }
        inline const_reverse_iterator   crend() const       { return const_reverse_iterator(begin()); }

        inline this_type& operator=(const this_type& rhs)       { return assign(rhs); }
        inline this_type& operator=(this_type&& rhs) { return assign(AZStd::move(rhs)); }
        inline this_type& operator=(AZStd::basic_string_view<Element, Traits> view) { return assign(view); }
        inline this_type& operator=(const_pointer ptr)          { return assign(ptr); }
        inline this_type& operator=(Element ch)             { return assign(1, ch); }
        inline this_type& operator=(AZStd::nullptr_t) = delete;
        inline this_type& operator+=(const this_type& rhs)      { return append(rhs); }
        inline this_type& operator+=(const_pointer ptr)     { return append(ptr); }
        inline this_type& operator+=(Element ch)                { return append(1, ch); }
        inline this_type& append(const this_type& rhs)          { return append(rhs, 0, npos);  }
        this_type& append(const this_type& rhs, size_type rhsOffset, size_type count)
        {   // append rhs [rhsOffset, rhsOffset + count)
            AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset!");
            count = AZStd::min(count, rhs.size() - rhsOffset);

            size_type oldSize = size();
            size_type newSize = oldSize + count;
            if (count > 0  && grow(newSize))
            {
                pointer data = m_storage.first().GetData();
                const_pointer rhsData = rhs.data();
                // make room and append new stuff
                Traits::copy(data + oldSize, rhsData + rhsOffset, count);
                m_storage.first().SetSize(newSize);
                Traits::assign(data[newSize], Element()); // terminate
            }
            return *this;
        }

        this_type& append(const_pointer ptr, size_type count)
        {
            // append [ptr, ptr + count)
            pointer data = m_storage.first().GetData();
            if (ptr != nullptr && ptr >= data && (data + size()) > ptr)
            {
                return append(*this, ptr - data, count);    // substring
            }
            AZSTD_CONTAINER_ASSERT(max_size() - size() >= count, "result is too long!");
            size_type oldSize = size();
            size_type newSize = oldSize + count;
            if (count > 0 && grow(newSize))
            {
                // make room and append new stuff
                data = m_storage.first().GetData();
                Traits::copy(data + oldSize , ptr, count);
                m_storage.first().SetSize(newSize);
                Traits::assign(data[newSize], Element());  // terminate
            }
            return *this;
        }

        inline this_type& append(const_pointer ptr) { return append(ptr, Traits::length(ptr)); }
        this_type& append(size_type count, Element ch)
        {
            // append count * ch
            AZSTD_CONTAINER_ASSERT(max_size() - size() >= count, "result is too long");
            size_type num  = size() + count;
            if (count > 0 && grow(num))
            {
                pointer data = m_storage.first().GetData();
                // make room and append new stuff using assign
                Traits::assign(data + size(), count, ch);
                m_storage.first().SetSize(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        template<class InputIt>
        inline auto append(InputIt first, InputIt last)
            -> enable_if_t<input_iterator<InputIt> && !is_convertible_v<InputIt, size_type>, this_type&>
        {   // append [first, last)
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                return append(AZStd::to_address(first), ranges::distance(first, last));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // Input Iterator pointer type doesn't match the const_pointer type
                // So the elements need to be appended one by one into the buffer
                size_type oldSize = size();
                size_type newSize = oldSize + ranges::distance(first, last);
                if (grow(newSize))
                {
                    pointer buffer = data();
                    for (size_t updateIndex = oldSize; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_storage.first().SetSize(newSize);
                    Traits::assign(buffer[newSize], Element());  // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                return append(inputCopy.c_str(), inputCopy.size());
            }
        }

        inline this_type& append(const_pointer first, const_pointer last)
        {   // append [first, last), const pointers
            return replace(end(), end(), first, last);
        }
        template<class R>
        auto append_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>, basic_string&>
        {
            return append(basic_string(from_range, AZStd::forward<R>(rg), get_allocator()));
        }

        basic_string& append(initializer_list<Element> iList)
        {
            return append(iList.begin(), iList.end());
        }

        inline this_type& assign(const this_type& rhs)
        {
            return this != &rhs ? assign(rhs, 0, npos) : *this;
        }

        inline this_type& assign(basic_string_view<Element, Traits> view)
        {
            return assign(view.data(), view.size());
        }

        inline this_type& assign(this_type&& rhs)
        {
            if (this != &rhs)
            {
                deallocate_memory(m_storage.first().GetData(), 0);

                m_storage.first().SetCapacity(rhs.capacity());

                pointer data = m_storage.first().GetData();
                pointer rhsData = rhs.data();
                // Memmove the right hand side string data if it is using the short string optimization
                // Otherwise set the pointer to the right hand side
                if (rhs.m_storage.first().ShortStringOptimizationActive() ||
                    (get_allocator() != rhs.get_allocator() && !allocator_traits<allocator_type>::propagate_on_container_move_assignment::value))
                {
                    Traits::move(data, rhsData, rhs.size() + 1); // string + null-terminator
                }
                else
                {
                    m_storage.first().SetData(rhsData);
                }
                m_storage.first().SetSize(rhs.size());
                m_storage.second() = rhs.m_storage.second();

                rhs.leak_and_reset();
            }
            return *this;
        }

        this_type& assign(const this_type& rhs, size_type rhsOffset, size_type count)
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
                erase(0, rhsOffset);    // substring
            }
            else if (grow(num))
            {   // make room and assign new stuff
                pointer data = m_storage.first().GetData();
                const_pointer rhsData = rhs.data();
                Traits::copy(data, rhsData + rhsOffset, num);
                m_storage.first().SetSize(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        this_type& assign(const_pointer ptr, size_type count)
        {   // assign [ptr, ptr + count)
            pointer data = m_storage.first().GetData();
            if (ptr != nullptr && ptr >= data && (data + size()) > ptr)
            {
                return assign(*this, ptr - data, count);    // substring
            }
            if (grow(count))
            {
                // make room and assign new stuff
                data = m_storage.first().GetData();
                if (count > 0)
                {
                    Traits::copy(data, ptr, count);
                }
                m_storage.first().SetSize(count);
                Traits::assign(data[count], Element());  // terminate
            }
            return *this;
        }
        inline this_type& assign(const_pointer ptr) { return assign(ptr, Traits::length(ptr)); }
        this_type& assign(size_type count, Element ch)
        {
            // assign count * ch
            if (grow(count))
            {   // make room and assign new stuff
                pointer data = m_storage.first().GetData();
                Traits::assign(data, count, ch);
                m_storage.first().SetSize(count);
                Traits::assign(data[count], Element());  // terminate
            }
            return *this;
        }

        template<class InputIt>
        auto assign(InputIt first, InputIt last)
            -> enable_if_t<input_iterator<InputIt> && !is_convertible_v<InputIt, size_type>, this_type&>
        {
            if constexpr (contiguous_iterator<InputIt>
                && is_same_v<iter_value_t<InputIt>, value_type>)
            {
                return assign(AZStd::to_address(first), ranges::distance(first, last));
            }
            else if constexpr (forward_iterator<InputIt>)
            {
                // forward iterator pointer type doesn't match the const_pointer type
                // So the elements need to be assigned one by one into the buffer
                size_type newSize = ranges::distance(first, last);
                if (grow(newSize))
                {
                    pointer buffer = data();
                    for (size_t updateIndex = 0; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_storage.first().SetSize(newSize);
                    Traits::assign(buffer[newSize], Element());  // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                return assign(AZStd::move(inputCopy));
            }
        }
        template<class R>
        auto assign_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>, basic_string&>
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

        basic_string& assign(initializer_list<Element> iList)
        {
            return assign(iList.begin(), iList.end());
        }

        inline this_type& insert(size_type offset, const this_type& rhs) { return insert(offset, rhs, 0, npos); }
        this_type& insert(size_type offset, const this_type& rhs, size_type rhsOffset, size_type count)
        {
            // insert rhs [rhsOffset, rhsOffset + count) at offset
            AZSTD_CONTAINER_ASSERT(size() >= offset && rhs.size() >= rhsOffset, "Invalid offset(s)");
            size_type num = rhs.size() - rhsOffset;
            if (num < count)
            {
                count = num;    // trim _Count to size
            }
            AZSTD_CONTAINER_ASSERT(npos - size() > count, "Result is too long");
            num = size() + count;
            if (count > 0 && grow(num))
            {
                pointer data = m_storage.first().GetData();
                // make room and insert new stuff
                Traits::move(data + offset + count, data + offset, size() - offset);   // empty out hole
                if (this == &rhs)
                {
                    Traits::move(data + offset, data + (offset < rhsOffset ? rhsOffset + count : rhsOffset), count); // substring
                }
                else
                {
                    const_pointer rhsData = rhs.data();
                    Traits::copy(data + offset, rhsData + rhsOffset, count);   // fill hole
                }
                m_storage.first().SetSize(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return (*this);
        }

        this_type& insert(size_type offset, const_pointer ptr, size_type count)
        {
            // insert [ptr, ptr + count) at offset
            pointer data = m_storage.first().GetData();
            if (ptr != nullptr && ptr >= data && (data + size()) > ptr)
            {
                return insert(offset, *this, ptr - data, count); // substring
            }
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            AZSTD_CONTAINER_ASSERT(npos - size() > count, "Result is too long");
            size_type num = size() + count;
            if (count > 0 && grow(num))
            {   // make room and insert new stuff
                data = m_storage.first().GetData();
                Traits::move(data + offset + count, data + offset, size() - offset);   // empty out hole
                Traits::copy(data + offset, ptr, count);   // fill hole
                m_storage.first().SetSize(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        inline this_type& insert(size_type offset, const_pointer ptr) { return insert(offset, ptr, Traits::length(ptr)); }

        this_type& insert(size_type offset, size_type count, Element ch)
        {
            // insert count * ch at offset
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            AZSTD_CONTAINER_ASSERT(npos - size() > count, "Result is too long");
            size_type num = size() + count;
            if (count > 0 && grow(num))
            {
                pointer data = m_storage.first().GetData();
                // make room and insert new stuff
                Traits::move(data + offset + count, data + offset, size() - offset);   // empty out hole
                Traits::assign(data + offset, count, ch);
                m_storage.first().SetSize(num);
                Traits::assign(data[num], Element());  // terminate
            }
            return *this;
        }

        inline iterator insert(const_iterator insertPos) { return insert(insertPos, Element()); }

        iterator insert(const_iterator insertPos, Element ch)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer insertPosPtr = insertPos.get_iterator();
#else
            const_pointer insertPosPtr = insertPos;
#endif

            const_pointer data = m_storage.first().GetData();
            size_type offset = insertPosPtr - data;
            insert(offset, 1, ch);
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, data + offset));
        }

        iterator insert(const_iterator insertPos, size_type count, Element ch)
        {   // insert count * elem at insertPos
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer insertPosPtr = insertPos.get_iterator();
#else
            const_pointer insertPosPtr = insertPos;
#endif
            pointer data = m_storage.first().GetData();
            size_type offset = insertPosPtr - data;
            insert(offset, count, ch);
            return begin() + offset;
        }

        template<class InputIt>
        auto insert(const_iterator insertPos, InputIt first, InputIt last)
            -> enable_if_t<input_iterator<InputIt> && !is_convertible_v<InputIt, size_type>, iterator>
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
                size_type oldSize = size();
                size_type newSize = oldSize + count;
                if (grow(newSize))
                {
                    pointer buffer = m_storage.first().GetData();
                    Traits::copy_backward(buffer + insertOffset + count, buffer + insertOffset, oldSize - insertOffset); // empty out hole
                    for (size_t updateIndex = insertOffset; first != last; ++first, ++updateIndex)
                    {
                        Traits::assign(buffer[updateIndex], static_cast<Element>(*first));
                    }
                    m_storage.first().SetSize(newSize);
                    Traits::assign(buffer[newSize], Element()); // terminate
                }
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_string inputCopy;
                for (; first != last; ++first)
                {
                    inputCopy.push_back(static_cast<Element>(*first));
                }

                insert(insertOffset, inputCopy.c_str(), inputCopy.size());
            }
            return begin() + insertOffset;
        }

        template<class R>
        auto insert_range(const_iterator insertPos, R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>, iterator>
        {
            size_t offset = insertPos - begin();
            insert(offset, basic_string(from_range, AZStd::forward<R>(rg), get_allocator()));
            return begin() + offset;
        }

        iterator insert(const_iterator insertPos, initializer_list<Element> iList)
        {
            return insert(insertPos, iList.begin(), iList.end());
        }

        this_type& erase(size_type offset = 0, size_type count = npos)
        {   // erase elements [offset, offset + count)
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            if (size() - offset < count)
            {
                count = size() - offset;    // trim count
            }
            if (count > 0)
            {
                // move elements down
                pointer data = m_storage.first().GetData();
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(data + offset, data + offset + count);
#endif
                Traits::move(data + offset, data + offset + count, size() - offset - count);
                m_storage.first().SetSize(size() - count);
                Traits::assign(data[size()], Element());  // terminate
            }
            return *this;
        }

        iterator erase(const_iterator erasePos)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer erasePtr = erasePos.get_iterator();
#else
            const_pointer erasePtr = erasePos;
#endif
            // erase element at insertPos
            const_pointer data = m_storage.first().GetData();
            size_type count = erasePtr - data;
            erase(count, 1);
            data = m_storage.first().GetData();
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, data + count));
        }

        iterator erase(const_iterator first, const_iterator last)
        {   // erase substring [first, last)
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer firstPtr = first.get_iterator();
            const_pointer lastPtr = last.get_iterator();
#else
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;
#endif
            pointer data = m_storage.first().GetData();
            size_type count = firstPtr - data;
            erase(count, lastPtr - firstPtr);
            data = m_storage.first().GetData();
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, data + count));
        }

        inline void  clear() { erase(begin(), end()); }
        this_type& replace(size_type offset, size_type count, const this_type& rhs)
        {
            return replace(offset, count, rhs.c_str(), rhs.size());
        }

        this_type& replace(size_type offset, size_type count, const this_type& rhs, size_type rhsOffset, size_type rhsCount)
        {
            return replace(offset, count, rhs.c_str() + rhsOffset, AZStd::min(rhsCount, rhs.size() - rhsOffset));
        }

        this_type& replace(size_type offset, size_type count, const_pointer ptr, size_type ptrCount)
        {
            // replace [offset, offset + count) with [ptr, ptr + ptrCount)
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            // Make sure count is within is no larger than the distance from the offset
            // to the end of this string
            count = AZStd::min(count, size() - offset);

            size_type newSize = size() + ptrCount - count;
            size_type charsAfterCountToMove = size() - count - offset;
            pointer inputStringCopy{};

            if (pointer thisBuffer = m_storage.first().GetData();
                (ptr >= thisBuffer && ptr < thisBuffer + size())
                || (ptr + ptrCount > thisBuffer && ptr + ptrCount <= thisBuffer + size()))
            {
                // Overlap checks for tring needs if the input pointer is anywhere within the string
                // even if it is outside of the range of [offset, offset + count) as a growing
                // the string buffer could cause a realloc to occur
                if (!fits_in_capacity(newSize))
                {
                    // If the input string is a sub-string and it would cause
                    // this string to need to re-allocated as it doesn't fit in the capacity
                    // Then the  input string is needs to be copied into a local buffer
                    inputStringCopy = reinterpret_cast<pointer>(get_allocator().allocate(ptrCount * sizeof(value_type), alignof(value_type)));
                    Traits::copy(inputStringCopy, ptr, ptrCount);
                    // Updated the input string pointer to point to the local buffer
                    ptr = inputStringCopy;
                    // Now this string buffer can now be safely resized and the non-overlapping string logic below can be used
                }
                else
                {
                    // overlapping string in-place logic
                    // Ex. this = "ABCDEFG", offset = 1, count=4
                    // substring is "CDE"
                    // The text from offset 1 for 4 chars "BCDE": should be replaced with "CDE"
                    // making a whole for the bytes results in output = "ABCDFG"
                    // Afterwards output = "ACDEFG"
                    // The input string overlaps with this string in this case
                    // So the string is copied piecewise
                    if (ptrCount <= count)
                    {   // hole doesn't get larger, just copy in substring
                        Traits::move(thisBuffer + offset, ptr, ptrCount);    // fill hole
                        Traits::copy(thisBuffer + offset + ptrCount, thisBuffer + offset + count, charsAfterCountToMove);   // move tail down
                    }
                    else
                    {
                        if (ptr <= thisBuffer + offset)
                        {   // hole gets larger, substring begins before hole
                            Traits::copy_backward(thisBuffer + offset + ptrCount, thisBuffer + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(thisBuffer + offset, ptr, ptrCount);   // fill hole
                        }
                        else if (thisBuffer + offset + count <= ptr)
                        {   // hole gets larger, substring begins after hole
                            Traits::copy_backward(thisBuffer + offset + ptrCount, thisBuffer + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(thisBuffer + offset, ptr + (ptrCount - count), ptrCount);   // fill hole
                        }
                        else
                        {   // hole gets larger, substring begins in hole
                            Traits::copy(thisBuffer + offset, ptr, count);   // fill old hole
                            Traits::copy_backward(thisBuffer + offset + ptrCount, thisBuffer + offset + count, charsAfterCountToMove);   // move tail down
                            Traits::copy(thisBuffer + offset + count, ptr + ptrCount, ptrCount - count); // fill rest of new hole
                        }
                    }
                    m_storage.first().SetSize(newSize);
                    Traits::assign(thisBuffer[newSize], Element());  // terminate
                    return *this;
                }
            }

            // input string doesn't overlap, so this string can be re-allocated safely
            if (grow(newSize))
            {
                // Need to regrab the memory address for the storage buffer
                // in case the grow re-allocated memory
                pointer thisBuffer = m_storage.first().GetData();
                if (count != ptrCount)
                {
                    Traits::move(thisBuffer + offset + ptrCount, thisBuffer + offset + count, charsAfterCountToMove);
                }
                if (ptrCount > 0)
                {
                    // Copy bytes up to the minimum of this string count and input string count
                    Traits::copy(thisBuffer + offset, ptr, ptrCount);
                }
                // input string doesn't overlap, so this string can be re-allocated safely
                m_storage.first().SetSize(newSize);
                Traits::assign(thisBuffer[newSize], Element());  // terminate
            }

            // If a local string was allocated, then de-allocate its memory
            if (inputStringCopy != nullptr)
            {
                get_allocator().deallocate(inputStringCopy, 0, alignof(value_type));
            }

            return *this;
        }

        inline this_type& replace(size_type offset, size_type count, const_pointer ptr) { return replace(offset, count, ptr, Traits::length(ptr)); }
        this_type& replace(size_type offset, size_type count, size_type num, Element ch)
        {   // replace [offset, offset + count) with num * ch
            AZSTD_CONTAINER_ASSERT(size() > offset, "Invalid offset");
            if (size() - offset < count)
            {
                count = size() - offset;    // trim count to size
            }
            AZSTD_CONTAINER_ASSERT(npos - num > size() - count, "Result is too long");
            size_type nm = size() - count - offset;

            pointer data = m_storage.first().GetData();
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(data + offset, data + offset + count);
#endif
            if (num < count)
            {
                Traits::move(data + offset + num, data + offset + count, nm); // smaller hole, move tail up
            }
            size_type numToGrow = size() + num - count;
            if ((0 < num || 0 < count) && grow(numToGrow))
            {   // make room and rearrange
                data = m_storage.first().GetData();
                if (count < num)
                {
                    Traits::move(data + offset + num, data + offset + count, nm); // move tail down
                }
                Traits::assign(data + offset, num, ch);
                m_storage.first().SetSize(numToGrow);
                Traits::assign(data[numToGrow], Element());  // terminate
            }
            return *this;
        }

        this_type& replace(const_iterator first, const_iterator last, const this_type& rhs)
        {
            // replace [first, last) with right
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer firstPtr = first.get_iterator();
            const_pointer lastPtr = last.get_iterator();
#else
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;
#endif
            pointer data = m_storage.first().GetData();
            return replace(firstPtr - data, lastPtr - firstPtr, rhs);
        }

        this_type& replace(const_iterator first, const_iterator last, const_pointer ptr, size_type count)
        {   // replace [first, last) with [ptr, ptr + count)
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer firstPtr = first.get_iterator();
            const_pointer lastPtr = last.get_iterator();
#else
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;
#endif
            pointer data = m_storage.first().GetData();
            return replace(firstPtr - data, lastPtr - firstPtr, ptr, count);
        }

        this_type& replace(const_iterator first, const_iterator last, const_pointer ptr)
        {   // replace [first, last) with [ptr, <null>)
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer firstPtr = first.get_iterator();
            const_pointer lastPtr = last.get_iterator();
#else
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;
#endif
            pointer data = m_storage.first().GetData();
            return replace(firstPtr - data, lastPtr - firstPtr, ptr);
        }

        this_type& replace(const_iterator first, const_iterator last, size_type count, Element ch)
        {   // replace [first, last) with count * ch
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer firstPtr = first.get_iterator();
            const_pointer lastPtr = last.get_iterator();
#else
            const_pointer firstPtr = first;
            const_pointer lastPtr = last;
#endif
            pointer data = m_storage.first().GetData();
            return replace(firstPtr - data, lastPtr - firstPtr, count, ch);
        }

        template<class InputIt>
        inline auto replace(const_iterator first, const_iterator last, InputIt replaceFirst, InputIt replaceLast)
            -> enable_if_t<input_iterator<InputIt> && !is_convertible_v<InputIt, size_type>, this_type&>
        {
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
                size_type oldSize = size();
                size_type newSize = oldSize + count - ranges::distance(first, last);
                if (grow(newSize))
                {
                    pointer buffer = data();
                    Traits::move(first + count, last, oldSize - postInsertOffset); // empty out hole
                    for (size_t updateIndex = insertOffset; replaceFirst != replaceLast; ++replaceFirst, ++updateIndex)
                    {
                        Traits::assign(buffer[updateIndex], static_cast<Element>(*replaceFirst));
                    }
                    m_storage.first().SetSize(newSize);
                    Traits::assign(buffer[newSize], Element()); // terminate
                }
                return *this;
            }
            else
            {
                // input iterator that aren't forward iterators can only be used in a single pass
                // algorithm. Therefore ranges::distance can't be used
                // So the input is copied into a local string and then delegated
                // to use the (const_pointer, size_type) overload
                basic_string inputCopy;
                for (; replaceFirst != replaceLast; ++replaceFirst)
                {
                    inputCopy.push_back(static_cast<Element>(*replaceFirst));
                }

                return replace(first, last, inputCopy.c_str(), inputCopy.size());
            }
        }

        template<class R>
        auto replace_with_range(const_iterator first, const_iterator last, R&& rg)
            -> enable_if_t<Internal::container_compatible_range<R, value_type>, basic_string&>
        {
            return replace(first, last, basic_string(from_range, AZStd::forward<R>(rg), get_allocator()));
        }

        basic_string& replace(const_iterator first, const_iterator last, initializer_list<Element> iList)
        {
            return replace(first, last, iList.begin(), iList.end());
        }

        inline reference at(size_type offset)
        {
            // subscript mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(size() > offset, "Invalid offset");
            pointer data = m_storage.first().GetData();
            return data[offset];
        }

        inline const_reference at(size_type offset) const
        {
            // subscript nonmutable sequence with checking
            AZSTD_CONTAINER_ASSERT(size() > offset, "Invalid offset");
            const_pointer data = m_storage.first().GetData();
            return data[offset];
        }

        inline reference operator[](size_type offset)
        {
            // subscript mutable sequence with checking
            AZSTD_CONTAINER_ASSERT(size() > offset, "Invalid offset");
            pointer data = m_storage.first().GetData();
            return data[offset];
        }

        inline const_reference operator[](size_type offset) const
        {
            // subscript nonmutable sequence with checking
            AZSTD_CONTAINER_ASSERT(size() > offset, "Invalid offset");
            const_pointer data = m_storage.first().GetData();
            return data[offset];
        }

        inline reference front()
        {
            AZSTD_CONTAINER_ASSERT(size() != 0, "AZStd::string::front - string is empty!");
            pointer data = m_storage.first().GetData();
            return data[0];
        }

        inline const_reference front() const
        {
            AZSTD_CONTAINER_ASSERT(size() != 0, "AZStd::string::front - string is empty!");
            const_pointer data = m_storage.first().GetData();
            return data[0];
        }

        inline reference back()
        {
            AZSTD_CONTAINER_ASSERT(size() != 0, "AZStd::string::back - string is empty!");
            pointer data = m_storage.first().GetData();
            return data[size() - 1];
        }

        inline const_reference back() const
        {
            AZSTD_CONTAINER_ASSERT(size() != 0, "AZStd::string::back - string is empty!");
            const_pointer data = m_storage.first().GetData();
            return data[size() - 1];
        }

        inline void push_back(Element ch)
        {
            const_pointer end = data();
            end += size();
            insert(end, ch);
        }

        inline const_pointer    c_str() const       {   return (data()); }
        inline size_type        length() const      {   return m_storage.first().GetSize(); }
        inline size_type        size() const        {   return m_storage.first().GetSize(); }
        inline size_type        capacity() const    {   return m_storage.first().GetCapacity(); }
        inline size_type        max_size() const
        {
            // return maximum possible length of sequence
            return AZStd::allocator_traits<allocator_type>::max_size(m_storage.second()) / sizeof(value_type);
        }

        inline void resize(size_type newSize)
        {
            resize(newSize, Element());
        }

        inline void resize_no_construct(size_type newSize)
        {
            if (newSize <= size())
            {
                erase(newSize);
            }
            else
            {
                reserve(newSize);
                pointer data = m_storage.first().GetData();
                m_storage.first().SetSize(newSize);
                Traits::assign(data[newSize], Element());  // terminate
            }
        }

        template<class Operation>
        void resize_and_overwrite(size_type n, Operation op)
        {
            resize_no_construct(n);
            const auto newSize = AZStd::move(op)(data(), n);
            AZSTD_CONTAINER_ASSERT(newSize >= 0 && newSize <= n,
                "resize_and_operation operation returned a new size that is outside the bounds of [0, %zu]", n);
            resize_no_construct(newSize);
        }

        inline void resize(size_type newSize, Element ch)
        {   // determine new length, padding with ch elements as needed
            if (newSize <= size())
            {
                erase(newSize);
            }
            else
            {
                append(newSize - size(), ch);
            }
        }

        void reserve(size_type newCapacity = 0)
        {   // determine new minimum length of allocated storage
            if (size() <= newCapacity && capacity() != newCapacity)
            {
                // change reservation
                size_type curSize = size();
                if (grow(newCapacity))
                {
                    m_storage.first().SetSize(curSize);
                    pointer data = m_storage.first().GetData();
                    Traits::assign(data[curSize], Element());  // terminate
                }
            }
        }

        inline bool empty() const { return size() == 0; }
        size_type copy(Element* dest, size_type count, size_type offset = 0) const
        {
            // copy [offset, offset + count) to [dest, dest + count)
            // assume there is enough space in _Ptr
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            if (size() - offset < count)
            {
                count = size() - offset;
            }
            const_pointer data = m_storage.first().GetData();
            Traits::copy(dest, data + offset, count);
            return count;
        }

        void swap(this_type& rhs)
        {   // exchange contents with _Right
            if (this == &rhs)
            {
                return;
            }

            if (m_storage.second() == rhs.m_storage.second())
            {
                // same allocator, swap storage
                m_storage.first().swap(rhs.m_storage.first());
            }
            else if (allocator_traits<allocator_type>::propagate_on_container_swap::value)
            {
                // The allocator propagates on swap, so the allocators can be swapped
                m_storage.first().swap(rhs.m_storage.first());
                using AZStd::swap;
                swap(m_storage.second(), rhs.m_storage.second());
            }
            else
            {
                // different allocator, do multiple assigns
                this_type tmp = AZStd::move(*this);
                *this = AZStd::move(rhs);
                rhs = AZStd::move(tmp);
            }
        }

        // C++23 contains
        bool contains(const basic_string& other) const
        {
            return find(other) != npos;
        }

        bool contains(value_type c) const
        {
            return find(c) != npos;
        }

        bool contains(const_pointer s) const
        {
            return find(s) != npos;
        }

        inline size_type find(const this_type& rhs, size_type offset = 0) const
        {
            const_pointer rhsData = rhs.data();
            return find(rhsData, offset, rhs.size());
        }
        size_type find(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::find<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type find(const_pointer ptr, size_type offset = 0) const    { return find(ptr, offset, Traits::length(ptr)); }
        inline size_type find(Element ch, size_type offset = 0) const           { return find((const_pointer) & ch, offset, 1); }
        inline size_type rfind(const this_type& rhs, size_type offset = npos) const
        {
            const_pointer rhsData = rhs.data();
            return rfind(rhsData, offset, rhs.size());
        }

        size_type rfind(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::rfind<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type rfind(const_pointer ptr, size_type offset = npos) const    { return rfind(ptr, offset, Traits::length(ptr)); }
        inline size_type rfind(Element ch, size_type offset = npos) const           { return rfind((const_pointer) & ch, offset, 1); }
        inline size_type find_first_of(const this_type& rhs, size_type offset = 0) const
        {
            const_pointer rhsData = rhs.data();
            return find_first_of(rhsData, offset, rhs.size());
        }

        size_type find_first_of(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::find_first_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type find_first_of(const_pointer ptr, size_type offset = 0) const   {   return find_first_of(ptr, offset, Traits::length(ptr)); }
        inline size_type find_first_of(Element ch, size_type offset = 0) const          {   return find((const_pointer) & ch, offset, 1); }
        inline size_type find_last_of(const this_type& rhs, size_type offset = npos) const
        {
            const_pointer rhsData = rhs.data();
            return find_last_of(rhsData, offset, rhs.size());
        }

        size_type find_last_of(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::find_last_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type find_last_of(const_pointer ptr, size_type offset = npos) const      { return find_last_of(ptr, offset, Traits::length(ptr)); }
        inline size_type find_last_of(Element ch, size_type offset = npos) const            { return rfind((const_pointer) & ch, offset, 1);  }
        inline size_type find_first_not_of(const this_type& rhs, size_type offset = 0) const
        {   // look for none of rhs at or after offset
            const_pointer rhsData = rhs.data();
            return find_first_not_of(rhsData, offset, rhs.size());
        }

        size_type find_first_not_of(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::find_first_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type find_first_not_of(const_pointer ptr, size_type offset = 0) const       {   return find_first_not_of(ptr, offset, Traits::length(ptr)); }
        inline size_type find_first_not_of(Element ch, size_type offset = 0) const              {   return find_first_not_of((const_pointer) & ch, offset, 1); }
        inline size_type find_last_not_of(const this_type& rhs, size_type offset = npos) const
        {   // look for none of rhs before offset
            const_pointer rhsData = rhs.data();
            return find_last_not_of(rhsData, offset, rhs.size());
        }
        size_type find_last_not_of(const_pointer ptr, size_type offset, size_type count) const
        {
            return StringInternal::find_last_not_of<traits_type>(data(), size(), ptr, offset, count, npos);
        }

        inline size_type find_last_not_of(const_pointer ptr, size_type offset = npos) const {   return find_last_not_of(ptr, offset, Traits::length(ptr)); }
        inline size_type find_last_not_of(Element ch, size_type offset = npos) const            {   return find_last_not_of((const_pointer) & ch, offset, 1); }

        inline this_type substr(size_type offset = 0, size_type count = npos) const
        {
            // return [offset, offset + count) as new string
            return this_type(*this, offset, count, get_allocator());
        }

        inline int compare(const this_type& rhs) const
        {
            const_pointer rhsData = rhs.data();
            return compare(0, size(), rhsData, rhs.size());
        }

        inline int compare(size_type offset, size_type count, const this_type& rhs) const
        {   // compare [offset, off + count) with rhs
            return compare(offset, count, rhs, 0, npos);
        }

        int compare(size_type offset, size_type count, const this_type& rhs, size_type rhsOffset, size_type rhsCount) const
        {
            // compare [offset, offset + count) with rhs [rhsOffset, rhsOffset + rhsCount)
            AZSTD_CONTAINER_ASSERT(rhs.size() >= rhsOffset, "Invalid offset");
            if (rhs.size() - rhsOffset < rhsCount)
            {
                rhsCount = rhs.size() - rhsOffset;  // trim rhsCount to size
            }
            const_pointer rhsData = rhs.data();
            return compare(offset, count, rhsData + rhsOffset, rhsCount);
        }

        inline int compare(const_pointer ptr) const                                     { return compare(0, size(), ptr, Traits::length(ptr)); }
        inline int compare(size_type offset, size_type count, const_pointer ptr) const      { return compare(offset, count, ptr, Traits::length(ptr)); }
        int compare(size_type offset, size_type count, const_pointer ptr, size_type ptrCount) const
        {
            // compare [offset, offset + _N0) with [_Ptr, _Ptr + _Count)
            AZSTD_CONTAINER_ASSERT(size() >= offset, "Invalid offset");
            if (size() - offset < count)
            {
                count = size() - offset;    // trim count to size
            }
            const_pointer data = m_storage.first().GetData();
            size_type ans = Traits::compare(data + offset, ptr, count < ptrCount ? count : ptrCount);
            return (ans != 0 ? (int)ans : count < ptrCount ? -1 : count == ptrCount ? 0 : +1);
        }

        // starts_with
        bool starts_with(AZStd::basic_string_view<Element, Traits> prefix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        bool starts_with(value_type prefix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        bool starts_with(const_pointer prefix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).starts_with(prefix);
        }

        // ends_with
        bool ends_with(AZStd::basic_string_view<Element, Traits> suffix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        bool ends_with(value_type suffix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        bool ends_with(const_pointer suffix) const
        {
            return AZStd::basic_string_view<Element, Traits>(data(), size()).ends_with(suffix);
        }

        inline void pop_back()
        {
            if (!empty())
            {
                pointer data = m_storage.first().GetData();
                m_storage.first().SetSize(m_storage.first().GetSize() - 1);
                Traits::assign(data[size()], Element());  // terminate
            }
        }

        /**
        * \anchor StringExtensions
        * \name Extensions
        * @{
        */
        /// TR1 Extension. Return pointer to the vector data. The vector data is guaranteed to be stored as an array.
        inline pointer          data()          { return m_storage.first().GetData(); }
        inline const_pointer    data() const    { return m_storage.first().GetData(); }

        ///

        /// The only difference from the standard is that we return the allocator instance, not a copy.
        inline allocator_type&          get_allocator()         { return m_storage.second(); }
        inline const allocator_type&    get_allocator() const   { return m_storage.second(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        void                            set_allocator(const allocator_type& allocator)
        {
            if (m_storage.second() != allocator)
            {
                if (!empty() && !m_storage.first().ShortStringOptimizationActive())
                {
                    allocator_type newAllocator = allocator;
                    pointer data = m_storage.first().GetData();

                    pointer newData = reinterpret_cast<pointer>(newAllocator.allocate(sizeof(node_type) * (capacity() + 1), alignof(node_type)));

                    Traits::copy(newData, data, size() + 1);  // copy elements and terminator

                    // Free memory (if needed).
                    deallocate_memory(data, 0);
                    m_storage.second() = newAllocator;
                }
                else
                {
                    m_storage.second() = allocator;
                }
            }
        }

        // Validate container status.
        bool    validate() const
        {
            return true;
        }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        int     validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            pointer iterPtr = iter.m_iter;
#else
            pointer iterPtr = iter;
#endif
            const_pointer data = m_storage.first().GetData();
            if (iterPtr < data || iterPtr > (data + size()))
            {
                return isf_none;
            }
            else if (iterPtr == (data + size()))
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        int     validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            const_pointer iterPtr = iter.m_iter;
#else
            const_pointer iterPtr = iter;
#endif
            const_pointer data = m_storage.first().GetData();
            if (iterPtr < data || iterPtr > (data + size()))
            {
                return isf_none;
            }
            else if (iterPtr == (data + size()))
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        void leak_and_reset()
        {
            m_storage.first() = {};
        }

        /**
        * Set the capacity, if necessary it will erase elements at the end of the container to match the new capacity.
        */
        void set_capacity(size_type numElements)
        {
            // sets the new capacity of the vector, can be smaller than size()
            if (capacity() != numElements)
            {
                if (numElements < ShortStringData::Capacity)
                {
                    if (!m_storage.first().ShortStringOptimizationActive())
                    {
                        // copy any leftovers to small buffer and deallocate
                        pointer ptr = m_storage.first().GetData();
                        numElements = numElements < size() ? numElements : size();
                        m_storage.first().SetCapacity(ShortStringData::Capacity);
                        if (0 < numElements)
                        {
                            Traits::copy(m_storage.first().GetData(), ptr, numElements);
                        }
                        // deallocate_memory functione examines the current
                        // m_storage short string optimization state was changed to true
                        // by the SetCapacity call above. Therefore m_storage.second().deallocate
                        // is used directly
                        m_storage.second().deallocate(ptr, 0, alignof(node_type));
                    }

                    m_storage.first().SetSize(numElements);
                    Traits::assign(m_storage.first().GetData()[numElements], Element());  // terminate
                }
                else
                {
                    pointer newData = reinterpret_cast<pointer>(m_storage.second().allocate(sizeof(node_type) * (numElements + 1), alignof(node_type)));
                    AZSTD_CONTAINER_ASSERT(newData != nullptr, "AZStd::string allocation failed!");

                    size_type newSize = numElements < m_storage.first().GetSize() ? numElements : m_storage.first().GetSize();
                    pointer data = m_storage.first().GetData();
                    if (newSize > 0)
                    {
                        Traits::copy(newData, data, newSize);  // copy existing elements
                    }
                    deallocate_memory(data, 0);

                    Traits::assign(newData[newSize], Element());  // terminate
                    m_storage.first().SetCapacity(numElements);
                    m_storage.first().SetData(newData);
                    m_storage.first().SetSize(newSize);
                }
            }
        }

        static basic_string<char, char_traits<char>, Allocator> format_arg(const char* formatStr, va_list argList)
        {
            basic_string<char, char_traits<char>, Allocator> result;
            // On Windows, AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS is set, so azvsnprintf calls _vsnprintf_s.
            // _vsnprintf_s, unlike vsnprintf, will fail and return -1 when the size argument is 0.
            // azvscprintf uses the proper function(_vscprintf or vsnprintf) on the given platform
            va_list argListCopy; //<- Depending on vprintf implementation va_list may be consumed, so send a copy
            va_copy(argListCopy, argList);
            const int len = azvscprintf(formatStr, argListCopy);
            va_end(argListCopy);
            if (len > 0)
            {
                result.resize(len);
                va_copy(argListCopy, argList);
                const int bytesPrinted = azvsnprintf(result.data(), result.size() + 1, formatStr, argListCopy);
                va_end(argListCopy);
                AZ_UNUSED(bytesPrinted);
                AZ_Assert(bytesPrinted >= 0, "azvsnprintf error! Format string: \"%s\"", formatStr);
                AZ_Assert(static_cast<size_t>(bytesPrinted) == result.size(), "azvsnprintf failed to print all bytes! Format string: \"%s\", bytesPrinted=%i/%i",
                    formatStr, bytesPrinted, len);
            }
            return result;
        }

        static basic_string<wchar_t, char_traits<wchar_t>, Allocator> format_arg(const wchar_t* formatStr, va_list argList)
        {
            basic_string<wchar_t, char_traits<wchar_t>, Allocator> result;
#if defined(AZ_COMPILER_MSVC)
            // On Windows, AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS is set, so azvsnwprintf calls _vsnwprintf_s.
            // _vsnwprintf_s, unlike vsnwprintf, will fail and return -1 when the size argument is 0.
            // azvscwprintf uses the proper function(_vscwprintf or vsnwprintf) on the given platform
            va_list argListCopy; //<- Depending on vprintf implementation va_list may be consumed, so send a copy
            va_copy(argListCopy, argList);
            const int len = azvscwprintf(formatStr, argListCopy);
            va_end(argListCopy);
            if (len > 0)
            {
                result.resize(len);
                va_copy(argListCopy, argList);
                const int bytesPrinted = azvsnwprintf(result.data(), result.size() + 1, formatStr, argListCopy);
                va_end(argListCopy);
                AZ_UNUSED(bytesPrinted);
                AZ_Assert(bytesPrinted >= 0, "azvsnwprintf error! Format string: \"%ws\"", formatStr);
                AZ_Assert(static_cast<size_t>(bytesPrinted) == result.size(), "azvsnwprintf failed to print all bytes! Format string: \"%ws\", bytesPrinted=%i/%i",
                    formatStr, bytesPrinted, len);
            }
#else
            constexpr int maxBufferLength = 2048;
            wchar_t buffer[maxBufferLength];
            [[maybe_unused]] const int len = azvsnwprintf(buffer, maxBufferLength, formatStr, argList);
            AZ_Assert(len != -1, "azvsnwprintf failed increase the buffer size!");
            result += buffer;
#endif
            return result;
        }

        struct _Format_Internal
        {
            struct ValidFormatArg
            {
                ValidFormatArg(const bool) {}
                ValidFormatArg(const char) {}
                ValidFormatArg(const unsigned char) {}
                ValidFormatArg(const signed char) {}
                ValidFormatArg(const wchar_t) {}
                ValidFormatArg(const char16_t) {}
                ValidFormatArg(const char32_t) {}
                ValidFormatArg(const unsigned short) {}
                ValidFormatArg(const short) {}
                ValidFormatArg(const unsigned int) {}
                ValidFormatArg(const int) {}
                ValidFormatArg(const unsigned long) {}
                ValidFormatArg(const long) {}
                ValidFormatArg(const unsigned long long) {}
                ValidFormatArg(const long long) {}
                ValidFormatArg(const float) {}
                ValidFormatArg(const double) {}
                ValidFormatArg(const char*) {}
                ValidFormatArg(const wchar_t*) {}
                ValidFormatArg(const void*) {}
            };

            static basic_string<char, char_traits<char>, Allocator> raw_format(const char* formatStr, ...)
            {
                va_list args;
                va_start(args, formatStr);
                basic_string<char, char_traits<char>, Allocator> result = this_type::format_arg(formatStr, args);
                va_end(args);
                return result;
            }

            static basic_string<wchar_t, char_traits<wchar_t>, Allocator> raw_format(const wchar_t* formatStr, ...)
            {
                va_list args;
                va_start(args, formatStr);
                basic_string<wchar_t, char_traits<wchar_t>, Allocator> result = this_type::format_arg(formatStr, args);
                va_end(args);
                return result;
            }
        };

        // Clang/GCC supports compile-time check for printf-like signatures
        // On MSVC, *only* if /analyze flag is enabled(defines _PREFAST_) we can also do a compile-time check
        // For not affecting final release binary size, we don't use the templated version on Release configuration either
#if AZ_COMPILER_CLANG || AZ_COMPILER_GCC || defined(_PREFAST_) || defined(_RELEASE)
#    if AZ_COMPILER_CLANG || AZ_COMPILER_GCC
#        define FORMAT_FUNC      __attribute__((format(printf, 1, 2)))
#        define FORMAT_FUNC_ARG
#    elif AZ_COMPILER_MSVC
#        define FORMAT_FUNC
#        define FORMAT_FUNC_ARG  _Printf_format_string_
#    else
#        define FORMAT_FUNC
#        define FORMAT_FUNC_ARG
#    endif

        FORMAT_FUNC static basic_string<char, char_traits<char>, Allocator> format(FORMAT_FUNC_ARG const char* formatStr, ...)
        {
            va_list args;
            va_start(args, formatStr);
            basic_string<char, char_traits<char>, Allocator> result = format_arg(formatStr, args);
            va_end(args);
            return result;
        }

#    undef FORMAT_FUNC
#    undef FORMAT_FUNC_ARG

#else // !AZ_COMPILER_CLANG && !defined(_PREFAST_) && !defined(_RELEASE)

        static basic_string<char, char_traits<char>, Allocator> format(const char* formatStr)
        {
            return { formatStr };
        }

        template<typename... Args>
        static basic_string<char, char_traits<char>, Allocator> format(const char* formatStr, Args... args)
        {
            using ValidFormatArg = typename _Format_Internal::ValidFormatArg;

            auto IsValidFormatArg = [](const auto& value) constexpr
            {
                constexpr bool isValid = AZSTD_IS_CONVERTIBLE(decltype(value), ValidFormatArg);
                static_assert(isValid, "Invalid string::format argument");
                return isValid;
            };
            constexpr bool allValid = (IsValidFormatArg(args) && ...);
            static_assert(allValid, "Invalid string::format arguments, must be: numeric(floating point, integral, pointer) or C String(char/w_char)");

            return _Format_Internal::raw_format(formatStr, args...);
        }
#endif // AZ_COMPILER_CLANG || defined(_PREFAST_) || defined(_RELEASE)

        static inline basic_string<wchar_t, char_traits<wchar_t>, Allocator> format(const wchar_t* formatStr)
        {
            return { formatStr };
        }

        template<typename... Args>
        static basic_string<wchar_t, char_traits<wchar_t>, Allocator> format(const wchar_t* formatStr, Args... args)
        {
            using ValidFormatArg = typename _Format_Internal::ValidFormatArg;

            auto IsValidFormatArg = [](const auto& value) constexpr
            {
                constexpr bool isValid = AZSTD_IS_CONVERTIBLE(decltype(value), ValidFormatArg);
                static_assert(isValid, "Invalid wstring::format argument");
                return isValid;
            };
            constexpr bool allValid = (IsValidFormatArg(args) && ...);
            static_assert(allValid, "Invalid wstring::format arguments, must be: numeric(floating point, integral, pointer) or C String(char/w_char)");

            return _Format_Internal::raw_format(formatStr, args...);
        }

        template<class UAllocator>
        inline basic_string(const basic_string<Element, Traits, UAllocator>& rhs)
        {
            assign(rhs.c_str());
        }
        template<class  UAllocator>
        inline this_type& operator=(const basic_string<Element, Traits, UAllocator>& rhs) { return assign(rhs.c_str()); }
        template<class  UAllocator>
        inline this_type& append(const basic_string<Element, Traits, UAllocator>& rhs) { return append(rhs.c_str()); }
        template<class  UAllocator>
        inline this_type& insert(size_type offset, const basic_string<Element, Traits, UAllocator>& rhs) { return insert(offset, rhs.c_str()); }
        template<class  UAllocator>
        inline this_type& replace(size_type offset, size_type count, const basic_string<Element, Traits, UAllocator>& rhs) { return replace(offset, count, rhs.c_str()); }
        template<class  UAllocator>
        inline int compare(const basic_string<Element, Traits, UAllocator>& rhs) { return compare(rhs.c_str()); }
        // @}

    protected:
        enum
        {   // roundup mask for allocated buffers, [0, 15]
            _ALLOC_MASK = sizeof(Element) <= 1 ? 15
                : sizeof(Element) <= 2 ? 7
                : sizeof(Element) <= 4 ? 3
                : sizeof(Element) <= 8 ? 1 : 0
        };

        void copy(size_type newSize, size_type oldLength)
        {
            size_type newCapacity = newSize | _ALLOC_MASK;
            size_type currentCapacity = capacity();
            if (newCapacity / 3 < currentCapacity / 2)
            {
                newCapacity = currentCapacity + currentCapacity / 2;  // grow exponentially if possible
            }
            if (newCapacity >= ShortStringData::Capacity)
            {
                pointer newData = reinterpret_cast<pointer>(m_storage.second().allocate(sizeof(node_type) * (newCapacity + 1), alignof(node_type)));
                AZSTD_CONTAINER_ASSERT(newData != nullptr, "AZStd::string allocation failed!");
                if (newData)
                {
                    pointer data = m_storage.first().GetData();
                    if (0 < oldLength)
                    {
                        Traits::copy(newData, data, oldLength);    // copy existing elements
                    }
                    deallocate_memory(data, 0);

                    Traits::assign(newData[oldLength], Element());  // terminate
                    m_storage.first().SetCapacity(newCapacity);
                    m_storage.first().SetSize(oldLength);
                    m_storage.first().SetData(newData);
                }
            }
        }

        bool grow(size_type newSize)
        {
            // ensure buffer is big enough, trim to size if _Trim is true
            if (capacity() < newSize)
            {
                copy(newSize, size());  // reallocate to grow
            }
            else if (newSize == 0)
            {
                pointer data = m_storage.first().GetData();
                m_storage.first().SetSize(0);
                Traits::assign(data[0], Element());  // terminate
            }
            return (0 < newSize);   // return true only if more work to do
        }

        bool fits_in_capacity(size_type newSize)
        {
            return newSize <= capacity();
        }

        inline void deallocate_memory(pointer data, size_type expandedSize)
        {
            if (!m_storage.first().ShortStringOptimizationActive())
            {
                size_type byteSize = (expandedSize == 0) ? (sizeof(node_type) * (m_storage.first().GetCapacity() + 1)) : expandedSize;
                m_storage.second().deallocate(data, byteSize, alignof(node_type));
            }
        }

        //! Assuming 64-bit for pointer and size_t size
        //! The offset and sizes of each structure are marked below

        //! dynamically allocated data
        struct AllocatedStringData
        {
            AllocatedStringData()
            {
                m_capacity = 0;
                m_ssoActive = false;
            }
            // bit offset: 0, bits: 64
            pointer m_data{};

            // bit offset: 64, bit: 64
            size_type m_size{};

            // Use all but the top bit of a size_t for the string capacity
            // This allows the short string optimization to be used
            // with no additional space at the cost of cutting the max_size in half
            // to 2^63-1
            // offset: 128, bits: 63
            size_type m_capacity : AZStd::numeric_limits<size_type>::digits - 1;

            // bit offset: 191, bits: 1
            size_type m_ssoActive : 1;

            // Total size 192 bits(24 bytes)
        };

        static_assert(sizeof(AllocatedStringData) <= 24, "The AllocatedStringData structure"
            " should be an 8-byte pointer, 8 byte size, 63-bit capacity and 1-bit SSO flag for"
            " a total of 24 bytes");

        //! small buffer used for small string optimization
        struct ShortStringData
        {
            //! The size can be stored within 7 bits since the buffer will be no larger
            //! than 23 bytes(22 characters + 1 null-terminating character)
            inline static constexpr size_type BufferMaxSize = sizeof(AllocatedStringData) - sizeof(AZ::u8);
            static_assert(sizeof(Element) < BufferMaxSize, "The size of Element type must be less than the size of "
                " the AllocatedStringData struct in order to use it with the basic_string class");
            inline static constexpr size_type BufferCapacityPlusNull = BufferMaxSize / sizeof(Element);

            inline static constexpr size_type Capacity = BufferCapacityPlusNull - 1;

            ShortStringData()
            {
                // Make sure the short string buffer is null-terminated
                m_buffer[0] = Element{};
                m_packed.m_size = 0;
                m_packed.m_ssoActive = true;
            }

            // bit offset: 0, bits: 184
            Element m_buffer[BufferCapacityPlusNull];

            // Padding to make sure for Element types with a size >1
            // such as wchar_t, that the `m_size` member starts at the bit 164
            // NOTE: Uses the anonymous struct extension
            // supported by MSVC, Clang and GCC
            // Takes advantage of the empty base optimization
            // to have the StringInternal::Padding<Element> struct
            // take 0 bytes when the Element type is 1-byte type like `char`
            // When C++20 support is added, this can be changed to use [[no_unique_address]]
            struct PackedSize
                : StringInternal::Padding<Element>
            {

                // bit offset: 184, bits: 7
                AZ::u8 m_size : AZStd::numeric_limits<AZ::u8>::digits - 1;

                // bit offset: 191, bits: 1
                AZ::u8 m_ssoActive : 1;
            } m_packed;
            // Total size 192 bits(24 bytes)
        };

        struct PointerAlignedData
        {
            uintptr_t m_alignedValues[sizeof(ShortStringData) / sizeof(uintptr_t)];
        };

        static_assert(sizeof(AllocatedStringData) == sizeof(ShortStringData) && "Short string struct must be the same size"
            " as the regular allocated string struct");

        static_assert(sizeof(PointerAlignedData) == sizeof(ShortStringData) && "Pointer aligned struct must be the same size"
            " as the short string struct ");

        // The top-bit in the last byte of the AllocatedStringData and ShortStringData is used to determine if the short string optimization is being used
        union Storage
        {
            Storage() {};

            bool ShortStringOptimizationActive() const
            {
                return m_shortData.m_packed.m_ssoActive;
            }
            const_pointer GetData() const
            {
                return ShortStringOptimizationActive() ? m_shortData.m_buffer
                    : reinterpret_cast<const AllocatedStringData&>(m_shortData).m_data;
            }
            pointer GetData()
            {
                return ShortStringOptimizationActive() ? m_shortData.m_buffer
                    : reinterpret_cast<AllocatedStringData&>(m_shortData).m_data;
            }
            void SetData(pointer address)
            {
                if (!ShortStringOptimizationActive())
                {
                    reinterpret_cast<AllocatedStringData&>(m_shortData).m_data = address;
                }
                else
                {
                    AZSTD_CONTAINER_ASSERT(false, "Programming Error: string class is invoking SetData when the Short Optimization"
                        " is active. Make sure SetCapacity(<size greater than ShortStringData::Capacity>) is invoked"
                        " before calling this function.");
                }
            }
            size_type GetSize() const
            {
                return ShortStringOptimizationActive() ? m_shortData.m_packed.m_size
                    : reinterpret_cast<const AllocatedStringData&>(m_shortData).m_size;
            }
            void SetSize(size_type size)
            {
                if (ShortStringOptimizationActive())
                {
                    m_shortData.m_packed.m_size = size;
                }
                else
                {
                    reinterpret_cast<AllocatedStringData&>(m_shortData).m_size = size;
                }
            }
            size_type GetCapacity() const
            {
                return ShortStringOptimizationActive() ? m_shortData.Capacity
                    : reinterpret_cast<const AllocatedStringData&>(m_shortData).m_capacity;
            }
            void SetCapacity(size_type capacity)
            {
                if (capacity <= ShortStringData::Capacity)
                {
                    m_shortData.m_packed.m_ssoActive = true;
                }
                else
                {
                    m_shortData.m_packed.m_ssoActive = false;
                    reinterpret_cast<AllocatedStringData&>(m_shortData).m_capacity = capacity;
                }
            }
            void swap(Storage& rhs)
            {
                // Use pointer sized swaps to swap the string storage
                AZStd::aligned_storage_for_t<Storage> tempStorage;
                ::memcpy(&tempStorage, this, sizeof(Storage));
                ::memcpy(this, &rhs, sizeof(Storage));
                ::memcpy(&rhs, &tempStorage, sizeof(Storage));
            }
        private:
            ShortStringData m_shortData{};
            AllocatedStringData m_allocatedData;
            PointerAlignedData m_pointerData;
        };

        AZStd::compressed_pair<Storage, allocator_type> m_storage;

#if defined(HAVE_BENCHMARK)
        friend class Benchmark::StringBenchmarkFixture;
#endif

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        void orphan_range(pointer first, pointer last) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "basic_string::orphan_range - iterator was corrupted!");
                pointer iterPtr = static_cast<iterator*>(iter)->m_iter;

                if (iterPtr >= first && iterPtr <= last)
                {
                    // orphan the iterator
                    iter->m_container = 0;

                    if (iter->m_prevIterator)
                    {
                        iter->m_prevIterator->m_nextIterator = iter->m_nextIterator;
                    }
                    else
                    {
                        m_iteratorList = iter->m_nextIterator;
                    }

                    if (iter->m_nextIterator)
                    {
                        iter->m_nextIterator->m_prevIterator = iter->m_prevIterator;
                    }
                }

                iter = iter->m_nextIterator;
            }
        }
#endif
    };

    // AZStd::basic_string deduction guides
    template<class InputIt, class Alloc = allocator>
    basic_string(InputIt, InputIt, Alloc = Alloc()) -> basic_string<iter_value_t<InputIt>,
        AZStd::char_traits<iter_value_t<InputIt>>,
        Alloc>;

    template<class R, class Alloc = allocator, class = enable_if_t<ranges::input_range<R>>>
    basic_string(from_range_t, R&&, Alloc = Alloc())
        -> basic_string<ranges::range_value_t<R>, char_traits<ranges::range_value_t<R>>, Alloc>;

    template<class CharT, class Traits, class Alloc = allocator>
    explicit basic_string(AZStd::basic_string_view<CharT, Traits>, const Alloc& = Alloc()) ->
        basic_string<CharT,Traits, Alloc>;

    template<class CharT, class Traits, class Alloc = allocator>
    explicit basic_string(AZStd::basic_string_view<CharT, Traits>, typename allocator_traits<Alloc>::size_type,
        typename allocator_traits<Alloc>::size_type, const Alloc& = Alloc()) ->
        basic_string<CharT, Traits, Alloc>;

    template<class Element, class Traits, class Allocator>
    inline void swap(basic_string<Element, Traits, Allocator>& left, basic_string<Element, Traits, Allocator>& right)
    {
        left.swap(right);
    }

    // 21.4.8
    template<class Element, class Traits, class Allocator>
    inline basic_string<Element, Traits, Allocator> operator+(const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).append(rhs);
    }
    //template<class Element, class Traits, class Allocator>
    // basic_string1<Element,Traits,Allocator>&&    operator+( basic_string1<Element,Traits,Allocator>&& lhs, const basic_string<Element,Traits,Allocator>& rhs);
    //template<class Element, class Traits, class Allocator>
    // basic_string1<Element,Traits,Allocator>&& operator+(const basic_string<Element,Traits,Allocator>& lhs, basic_string<Element,Traits,Allocator>&& rhs);
    //template<class Element, class Traits, class Allocator>
    // basic_string1<Element,Traits,Allocator>&& operator+( basic_string<Element,Traits,Allocator>&& lhs, basic_string<Element,Traits,Allocator>&& rhs);
    template<class Element, class Traits, class Allocator>
    inline basic_string<Element, Traits, Allocator> operator+(const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).append(rhs);
    }
    //template<class Element, class Traits, class Allocator>
    //basic_string<Element,Traits,Allocator>&& operator+(const Element* lhs, basic_string<Element,Traits,Allocator>&& rhs);
    template<class Element, class Traits, class Allocator>
    inline basic_string<Element, Traits, Allocator> operator+(Element lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(1, lhs).append(rhs);
    }
    //template<class Element, class Traits, class Allocator>
    //basic_string<Element,Traits,Allocator>&& operator+(Element lhs, basic_string<Element,Traits,Allocator>&& rhs);
    template<class Element, class Traits, class Allocator>
    inline basic_string<Element, Traits, Allocator> operator+(const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs + basic_string<Element, Traits, Allocator>(rhs);
    }
    //template<class Element, class Traits, class Allocator>
    //basic_string<Element,Traits,Allocator>&& operator+( basic_string<Element,Traits,Allocator>&& lhs, const Element* rhs);
    template<class Element, class Traits, class Allocator>
    inline basic_string<Element, Traits, Allocator> operator+(const basic_string<Element, Traits, Allocator>& lhs, Element rhs)
    {
        return lhs + basic_string<Element, Traits, Allocator>(1, rhs);
    }
    //template<class Element, class Traits, class Allocator>
    // basic_string1<Element,Traits,Allocator>&& operator+( basic_string1<Element,Traits,Allocator>&& lhs, Element rhs);
    template<class Element, class Traits, class Allocator>
    inline bool operator==(const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) == 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator==(const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) == 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator==(const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) == 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator!=(const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) != 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator!=(const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) != 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator!=(const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) != 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator< (const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) < 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator< (const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) < 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator< (const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) < 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator> (const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) > 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator> (const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) > 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator> (const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) > 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator<=(const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) <= 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator<=(const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) <= 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator<=(const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) <= 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator>=(const basic_string<Element, Traits, Allocator>& lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return lhs.compare(rhs) >= 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator>=(const basic_string<Element, Traits, Allocator>& lhs, const Element* rhs)
    {
        return lhs.compare(rhs) >= 0;
    }
    template<class Element, class Traits, class Allocator>
    inline bool operator>=(const Element* lhs, const basic_string<Element, Traits, Allocator>& rhs)
    {
        return basic_string<Element, Traits, Allocator>(lhs).compare(rhs) >= 0;
    }

    template<class Element, class Traits, class Allocator, class U>
    decltype(auto) erase(basic_string<Element, Traits, Allocator>& container, const U& element)
    {
        auto iter = AZStd::remove(container.begin(), container.end(), element);
        auto removedCount = ranges::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }

    template<class Element, class Traits, class Allocator, class Predicate>
    decltype(auto) erase_if(basic_string<Element, Traits, Allocator>& container, Predicate predicate)
    {
        auto iter = AZStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = ranges::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }


#if !defined(AZ_STRING_EXPLICIT_SPECIALIZATION) && defined(AZ_COMPILER_MSVC)

    // extern explicit specialization - to speed up build time.
    extern template class basic_string<char>;
    //extern template class basic_string<wchar_t>;

#endif // defined(AZ_STRING_EXPLICIT_SPECIALIZATION) && defined(AZ_COMPILER_MSVC)

    typedef basic_string<char >     string;
    typedef basic_string<wchar_t >  wstring;
    //typedef basic_string<char16_t> u16string;
    //typedef basic_string<char32_t> u32string;

    //// 21.5: numeric conversions
    //int stoi(const string& str, size_t *idx = 0, int base = 10);
    //long stol(const string& str, size_t *idx = 0, int base = 10);
    //unsigned long stoul(const string& str, size_t *idx = 0, int base = 10);
    //long long stoll(const string& str, size_t *idx = 0, int base = 10);
    //unsigned long long stoull(const string& str, size_t *idx = 0, int base = 10);
    //float stof(const string& str, size_t *idx = 0);
    //double stod(const string& str, size_t *idx = 0);
    //long double stold(const string& str, size_t *idx = 0);
    //string to_string(long long val);
    //string to_string(unsigned long long val);
    //string to_string(long double val);
    //int stoi(const wstring& str, size_t *idx = 0, int base = 10);
    //long stol(const wstring& str, size_t *idx = 0, int base = 10);
    //unsigned long stoul(const wstring& str, size_t *idx = 0, int base = 10);
    //long long stoll(const wstring& str, size_t *idx = 0, int base = 10);
    //unsigned long long stoull(const wstring& str, size_t *idx = 0, int base = 10);
    //float stof(const wstring& str, size_t *idx = 0);
    //double stod(const wstring& str, size_t *idx = 0);
    //long double stold(const wstring& str, size_t *idx = 0);
    //wstring to_wstring(long long val);
    //wstring to_wstring(unsigned long long val);
    //wstring to_wstring(long double val);

    template<class T>
    struct hash;

    template<class Element, class Traits, class Allocator>
    struct hash< basic_string< Element, Traits, Allocator> >
    {
        using is_transparent = void;
        inline constexpr size_t operator()(const basic_string_view<Element, Traits>& value) const
        {
            return hash_string(value.begin(), value.length());
        }
    };

} // namespace AZStd
