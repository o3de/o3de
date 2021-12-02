/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <wchar.h>

#include <AzCore/std/base.h>
#include <AzCore/std/iterator.h>

#include <AzCore/std/string/string_view.h>

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
        // #1
        constexpr basic_fixed_string();

        // #2
        constexpr basic_fixed_string(size_type count, Element ch);

        // #3
        constexpr basic_fixed_string(const basic_fixed_string& rhs, size_type rhsOffset);

        // #3
        constexpr basic_fixed_string(const basic_fixed_string& rhs, size_type rhsOffset, size_type count);

        // #4
        constexpr basic_fixed_string(const_pointer ptr, size_type count);

        // #5
        constexpr basic_fixed_string(const_pointer ptr);

        // #6
        template<class InputIt, typename = enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_t>>>
        constexpr basic_fixed_string(InputIt first, InputIt last);

        // #7
        constexpr basic_fixed_string(const basic_fixed_string& rhs);

        // #8
        constexpr basic_fixed_string(basic_fixed_string&& rhs);

        // #9
        constexpr basic_fixed_string(AZStd::initializer_list<Element> ilist);

        // #10
        template<typename T, typename = AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>>>
        constexpr explicit basic_fixed_string(const T& convertibleToView);

        // #11
        template<typename T, typename = AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>>>
        constexpr basic_fixed_string(const T& convertibleToView, size_type rhsOffset, size_type count);


        // #12
        constexpr basic_fixed_string(AZStd::nullptr_t) = delete;

        constexpr operator AZStd::basic_string_view<Element, Traits>() const;

        constexpr auto begin() -> iterator;
        constexpr auto begin() const -> const_iterator;
        constexpr auto cbegin() const -> const_iterator;
        constexpr auto end() -> iterator;
        constexpr auto end() const -> const_iterator;
        constexpr auto cend() const -> const_iterator;
        constexpr auto rbegin() -> reverse_iterator;
        constexpr auto rbegin() const -> const_reverse_iterator;
        constexpr auto crbegin() const -> const_reverse_iterator;
        constexpr auto rend() -> reverse_iterator;
        constexpr auto rend() const -> const_reverse_iterator;
        constexpr auto crend() const -> const_reverse_iterator;

        constexpr auto operator=(const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto operator=(basic_fixed_string&& rhs) -> basic_fixed_string&;
        constexpr auto operator=(const_pointer ptr) -> basic_fixed_string&;
        constexpr auto operator=(Element ch) -> basic_fixed_string&;
        constexpr auto operator=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&;
        template<typename T>
        constexpr auto operator=(const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto operator=(AZStd::nullptr_t) -> basic_fixed_string& = delete;

        constexpr auto operator+=(const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto operator+=(const_pointer ptr) -> basic_fixed_string&;
        constexpr auto operator+=(Element ch) -> basic_fixed_string&;
        constexpr auto operator+=(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&;
        template<typename T>
        constexpr auto operator+=(const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;

        constexpr auto append(const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto append(const basic_fixed_string& rhs, size_type rhsOffset, size_type count) -> basic_fixed_string&;
        constexpr auto append(const_pointer ptr, size_type count) -> basic_fixed_string&;
        template<typename T>
        constexpr auto append(const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto append(const_pointer ptr) -> basic_fixed_string&;
        constexpr auto append(size_type count, Element ch) -> basic_fixed_string&;
        template<class InputIt>
        constexpr auto append(InputIt first, InputIt last)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;
        constexpr auto append(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&;

        constexpr auto assign(const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto assign(basic_fixed_string&& rhs) -> basic_fixed_string&;
        constexpr auto assign(const basic_fixed_string& rhs, size_type rhsOffset, size_type count) -> basic_fixed_string&;
        constexpr auto assign(const_pointer ptr, size_type count) -> basic_fixed_string&;
        template<typename T>
        constexpr auto assign(const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto assign(const_pointer ptr) -> basic_fixed_string&;
        constexpr auto assign(size_type count, Element ch) -> basic_fixed_string&;
        template<class InputIt>
        constexpr auto assign(InputIt first, InputIt last)
            ->enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;

        constexpr auto assign(AZStd::initializer_list<Element> ilist) -> basic_fixed_string&;

        constexpr auto insert(size_type offset, const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto insert(size_type offset, const basic_fixed_string& rhs, size_type rhsOffset, size_type count) -> basic_fixed_string&;
        constexpr auto insert(size_type offset, const_pointer ptr, size_type count) -> basic_fixed_string&;
        constexpr auto insert(size_type offset, const_pointer ptr) -> basic_fixed_string&;
        template<typename T>
        constexpr auto insert(size_type offset, const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto insert(size_type offset, size_type count, Element ch) -> basic_fixed_string&;
        constexpr auto insert(const_iterator insertPos) -> iterator;
        constexpr auto insert(const_iterator insertPos, Element ch) -> iterator;
        constexpr auto insert(const_iterator insertPos, size_type count, Element ch) -> iterator;
        template<class InputIt>
        constexpr auto insert(const_iterator insertPos, InputIt first, InputIt last)
        -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, iterator>;

        constexpr auto insert(const_iterator insertPos, AZStd::initializer_list<Element> ilist) -> iterator;

        constexpr auto erase(size_type offset = 0, size_type count = npos) -> basic_fixed_string&;
        constexpr auto erase(const_iterator erasePos) -> iterator;
        constexpr auto erase(const_iterator first, const_iterator last) -> iterator;

        constexpr auto clear() -> void;

        constexpr auto replace(size_type offset, size_type count, const basic_fixed_string& rhs) -> basic_fixed_string&;
        constexpr auto replace(size_type offset, size_type count, const basic_fixed_string& rhs, size_type rhsOffset, size_type rhsCount)
            -> basic_fixed_string&;
        template<typename T>
        constexpr auto replace(size_type offset, size_type count, const T& convertible_to_view, size_type rhsOffset, size_type rhsCount = npos)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto replace(size_type offset, size_type count, const_pointer ptr, size_type ptrCount) -> basic_fixed_string&;
        constexpr auto replace(size_type offset, size_type count, const_pointer ptr) -> basic_fixed_string&;
        template<typename T>
        constexpr auto replace(size_type offset, size_type count, const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto replace(size_type offset, size_type count, size_type num, Element ch) -> basic_fixed_string&;

        constexpr auto replace(const_iterator first, const_iterator last, const basic_fixed_string& rhs) -> basic_fixed_string&;

        constexpr auto replace(const_iterator first, const_iterator last, const_pointer ptr, size_type count) -> basic_fixed_string&;
        constexpr auto replace(const_iterator first, const_iterator last, const_pointer ptr) -> basic_fixed_string&;
        template<typename T>
        constexpr auto replace(const_iterator first, const_iterator last, const T& convertible_to_view)
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, basic_fixed_string&>;
        constexpr auto replace(const_iterator first, const_iterator last, size_type count, Element ch) -> basic_fixed_string&;
        template<class InputIt>
        constexpr auto replace(const_iterator first, const_iterator last, InputIt first2, InputIt last2)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;
        constexpr auto replace(const_iterator first, const_iterator last, AZStd::initializer_list<Element> ilist) -> basic_fixed_string&;

        constexpr auto at(size_type offset) -> reference;
        constexpr auto at(size_type offset) const -> const_reference;
        constexpr auto operator[](size_type offset) -> reference;
        constexpr auto operator[](size_type offset) const -> const_reference;

        constexpr auto front() -> reference;
        constexpr auto front() const -> const_reference;
        constexpr auto back() -> reference;
        constexpr auto back() const -> const_reference;

        constexpr auto push_back(Element ch) -> void;
        constexpr auto pop_back() -> void;

        constexpr auto c_str() const -> const_pointer;
        constexpr auto data() const -> const_pointer;
        constexpr auto data() -> pointer;
        constexpr auto length() const -> size_type;
        constexpr auto size() const -> size_type;
        constexpr auto capacity() const -> size_type;
        constexpr auto max_size() const -> size_type;

        constexpr auto resize(size_type newSize) -> void;
        constexpr auto resize(size_type newSize, Element ch) -> void;
        constexpr auto resize_no_construct(size_type newSize) -> void;

        constexpr auto reserve(size_type newCapacity = 0) -> void;

        constexpr auto empty() const -> bool;
        constexpr auto copy(Element* dest, size_type count, size_type offset = 0) const -> size_type;

        constexpr auto swap(basic_fixed_string& rhs) -> void;

        // C++23 contains
        constexpr auto contains(const basic_fixed_string& other) const -> bool;
        constexpr auto contains(Element ch) const -> bool;
        constexpr auto contains(const_pointer s) const -> bool;

        constexpr auto find(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type;
        constexpr auto find(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto find(const_pointer ptr, size_type offset = 0) const -> size_type;
        constexpr auto find(Element ch, size_type offset = 0) const -> size_type;
        template<typename T>
        constexpr auto find(const T& convertibleToView, size_type offset = 0) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto rfind(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type;
        constexpr auto rfind(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto rfind(const_pointer ptr, size_type offset = npos) const -> size_type;
        constexpr auto rfind(Element ch, size_type offset = npos) const -> size_type;
        template<typename T>
        constexpr auto rfind(const T& convertibleToView, size_type offset = npos) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto find_first_of(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type;
        constexpr auto find_first_of(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto find_first_of(const_pointer ptr, size_type offset = 0) const -> size_type;
        constexpr auto find_first_of(Element ch, size_type offset = 0) const -> size_type;
        template<typename T>
        constexpr auto find_first_of(const T& convertibleToView, size_type offset = 0) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto find_first_not_of(const basic_fixed_string& rhs, size_type offset = 0) const -> size_type;
        constexpr auto find_first_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto find_first_not_of(const_pointer ptr, size_type offset = 0) const -> size_type;
        constexpr auto find_first_not_of(Element ch, size_type offset = 0) const -> size_type;
        template<typename T>
        constexpr auto find_first_not_of(const T& convertibleToView, size_type offset = 0) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto find_last_of(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type;
        constexpr auto find_last_of(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto find_last_of(const_pointer ptr, size_type offset = npos) const -> size_type;
        constexpr auto find_last_of(Element ch, size_type offset = npos) const -> size_type;
        template<typename T>
        constexpr auto find_last_of(const T& convertibleToView, size_type offset = npos) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto find_last_not_of(const basic_fixed_string& rhs, size_type offset = npos) const -> size_type;
        constexpr auto find_last_not_of(const_pointer ptr, size_type offset, size_type count) const -> size_type;
        constexpr auto find_last_not_of(const_pointer ptr, size_type offset = npos) const -> size_type;
        constexpr auto find_last_not_of(Element ch, size_type offset = npos) const -> size_type;
        template<typename T>
        constexpr auto find_last_not_of(const T& convertibleToView, size_type offset = npos) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>>
            && !is_convertible_v<const T&, const Element*>, size_type>;

        constexpr auto substr(size_type offset = 0, size_type count = npos) const -> basic_fixed_string;

        constexpr auto compare(const basic_fixed_string& rhs) const -> int;
        constexpr auto compare(size_type offset, size_type count, const basic_fixed_string& rhs) const -> int;
        constexpr auto compare(size_type offset, size_type count, const basic_fixed_string& rhs, size_type rhsOffset, size_type rhsCount) const -> int;
        constexpr auto compare(const_pointer ptr) const -> int;
        constexpr auto compare(size_type offset, size_type count, const_pointer ptr) const -> int;
        constexpr auto compare(size_type offset, size_type count, const_pointer ptr, size_type ptrCount) const -> int;

        // starts_with
        template<typename T>
        constexpr auto starts_with(const T& prefix) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, bool>;
        constexpr auto starts_with(value_type prefix) const -> bool;
        constexpr auto starts_with(const_pointer prefix) const -> bool;

        // ends_with
        template<typename T>
        constexpr auto ends_with(const T& suffix) const
            -> AZStd::enable_if_t<is_convertible_v<const T&, basic_string_view<Element, Traits>> && !is_convertible_v<const T&, const Element*>, bool>;
        constexpr auto ends_with(value_type suffix) const -> bool;
        constexpr auto ends_with(const_pointer suffix) const -> bool;


        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. For fixed_string case this performs the same action as clear()
        */
        constexpr auto leak_and_reset() -> void;

        static decltype(auto) format_arg(const char* format, va_list argList);
        static decltype(auto) format(const char* format, ...) AZ_FORMAT_ATTRIBUTE(1, 2);
        static decltype(auto) format_arg(const wchar_t* format, va_list argList);
        static decltype(auto) format(const wchar_t* format, ...);

    protected:
        template<class InputIt>
        constexpr auto append_iter(InputIt first, InputIt last)
           -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;

        template<class InputIt>
        constexpr auto construct_iter(InputIt first, InputIt last)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>>;

        template<class InputIt>
        constexpr auto assign_iter(InputIt first, InputIt last)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;

        template<class InputIt>
        constexpr auto insert_iter(const_iterator insertPos, InputIt first, InputIt last)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, iterator>;

        template<class InputIt>
        constexpr auto replace_iter(const_iterator first, const_iterator last, InputIt first2, InputIt last2)
            -> enable_if_t<Internal::is_input_iterator_v<InputIt> && !is_convertible_v<InputIt, size_type>, basic_fixed_string&>;

        constexpr auto fits_in_capacity(size_type newSize) -> bool;

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

    template<class T>
    struct hash;

    template<class Element, size_t MaxElementCount, class Traits>
    struct hash<basic_fixed_string<Element, MaxElementCount, Traits>>;

} // namespace AZStd

#include <AzCore/std/string/fixed_string.inl>
