/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/createdestroy.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/limits.h>


namespace AZStd
{
    namespace StringInternal
    {
        constexpr size_t min_size(size_t left, size_t right) { return (left < right) ? left : right; }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT char_find(const CharT* s, size_t count, CharT ch, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            const CharT* foundIter = Traits::find(s, count, ch);
            return foundIter ? AZStd::distance(s, foundIter) : npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (offset > size || count > size)
            {
                return npos;
            }
            if (count == 0)
            {
                return offset; // count == 0 means that highest index should be returned
            }

            size_t searchRange = size - offset;
            for (size_t searchIndex = offset; searchIndex < size;)
            {
                size_t charFindIndex = char_find<Traits>(&data[searchIndex], searchRange, *ptr, npos);
                if (charFindIndex == npos)
                {
                    return npos;
                }
                size_t foundIndex = searchIndex + charFindIndex;
                if (foundIndex + count > size)
                {
                    return npos; // the rest of the string doesnt fit in the remainder of the data buffer
                }
                if (Traits::compare(&data[foundIndex], ptr, count) == 0)
                {
                    return foundIndex;
                }

                searchRange = size - (foundIndex + 1);
                searchIndex = foundIndex + 1;
            }

            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find(const CharT* data, SizeT size, CharT c, SizeT offset, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (offset > size)
            {
                return npos;
            }

            size_t charFindIndex = char_find<Traits>(&data[offset], size - offset, c, npos);
            return charFindIndex != npos ? offset + charFindIndex : npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT rfind(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0 || count > size)
            {
                return npos;
            }

            if (count == 0)
            {
                return offset > size ? size : offset; // count == 0 means that highest index should be returned
            }

            // Add one to offset so that for loop condition can check against 0 as the breakout condition
            size_t lastIndex = StringInternal::min_size(offset, size - count) + 1;

            for (; lastIndex; --lastIndex)
            {
                if (Traits::eq(*ptr, data[lastIndex - 1]) && Traits::compare(ptr, &data[lastIndex - 1], count) == 0)
                {
                    return lastIndex - 1;
                }
            }

            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT rfind(const CharT* data, SizeT size, CharT c, SizeT offset, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            // Add one to offset so that for loop condition can check against 0 as the breakout condition
            size_t lastIndex = offset > size ? size : offset + 1;

            for (; lastIndex; --lastIndex)
            {
                if (Traits::eq(c, data[lastIndex - 1]))
                {
                    return lastIndex - 1;
                }
            }

            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_first_of(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            if (count == 0)
            {
                return npos; // count == 0 means that the set of valid entries is empty
            }

            for (size_t firstIndex = offset; firstIndex < size; ++firstIndex)
            {
                size_t charFindIndex = char_find<Traits>(ptr, count, data[firstIndex], npos);
                if (charFindIndex != npos)
                {
                    return firstIndex;
                }
            }
            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_last_of(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            if (count == 0)
            {
                return npos; // count == 0 means that the set of valid entries is empty
            }

            // Add one to offset so that for loop condition can against 0 as the breakout condition
            size_t lastIndex = offset > size ? size : offset + 1;
            for (; lastIndex; --lastIndex)
            {
                size_t charFindIndex = char_find<Traits>(ptr, count, data[lastIndex - 1], npos);
                if (charFindIndex != npos)
                {
                    return lastIndex - 1;
                }
            }

            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_first_not_of(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            if (count == 0)
            {
                return offset; // count == 0 means that the every character is part of the set of valid entries
            }

            for (size_t firstIndex = offset; firstIndex < size; ++firstIndex)
            {
                size_t charFindIndex = char_find<Traits>(ptr, count, data[firstIndex], npos);
                if (charFindIndex == npos)
                {
                    return firstIndex;
                }
            }
            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_first_not_of(const CharT* data, SizeT size, CharT c, SizeT offset, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            for (size_t firstIndex = offset; firstIndex < size; ++firstIndex)
            {
                if (!Traits::eq(c, data[firstIndex]))
                {
                    return firstIndex;
                }
            }
            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_last_not_of(const CharT* data, SizeT size, const CharT* ptr, SizeT offset, SizeT count, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            // Add one to offset so that for loop condition can check against 0 as the breakout condition
            size_t lastIndex = offset > size ? size : offset + 1;
            if (count == 0)
            {
                return lastIndex - 1; // count == 0 means that the every character is part of the set of valid entries
            }

            for (; lastIndex; --lastIndex)
            {
                size_t charFindIndex = char_find<Traits>(ptr, count, data[lastIndex - 1], npos);
                if (charFindIndex == npos)
                {
                    return lastIndex - 1;
                }
            }

            return npos;
        }

        template<class Traits, class CharT, class SizeT>
        constexpr SizeT find_last_not_of(const CharT* data, SizeT size, CharT c, SizeT offset, SizeT npos = static_cast<SizeT>(-1)) noexcept
        {
            if (size == 0)
            {
                return npos;
            }

            // Add one to offset so that for loop condition can against 0 as the breakout condition
            size_t lastIndex = offset > size ? size : offset + 1;
            for (; lastIndex; --lastIndex)
            {
                if (!Traits::eq(c, data[lastIndex - 1]))
                {
                    return lastIndex - 1;
                }
            }
            return npos;
        }
    }

    template<class Element>
    struct char_traits
    {
        // properties of a string or stream char_type element
        using char_type = Element;
        using int_type = AZStd::conditional_t<AZStd::is_same_v<Element, char>, int,
            AZStd::conditional_t<AZStd::is_same_v<Element, wchar_t>, wint_t, int>
        >;
        using pos_type = std::streampos;
        using off_type = std::streamoff;
        using state_type = mbstate_t;

        static constexpr void assign(char_type& left, const char_type& right) noexcept { left = right; }
        static constexpr char_type* assign(char_type* dest, size_t count, char_type ch) noexcept
        {
            AZ_Assert(dest, "Invalid input!");
            for (char_type* iter = dest; count; --count, ++iter)
            {
                assign(*iter, ch);
            }
            return dest;
        }
        static constexpr bool eq(char_type left, char_type right) noexcept { return left == right; }
        static constexpr bool lt(char_type left, char_type right) noexcept { return left < right; }
        static constexpr int compare(const char_type* s1, const char_type* s2, size_t count) noexcept
        {
            // Regression in VS2017 15.8 and 15.9 where __builtin_memcmp fails in valid checks in constexpr evaluation
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC < 1915 || AZ_COMPILER_MSVC > 1916
            if constexpr (AZStd::is_same_v<char_type, char>)
            {
                return __builtin_memcmp(s1, s2, count);
            }
            else if constexpr (AZStd::is_same_v<char_type, wchar_t>)
            {
                return __builtin_wmemcmp(s1, s2, count);
            }
            else
#endif
            {
                for (; count; --count, ++s1, ++s2)
                {
                    if (lt(*s1, *s2))
                    {
                        return -1;
                    }
                    else if (lt(*s2, *s1))
                    {
                        return 1;
                    }
                }
                return 0;
            }
        }
        static constexpr size_t length(const char_type* s) noexcept
        {
            if constexpr (AZStd::is_same_v<char_type, char>)
            {
                return __builtin_strlen(s);
            }
            else if constexpr (AZStd::is_same_v<char_type, wchar_t>)
            {
                return __builtin_wcslen(s);
            }
            else
            {
                size_t strLength{};
                for (; *s; ++s, ++strLength)
                {
                    ;
                }
                return strLength;
            }
        }
        static constexpr const char_type* find(const char_type* s, size_t count, const char_type& ch) noexcept
        {
                // There is a bug with the __builtin_char_memchr intrinsic in Visual Studio 2017 15.8.x and 15.9.x
                // It reads in one more additional character than the value of count.
                // This is probably due to assuming null-termination
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC < 1915 || AZ_COMPILER_MSVC > 1916
            if constexpr (AZStd::is_same_v<char_type, char>)
            {
                return __builtin_char_memchr(s, ch, count);
            }
            else if constexpr (AZStd::is_same_v<char_type, wchar_t>)
            {
                return __builtin_wmemchr(s, ch, count);

            }
            else
#endif
            {
                for (; count; --count, ++s)
                {
                    if (eq(*s, ch))
                    {
                        return s;
                    }
                }
                return nullptr;
            }
        }
        static constexpr char_type* move(char_type* dest, const char_type* src, size_t count) noexcept
        {
            AZ_Assert(dest != nullptr && src != nullptr, "Invalid input!");
            if (count == 0)
            {
                return dest;
            }
            char_type* result = dest;
            // The less than(<), greater than(>) and other variants(<=, >=)
            // Cannot be compare pointers within a constexpr due to the potential for undefined behavior
            // per the bullet linked in the C++ standard at http://eel.is/c++draft/expr.compound#expr.rel-5
            // Now clang and gcc compilers allow the use of this relation operators in a constexpr, but
            // msvc is not so forgiving
            // So a workaround of iterating the src pointer, checking for equality with the dest pointer
            // is used to check for overlap
            auto should_copy_forward = [](const char_type* dest1, const char_type* src2, size_t count2) constexpr -> bool
            {
                bool dest_less_than_src{ true };
                for(const char_type* src_iter = src2; src_iter != src2 + count2; ++src_iter)
                {
                    if (src_iter == dest1)
                    {
                        dest_less_than_src = false;
                        break;
                    }
                }
                return dest_less_than_src;
            };

            if (should_copy_forward(dest, src, count))
            {
                copy(dest, src, count);
            }
            else
            {
                copy_backward(dest, src, count);
            }

            return result;
        }
        static constexpr char_type* copy(char_type* dest, const char_type* src, size_t count) noexcept
        {
            AZ_Assert(dest != nullptr && src != nullptr, "Invalid input!");
            char_type* result = dest;
            for(; count; --count, ++dest, ++src)
            {
                assign(*dest, *src);
            }
            return result;
        }
        // Extension for constexpr workarounds: Addresses of a string literal cannot be compared at compile time and MSVC and clang will just refuse to compile the constexpr
        // Adding a copy_backwards overload that always copies backwards.
        static constexpr char_type* copy_backward(char_type* dest, const char_type*src, size_t count) noexcept
        {
            char_type* result = dest;
            dest += count;
            src += count;
            for (; count; --count)
            {
                assign(*--dest, *--src);
            }
            return result;
        }

        static constexpr char_type to_char_type(int_type c) noexcept { return static_cast<char_type>(c); }
        static constexpr int_type to_int_type(char_type c) noexcept
        {
            if constexpr (AZStd::is_same_v<char_type, char>)
            {
                return static_cast<int_type>(static_cast<unsigned char>(c));
            }
            else
            {
                return static_cast<int_type>(c);
            }
        }
        static constexpr bool eq_int_type(int_type left, int_type right) noexcept { return left == right; }
        static constexpr int_type eof() noexcept { return static_cast<int_type>(-1); }
        static constexpr int_type not_eof(int_type c) noexcept { return c != eof() ? c : !eof(); }
    };

    /**
     * Immutable string wrapper based on boost::const_string and std::string_view. When we operate on
     * const char* we don't know if this points to NULL terminated string or just a char array.
     * to have a clear distinction between them we provide this wrapper.
     */
    template <class Element, class Traits = AZStd::char_traits<Element>>
    class basic_string_view
    {
    public:
        using traits_type = Traits;
        using value_type = Element;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        using reference = value_type&;
        using const_reference = const value_type&;

        using size_type = size_t;
        using difference_type = ptrdiff_t;

        inline static constexpr size_type npos = size_type(-1);

        using iterator = const value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        constexpr basic_string_view() noexcept = default;

        constexpr basic_string_view(const_pointer s)
            : m_begin(s)
            , m_size(s ? traits_type::length(s) : 0)
        { }

        constexpr basic_string_view(const_pointer s, size_type count)
            : m_begin(s)
            , m_size(count)
        {}

        template <typename It, typename End, typename = AZStd::enable_if_t<
            Internal::satisfies_contiguous_iterator_concept_v<It>
            && is_same_v<typename AZStd::iterator_traits<It>::value_type, value_type>
            && !is_convertible_v<End, size_type>>
        >
        constexpr basic_string_view(It first, End last)
            : m_begin(AZStd::to_address(first))
            , m_size(AZStd::to_address(last) - AZStd::to_address(first))
        {}

        constexpr basic_string_view(const basic_string_view&) noexcept = default;
        constexpr basic_string_view(basic_string_view&& other)
        {
            swap(other);
        }

        // C++23 overload to prevent initializing a string_view via a nullptr or integer type
        constexpr basic_string_view(AZStd::nullptr_t) = delete;

        constexpr const_reference operator[](size_type index) const { return data()[index]; }
        /// Returns value, not reference. If index is out of bounds, 0 is returned (can't be reference).
        constexpr value_type at(size_type index) const
        {
            AZ_Assert(index < size(), "pos value is out of range");
            return index >= size() ? value_type(0) : data()[index];
        }

        constexpr const_reference front() const
        {
            AZ_Assert(!empty(), "string_view::front(): string is empty");
            return data()[0];
        }

        constexpr const_reference back() const
        {
            AZ_Assert(!empty(), "string_view::back(): string is empty");
            return data()[m_size - 1];
        }

        constexpr const_pointer data() const { return m_begin; }

        constexpr size_type length() const { return size(); }
        constexpr size_type size() const { return m_size; }
        constexpr size_type max_size() const { return (std::numeric_limits<size_type>::max)(); } //< Wrapping the numeric_limits<size_type>::max function in parenthesis to get around the issue with windows.h defining max as a macro
        constexpr bool      empty() const { return size() == 0; }

        constexpr void remove_prefix(size_type n)
        {
            AZ_Assert(n <= size(), "Attempting to remove prefix larger than string size");
            m_begin += n;
            m_size -= n;
        }

        constexpr void remove_suffix(size_type n)
        {
            AZ_Assert(n <= size(), "Attempting to remove suffix larger than string size");
            m_size -= n;
        }

        constexpr basic_string_view& operator=(const basic_string_view& s) noexcept = default;

        constexpr void swap(basic_string_view& s) noexcept
        {
            const_pointer tmp1 = m_begin;
            size_type tmp2 = m_size;
            m_begin = s.m_begin;
            m_size = s.m_size;
            s.m_begin = tmp1;
            s.m_size = tmp2;
        }
        friend bool constexpr operator==(basic_string_view s1, basic_string_view s2) noexcept
        {
            return s1.length() == s2.length() && (s1.length() == 0 || traits_type::compare(s1.data(), s2.data(), s1.length()) == 0);
        }

        friend constexpr bool operator!=(basic_string_view s1, basic_string_view s2) noexcept { return !(s1 == s2); }
        friend constexpr bool operator<(basic_string_view lhs, basic_string_view rhs) noexcept { return lhs.compare(rhs) < 0; }
        friend constexpr bool operator>(basic_string_view lhs, basic_string_view rhs) noexcept { return lhs.compare(rhs) > 0; }
        friend constexpr bool operator<=(basic_string_view lhs, basic_string_view rhs) noexcept { return lhs.compare(rhs) <= 0; }
        friend constexpr bool operator>=(basic_string_view lhs, basic_string_view rhs) noexcept { return lhs.compare(rhs) >= 0; }

        constexpr iterator         begin() const noexcept { return data(); }
        constexpr iterator         end() const noexcept { return data() + size(); }
        constexpr const_iterator   cbegin() const noexcept { return begin(); }
        constexpr const_iterator   cend() const noexcept { return end(); }
        constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
        constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // [string.view.modifiers], modifiers:
        constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const
        {
            AZ_Assert(pos <= size(), "Position of bytes to copy to destination is out of string_view range");
            if (pos > size())
            {
                return 0;
            }
            size_type rlen = StringInternal::min_size(count, size() - pos);
            Traits::copy(dest, data() + pos, rlen);
            return rlen;
        }

        constexpr basic_string_view substr(size_type pos = 0, size_type count = npos) const
        {
            AZ_Assert(pos <= size(), "Cannot create substring where position is larger than size");
            return pos > size() ? basic_string_view() : basic_string_view(data() + pos, StringInternal::min_size(count, size() - pos));
        }

        constexpr int compare(basic_string_view other) const
        {
            size_t cmpSize = StringInternal::min_size(size(), other.size());
            int cmpval = cmpSize == 0 ? 0 : Traits::compare(data(), other.data(), cmpSize);
            if (cmpval == 0)
            {
                if (size() == other.size())
                {
                    return 0;
                }
                else if (size() < other.size())
                {
                    return -1;
                }

                return 1;
            }

            return cmpval;
        }

        constexpr int compare(size_type pos1, size_type count1, basic_string_view other) const
        {
            return substr(pos1, count1).compare(other);
        }

        constexpr int compare(size_type pos1, size_type count1, basic_string_view sv, size_type pos2, size_type count2) const
        {
            return substr(pos1, count1).compare(sv.substr(pos2, count2));
        }

        constexpr int compare(const_pointer s) const
        {
            return compare(basic_string_view(s));
        }

        constexpr int compare(size_type pos1, size_type count1, const_pointer s) const
        {
            return substr(pos1, count1).compare(basic_string_view(s));
        }

        constexpr int compare(size_type pos1, size_type count1, const_pointer s, size_type count2) const
        {
            return substr(pos1, count1).compare(basic_string_view(s, count2));
        }

        // starts_with
        constexpr bool starts_with(basic_string_view prefix) const
        {
            return size() >= prefix.size() && compare(0, prefix.size(), prefix) == 0;
        }

        constexpr bool starts_with(value_type prefix) const
        {
            return starts_with(basic_string_view(&prefix, 1));
        }

        constexpr bool starts_with(const_pointer prefix) const
        {
            return starts_with(basic_string_view(prefix));
        }

        // ends_with
        constexpr bool ends_with(basic_string_view suffix) const
        {
            return size() >= suffix.size() && compare(size() - suffix.size(), npos, suffix) == 0;
        }

        constexpr bool ends_with(value_type suffix) const
        {
            return ends_with(basic_string_view(&suffix, 1));
        }

        constexpr bool ends_with(const_pointer suffix) const
        {
            return ends_with(basic_string_view(suffix));
        }

        // C++23 contains
        constexpr bool contains(basic_string_view other) const
        {
            return find(other) != npos;
        }

        constexpr bool contains(value_type c) const
        {
            return find(c) != npos;
        }

        constexpr bool contains(const_pointer s) const
        {
            return find(s) != npos;
        }

        // find
        constexpr size_type find(basic_string_view other, size_type pos = 0) const
        {
            return StringInternal::find<traits_type>(data(), size(), other.data(), pos, other.size(), npos);
        }

        constexpr size_type find(value_type c, size_type pos = 0) const
        {
            return StringInternal::find<traits_type>(data(), size(), c, pos, npos);
        }

        constexpr size_type find(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::find(): received nullptr");
            return find(basic_string_view{ s, count }, pos);
        }

        constexpr size_type find(const_pointer s, size_type pos = 0) const
        {
            AZ_Assert(s, "string_view::find(): received nullptr");
            return find(basic_string_view{ s, traits_type::length(s) }, pos);
        }

        // rfind
        constexpr size_type rfind(basic_string_view s, size_type pos = npos) const
        {
            return StringInternal::rfind<traits_type>(data(), size(), s.data(), pos, s.size(), npos);
        }

        constexpr size_type rfind(value_type c, size_type pos = npos) const
        {
            return StringInternal::rfind<traits_type>(data(), size(), c, pos, npos);
        }

        constexpr size_type rfind(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::rfind(): received nullptr");
            return rfind(basic_string_view{ s, count }, pos);
        }

        constexpr size_type rfind(const_pointer s, size_type pos = npos) const
        {
            AZ_Assert(s, "string_view::rfind(): received nullptr");
            return rfind(basic_string_view{ s, traits_type::length(s) }, pos);
        }

        // find_first_of
        constexpr size_type find_first_of(basic_string_view s, size_type pos = 0) const
        {
            return StringInternal::find_first_of<traits_type>(data(), size(), s.data(), pos, s.size(), npos);
        }

        constexpr size_type find_first_of(value_type c, size_type pos = 0) const
        {
            return find(c, pos);
        }

        constexpr size_type find_first_of(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::find_first_of(): received nullptr");
            return find_first_of(basic_string_view{ s, count }, pos);
        }

        constexpr size_type find_first_of(const_pointer s, size_type pos = 0) const
        {
            AZ_Assert(s, "string_view::find_first_of(): received nullptr");
            return find_first_of(basic_string_view{ s, traits_type::length(s) }, pos);
        }

        // find_last_of
        constexpr size_type find_last_of(basic_string_view s, size_type pos = npos) const
        {
            return StringInternal::find_last_of<traits_type>(data(), size(), s.data(), pos, s.size(), npos);
        }

        constexpr size_type find_last_of(value_type c, size_type pos = npos) const
        {
            return rfind(c, pos);
        }

        constexpr size_type find_last_of(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::find_last_of(): received nullptr");
            return find_last_of(basic_string_view{ s, count }, pos);
        }

        constexpr size_type find_last_of(const_pointer s, size_type pos = npos) const
        {
            AZ_Assert(s, "string_view::find_last_of(): received nullptr");
            return find_last_of(basic_string_view{ s, traits_type::length(s) }, pos);
        }

        // find_first_not_of
        constexpr size_type find_first_not_of(basic_string_view s, size_type pos = 0) const
        {
            return StringInternal::find_first_not_of<traits_type>(data(), size(), s.data(), pos, s.size(), npos);
        }

        constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const
        {
            return StringInternal::find_first_not_of<traits_type>(data(), size(), c, pos, npos);
        }

        constexpr size_type find_first_not_of(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::find_first_not_of(): received nullptr");
            return find_first_not_of(basic_string_view{ s, count }, pos);
        }

        constexpr size_type find_first_not_of(const_pointer s, size_type pos = 0) const
        {
            AZ_Assert(s, "string_view::find_first_not_of(): received nullptr");
            return find_first_not_of(basic_string_view{ s, traits_type::length(s) }, pos);
        }

        // find_last_not_of
        constexpr size_type find_last_not_of(basic_string_view s, size_type pos = npos) const
        {
            return StringInternal::find_last_not_of<traits_type>(data(), size(), s.data(), pos, s.size(), npos);
        }

        constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const
        {
            return StringInternal::find_last_not_of<traits_type>(data(), size(), c, pos, npos);
        }

        constexpr size_type find_last_not_of(const_pointer s, size_type pos, size_type count) const
        {
            AZ_Assert(count == 0 || s, "string_view::find_last_not_of(): received nullptr");
            return find_last_not_of(basic_string_view{ s, count }, pos);
        }

        constexpr size_type find_last_not_of(const_pointer s, size_type pos = npos) const
        {
            AZ_Assert(s, "string_view::find_last_not_of(): received nullptr");
            return find_last_not_of(basic_string_view{ s, traits_type::length(s) }, pos);
        }

    private:
        const_pointer m_begin{};
        size_type m_size{};
    };

    using string_view = basic_string_view<char>;
    using wstring_view = basic_string_view<wchar_t>;

    template<class Element, class Traits = AZStd::char_traits<Element>>
    using basic_const_string = basic_string_view<Element, Traits>;
    using const_string = string_view;
    using const_wstring = wstring_view;

    template <class Element, class Traits = AZStd::char_traits<Element>>
    constexpr typename basic_string_view<Element, Traits>::const_iterator begin(basic_string_view<Element, Traits> sv)
    {
        return sv.begin();
    }

    template <class Element, class Traits = AZStd::char_traits<Element>>
    constexpr typename basic_string_view<Element, Traits>::const_iterator end(basic_string_view<Element, Traits> sv)
    {
        return sv.end();
    }

    inline namespace literals
    {
        inline namespace string_view_literals
        {
            constexpr string_view operator "" _sv(const char* str, size_t len) noexcept
            {
                return string_view{ str, len };
            }

            constexpr wstring_view operator "" _sv(const wchar_t* str, size_t len) noexcept
            {
                return wstring_view{ str, len };
            }
        }
    }

    /// For string hashing we are using FNV-1a algorithm 64 bit version.
    template<class RandomAccessIterator>
    constexpr size_t hash_string(RandomAccessIterator first, size_t length)
    {
        size_t hash = 14695981039346656037ULL;
        constexpr size_t fnvPrime = 1099511628211ULL;

        const RandomAccessIterator last(first + length);
        for (; first != last; ++first)
        {
            hash ^= static_cast<size_t>(*first);
            hash *= fnvPrime;
        }
        return hash;
    }

    template<class T>
    struct hash;
    template<class Element, class Traits>
    struct hash<basic_string_view<Element, Traits>>
    {
        constexpr size_t operator()(const basic_string_view<Element, Traits>& value) const
        {
            // from the string class
            return hash_string(value.begin(), value.length());
        }
    };

} // namespace AZStd

//! Use this macro to simplify safe printing of a string_view which may not be null-terminated.
//! Example: AZStd::string::format("Safely formatted: %.*s", AZ_STRING_ARG(myString));
#define AZ_STRING_ARG(str) aznumeric_cast<int>(str.size()), str.data()

//! Can be used with AZ_STRING_ARG for convenience, rather than manually including "%.*s" in format strings
//! Example: AZStd::string::format("Safely formatted: " AZ_STRING_FORMAT, AZ_STRING_ARG(myString));
#define AZ_STRING_FORMAT "%.*s"
