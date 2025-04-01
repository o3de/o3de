/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/span_fwd.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/typetraits/type_identity.h>

namespace AZStd::Internal
{
    template <class T>
    inline constexpr bool is_std_array = false;

    template <class U, size_t N>
    inline constexpr bool is_std_array<::AZStd::array<U, N>> = true;

    template <class T>
    inline constexpr bool is_std_span = false;

    template <class U, size_t Extent>
    inline constexpr bool is_std_span<::AZStd::span<U, Extent>> = true;

    template <class T, class U>
    inline constexpr bool is_array_convertible = is_convertible_v<T(*)[], U(*)[]>;

}

namespace AZStd
{
    /**
     * Full C++20 implementation of span done using the C++ draft at https://eel.is/c++draft/views.
     * It does not maintain storage for the data, 
     * but just hold a pointer to mark the beginning and the size for the elements. 
     * It can be constructed any type that models the C++ contiguous_range concept
     * such like array, vector, fixed_vector, raw-array, string_view, string, etc... . 
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
    template <class T, size_t Extent>
    class span
    {
    public:
        using element_type = T;
        using value_type = AZStd::remove_cv_t<T>;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        using pointer = element_type*;
        using const_pointer = const element_type*;

        using reference = element_type&;
        using const_reference = const element_type&;


        using iterator = element_type*;
        using const_iterator = const element_type*;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        inline static constexpr size_t extent = Extent;

        constexpr span() noexcept = default;

        ~span() = default;

        template <class It, enable_if_t<contiguous_iterator<It> &&
            Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
            Extent == dynamic_extent>* = nullptr>
        constexpr span(It first, size_type length);

        template <class It, enable_if_t<contiguous_iterator<It> &&
            Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
            Extent != dynamic_extent, int> = 0>
        constexpr explicit span(It first, size_type length);

        template <class It, class End, enable_if_t<contiguous_iterator<It> &&
            Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
            sized_sentinel_for<End, It> &&
            Extent == dynamic_extent>* = nullptr>
        constexpr span(It first, End last);

        template <class It, class End, enable_if_t<contiguous_iterator<It> &&
            Internal::is_array_convertible<remove_reference_t<iter_reference_t<It>>, T> &&
            sized_sentinel_for<End, It> &&
            Extent != dynamic_extent, int> = 0>
        constexpr explicit span(It first, End last);

        template<size_t N, class = enable_if_t<extent == dynamic_extent || N == Extent>>
        constexpr span(type_identity_t<element_type> (&arr)[N]) noexcept;

        template <class U, size_t N, class = enable_if_t<(extent == dynamic_extent || N == Extent) && is_convertible_v<U, T>>>
        constexpr span(array<U, N>& data) noexcept;
        template <class U, size_t N, class = enable_if_t<(extent == dynamic_extent || N == Extent) && is_convertible_v<U, T>>>
        constexpr span(const array<U, N>& data) noexcept;

        template <class R, class = enable_if_t<ranges::contiguous_range<R> &&
            ranges::sized_range<R> &&
            (ranges::borrowed_range<R> || is_const_v<element_type>) &&
            !Internal::is_std_span<remove_cvref_t<R>> &&
            !Internal::is_std_array<remove_cvref_t<R>> &&
            !is_array_v<remove_cvref_t<R>> &&
            Internal::is_array_convertible<remove_reference_t<ranges::range_reference_t<R>>, element_type> >>
        constexpr span(R&& r);

        template <class U, size_t OtherExtent, class = enable_if_t<
            (extent == dynamic_extent || OtherExtent == dynamic_extent || extent == OtherExtent)
            && Internal::is_array_convertible<U, element_type> >>
        constexpr span(const span<U, OtherExtent>& other);

        constexpr span(const span&) noexcept = default;

        constexpr span& operator=(const span& other) = default;

        // subviews -> https://eel.is/c++draft/views#span.sub
        template <size_t Count>
        constexpr span<element_type, Count> first() const;
        template <size_t Count>
        constexpr span<element_type, Count> last() const;
        template <size_t Offset, size_t Count = dynamic_extent>
        constexpr auto subspan() const;

        constexpr span<element_type, dynamic_extent> first(size_type count) const;
        constexpr span<element_type, dynamic_extent> last(size_type count) const;
        constexpr span<element_type, dynamic_extent> subspan(size_type offset, size_type count = dynamic_extent) const;

        // observers - https://eel.is/c++draft/views#span.obs
        constexpr size_type size() const noexcept;
        constexpr size_type size_bytes() const noexcept;

        [[nodiscard]] constexpr bool empty() const noexcept;

        // element access - https://eel.is/c++draft/views#span.elem
        constexpr reference operator[](size_type index) const;
        constexpr reference front() const;
        constexpr reference back() const;
        constexpr pointer data() const noexcept;

        // iterator support - https://eel.is/c++draft/views#span.iterators
        constexpr iterator         begin() const noexcept;
        constexpr iterator         end() const noexcept;

        constexpr reverse_iterator rbegin() const noexcept;
        constexpr reverse_iterator rend() const noexcept;

    private:
        pointer m_data{};
        size_type m_size{};
    };

    // deduction guides https://eel.is/c++draft/views#span.deduct
    template <class It, class EndOrSize, class = enable_if_t<contiguous_iterator<It>>>
    span(It, EndOrSize) -> span<remove_reference_t<iter_reference_t<It>>>;

    // array deductions
    template <class T, size_t N>
    span(T(&)[N]) -> span<T, N>;
    template <class T, size_t N>
    span(array<T, N>&) -> span<T, N>;
    template <class T, size_t N>
    span(const array<T, N>&) -> span<const T, N>;

    template <class R, class = enable_if_t<ranges::contiguous_range<R>>>
    span(R&&) -> span<remove_reference_t<ranges::range_reference_t<R>>>;

    // [span.objectrep], views of object representation
    template <class ElementType, size_t Extent>
    auto as_bytes(span<ElementType, Extent> s) noexcept
        -> span<const AZStd::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>;

    template <class ElementType, size_t Extent>
    auto as_writable_bytes(span<ElementType, Extent> s) noexcept
        -> enable_if_t<!is_const_v<ElementType>, span<AZStd::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>>;

} // namespace AZStd

namespace AZStd::ranges
{
    template<class ElementType, size_t Extent>
    inline constexpr bool enable_view<span<ElementType, Extent>> = true;
    template<class ElementType, size_t Extent>
    inline constexpr bool enable_borrowed_range<span<ElementType, Extent>> = true;
}

#include <AzCore/std/containers/span.inl>
