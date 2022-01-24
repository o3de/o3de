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
    template <class T, size_t Extent>
    template <class It, enable_if_t<contiguous_iterator<It> &&
        Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
        Extent == dynamic_extent>*>
    inline constexpr span<T, Extent>::span(It first, size_type length)
        : m_data{ to_address(first) }
        , m_size{ length }
    {}

    template <class T, size_t Extent>
    template <class It, enable_if_t<contiguous_iterator<It> &&
        Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
        Extent != dynamic_extent, int>>
    inline constexpr span<T, Extent>::span(It first, size_type length)
        : m_data{ to_address(first) }
        , m_size{ length }
    {}

    template <class T, size_t Extent>
    template <class It, class End, enable_if_t<contiguous_iterator<It> &&
        Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
        sized_sentinel_for<End, It> &&
        Extent == dynamic_extent>*>
    inline constexpr span<T, Extent>::span(It first, End last)
        : m_data{to_address(first)}
        , m_size(last - first)
    {}

    template <class T, size_t Extent>
    template <class It, class End, enable_if_t<contiguous_iterator<It> &&
        Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
        sized_sentinel_for<End, It> &&
        Extent != dynamic_extent, int>>
    inline constexpr span<T, Extent>::span(It first, End last)
        : m_data{to_address(first)}
        , m_size(last - first)
    {}

    template <class T, size_t Extent>
    template <size_t N, class>
    inline constexpr span<T, Extent>::span(type_identity_t<element_type>(&arr)[N]) noexcept
        : m_data{ arr }
        , m_size{ N }
    {}

    template <class T, size_t Extent>
    template <class U, size_t N, class>
    inline constexpr span<T, Extent>::span(array<U, N>& arr) noexcept
        : m_data{ arr.data() }
        , m_size{ arr.size() }
    {}
    template <class T, size_t Extent>
    template <class U, size_t N, class>
    inline constexpr span<T, Extent>::span(const array<U, N>& arr) noexcept
        : m_data{ arr.data() }
        , m_size{ arr.size() }
    {}

    template <class T, size_t Extent>
    template <class R, class>
    inline constexpr span<T, Extent>::span(R&& r)
        : m_data{ ranges::data(r) }
        , m_size{ ranges::size(r) }
    {
        AZ_Assert(Extent == dynamic_extent || Extent == m_size, "The extent of the span is non dynamic,"
            " therefore the range size must match the extent. Extent=%zu, Range size=%zu",
            Extent, ranges::size(r));
    }

    template <class T, size_t Extent>
    template <class U, size_t OtherExtent, class>
    inline constexpr span<T, Extent>::span(const span<U, OtherExtent>& other)
        : m_data{ other.data() }
        , m_size{ other.size() }
    {
        AZ_Assert(Extent == dynamic_extent || Extent == m_size, "The extent of the span is non dynamic,"
            " therefore the current size of the other span must match the extent. Extent=%zu, Other span size=%zu",
            Extent, other.size());
    }

    // subviews
    template <class T, size_t Extent>
    template <size_t Count>
    inline constexpr auto span<T, Extent>::first() const -> span<element_type, Count>
    {
        static_assert(Count <= Extent, "Count is larger than the Extent of the span, a subview of the first"
            " Count elemnts of the span cannot be returned");
        AZ_Assert(Count <= size(), "Count %zu is larger than span size %zu", Count, size());
        return span<element_type, Count>{data(), Count};
    }
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::first(size_type count) const -> span<element_type, dynamic_extent>
    {
        AZ_Assert(count <= size(), "Count %zu is larger than current size of span size %zu", count, size());
        return { data(), count };
    }

    template <class T, size_t Extent>
    template <size_t Count>
    inline constexpr auto span<T, Extent>::last() const -> span<element_type, Count>
    {
        static_assert(Count <= Extent, "Count is larger than the Extent of the span, a subview of the last"
            " Count elements of the span cannot be returned");
        AZ_Assert(Count <= size(), "Count %zu is larger than span size %zu", Count, size());
        return span<element_type, Count>{data() + (size() - Count), Count};
    }
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::last(size_type count) const -> span<element_type, dynamic_extent>
    {
        AZ_Assert(count <= size(), "Count %zu is larger than span size %zu", count, size());
        return { data() + (size() - count), count };
    }

    template <class T, size_t Extent>
    template <size_t Offset, size_t Count>
    inline constexpr auto span<T, Extent>::subspan() const
    {
        static_assert(Offset <= Extent && (Count == dynamic_extent || Count <= Extent - Offset),
            "Subspan Offset must <= span Extent and the Count must be either dynamic_extent"
            " or <= (span Extent - Offset)");
        AZ_Assert(Offset <= size() && (Count == dynamic_extent || Count <= size() - Offset),
            "Either the Subspan Offset %zu is larger than the span size %zu or the Count != dynamic_extent and"
            " its value %zu is greater than \"span size - Offset\" %zu",
            Offset, size(), Count, size() - Offset);
        using return_type = span<element_type, Count != dynamic_extent ? Extent - Offset : dynamic_extent>;
        return return_type{ data() + Offset, Count != dynamic_extent ? Count : size() - Offset };
    }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::subspan(size_type offset, size_type count) const -> span<element_type, dynamic_extent>
    {
        AZ_Assert(offset <= size() && (count == dynamic_extent || count <= size() - offset),
            "Either the Subspan offset %zu is larger than the span size %zu or the count != dynamic_extent and"
            " its value %zu is greater than \"span size - offset\" %zu",
            offset, size(), count, size() - offset);
        return { data() + offset, count != dynamic_extent ? count : size() - offset };
    }


    // observers
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::size() const noexcept -> size_type { return m_size; }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::size_bytes() const noexcept -> size_type { return m_size * sizeof(element_type); }

    template <class T, size_t Extent>
    inline constexpr bool span<T, Extent>::empty() const noexcept{ return size() == 0; }

    // element access
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::operator[](size_type index) const -> reference
    {
        AZ_Assert(index < size(), "index value is out of range");
        return data()[index];
    }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::front() const -> reference
    {
        AZ_Assert(!empty(), "span cannot be empty when invoking front");
        return *data();
    }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::back() const -> reference
    {
        AZ_Assert(!empty(), "span cannot be empty when invoking back");
        return *(data() + (size() - 1));
    }

    // iterator support
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::data() const noexcept -> pointer { return m_data; }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::begin() const noexcept -> iterator{ return m_data; }
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::end() const noexcept -> iterator { return m_data + m_size; }

    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::rbegin() const noexcept -> reverse_iterator { return AZStd::make_reverse_iterator(end()); }
    template <class T, size_t Extent>
    inline constexpr auto span<T, Extent>::rend() const noexcept -> reverse_iterator { return AZStd::make_reverse_iterator(begin()); }


    template <class ElementType, size_t Extent>
    inline auto as_bytes(span<ElementType, Extent> s) noexcept
        -> span<const byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>
    {
        return span<const byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
            reinterpret_cast<const byte*>(s.data()), s.size_bytes());
    }


    template <class ElementType, size_t Extent>
    inline auto as_writable_bytes(span<ElementType, Extent> s) noexcept
        -> enable_if_t<!is_const_v<ElementType>, span<byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>>
    {
        return span<byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
            reinterpret_cast<byte*>(s.data()), s.size_bytes());
    }
} // namespace AZStd
