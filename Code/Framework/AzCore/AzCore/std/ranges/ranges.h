/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/typetraits/remove_cv.h>

namespace AZStd
{
    // alias std:: reverse_iterator names into AZStd::
    using std::make_reverse_iterator;
}

namespace AZStd::ranges
{
    // Range variable templates
    template<class T>
    inline constexpr bool enable_borrowed_range = false;

    template<class T>
    inline constexpr bool disable_sized_range = false;

    namespace Internal
    {
        // Variadic template which maps types to true For SFINAE
        template <class... Args>
        constexpr bool sfinae_trigger_v = true;

        template <class T>
        constexpr bool is_lvalue_or_borrowable = is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>;

        //! begin
        template <class T, typename = void>
        constexpr bool has_member_begin = false;
        template <class T>
        constexpr bool has_member_begin<T, void_t<decltype(declval<T&>().begin())>> = true;

        template <class T, typename = void>
        constexpr bool has_unqualified_begin = false;
        template <class T>
        constexpr bool has_unqualified_begin<T, void_t<decltype(begin(declval<T&>()))>>
            = !has_member_begin<T> && AZStd::Internal::is_class_or_enum<T>;

        template<class T>
        void begin(T&) = delete;
        template<class T>
        void begin(const T&) = delete;

        struct begin_fn
        {
            template<class T>
            constexpr auto operator()(T& t) const noexcept ->
                enable_if_t<is_array_v<T> && sfinae_trigger_v<remove_all_extents_t<T>>,
                decltype(t + 0)>
            {
                return t + 0;
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).begin())) ->
                enable_if_t<input_or_output_iterator<decltype(t.begin())>&& is_lvalue_or_borrowable<T>
                && !is_array_v<T>&& has_member_begin<T>,
                decltype(AZStd::forward<T>(t).begin())>
            {
                return AZStd::forward<T>(t).begin();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(begin(AZStd::forward<T>(t)))) ->
                enable_if_t<input_or_output_iterator<decltype(begin(t))>&& is_lvalue_or_borrowable<T>
                && !is_array_v<T> && has_unqualified_begin<T>,
                decltype(begin(AZStd::forward<T>(t)))>
            {
                return begin(AZStd::forward<T>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::begin_fn begin{};
    }

    template<class T>
    using iterator_t = decltype(ranges::begin(declval<T&>()));

    namespace Internal
    {
        template <class T>
        constexpr bool has_iterator_t = has_member_begin<T>;

        //! end
        template <class T, typename = void>
        constexpr bool has_member_end = false;
        template <class T>
        constexpr bool has_member_end<T, void_t<decltype(declval<T&>().end())>> = true;

        template <class T, typename = void>
        constexpr bool has_unqualified_end = false;
        template <class T>
        constexpr bool has_unqualified_end<T, void_t<decltype(end(declval<T&>()))>>
            = !has_member_end<T> && AZStd::Internal::is_class_or_enum<T>;

        template<class T>
        void end(T&) = delete;
        template<class T>
        void end(const T&) = delete;

        struct end_fn
        {
            template<class T>
            constexpr auto operator()(T& t) const noexcept ->
                enable_if_t<is_array_v<T> && extent_v<T> != 0,
                decltype(t + extent_v<T>)>
            {
                return t + extent_v<T>;
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).end())) ->
                enable_if_t<sentinel_for<decltype(t.end()), iterator_t<T>>&& is_lvalue_or_borrowable<T>
                && !is_array_v<T> && has_member_end<T>,
                decltype(AZStd::forward<T>(t).end())>
            {
                return AZStd::forward<T>(t).end();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(end(AZStd::forward<T>(t)))) ->
                enable_if_t<sentinel_for<decltype(end(t)), iterator_t<T>>&& is_lvalue_or_borrowable<T>
                && !is_array_v<T> && has_unqualified_end<T>,
                decltype(end(AZStd::forward<T>(t)))>
            {
                return end(AZStd::forward<T>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::end_fn end{};
    }

    namespace Internal
    {
        //! cbegin
        struct cbegin_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::begin(static_cast<const T&>(t))))
                ->enable_if_t<is_lvalue_reference_v<T>, decltype(ranges::begin(static_cast<const T&>(t)))>
            {
                return ranges::begin(static_cast<const T&>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::begin(static_cast<const T&>(t)))) ->
                enable_if_t<!is_lvalue_reference_v<T>, decltype(ranges::begin(static_cast<const T&&>(t)))>
            {
                return ranges::begin(static_cast<const T&&>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::cbegin_fn cbegin{};
    }

    namespace Internal
    {
        //! cend
        struct cend_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::end(static_cast<const T&>(t))))
                ->enable_if_t<is_lvalue_reference_v<T>, decltype(ranges::end(static_cast<const T&>(t)))>
            {
                return ranges::end(static_cast<const T&>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::end(static_cast<const T&>(t)))) ->
                enable_if_t<!is_lvalue_reference_v<T>, decltype(ranges::end(static_cast<const T&&>(t)))>
            {
                return ranges::end(static_cast<const T&&>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::cend_fn cend{};
    }

    namespace Internal
    {
        //! rbegin
        template <class T, class = void>
        constexpr bool has_member_rbegin = false;
        template <class T>
        constexpr bool has_member_rbegin<T, void_t<decltype(declval<T&>().rbegin())>> = true;

        template <class T, class = void>
        constexpr bool has_unqualified_rbegin = false;
        template <class T>
        constexpr bool has_unqualified_rbegin<T, void_t<decltype(rbegin(declval<T&>()))>>
            = !has_member_rbegin<T> && AZStd::Internal::is_class_or_enum<T>;

        template <class T, class = void>
        constexpr bool has_bidirectional_rbegin = false;
        template <class T>
        constexpr bool has_bidirectional_rbegin<T, enable_if_t<
            same_as<decltype(ranges::begin(declval<T&>())), decltype(ranges::end(declval<T&>()))>
            && bidirectional_iterator<decltype(ranges::begin(declval<T&>()))>
            && bidirectional_iterator<decltype(ranges::end(declval<T&>()))>
            >> = !has_member_rbegin<T> && !has_unqualified_rbegin<T>;


        template<class T>
        void rbegin(T&) = delete;
        template<class T>
        void rbegin(const T&) = delete;

        struct rbegin_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).rbegin())) ->
                enable_if_t<input_or_output_iterator<decltype(t.rbegin())> && is_lvalue_or_borrowable<T>
                && has_member_rbegin<T>,
                decltype(AZStd::forward<T>(t).rbegin())>
            {
                return AZStd::forward<T>(t).rbegin();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(rbegin(AZStd::forward<T>(t)))) ->
                enable_if_t<input_or_output_iterator<decltype(rbegin(t))> && is_lvalue_or_borrowable<T>
                && has_unqualified_rbegin<T>,
                decltype(rbegin(AZStd::forward<T>(t)))>
            {
                return rbegin(AZStd::forward<T>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::make_reverse_iterator(ranges::end(AZStd::forward<T>(t))))) ->
                enable_if_t<has_bidirectional_rbegin<T>,
                decltype(AZStd::make_reverse_iterator(ranges::end(AZStd::forward<T>(t))))>
            {
                return AZStd::make_reverse_iterator(ranges::end(AZStd::forward<T>(t)));
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::rbegin_fn rbegin{};
    }

    namespace Internal
    {
        //! rend
        template <class T, class = void>
        constexpr bool has_member_rend = false;
        template <class T>
        constexpr bool has_member_rend<T, void_t<decltype(declval<T&>().rend())>> = true;

        template <class T, class = void>
        constexpr bool has_unqualified_rend = false;
        template <class T>
        constexpr bool has_unqualified_rend<T, void_t<decltype(rend(declval<T&>()))>>
            = !has_member_rend<T> && AZStd::Internal::is_class_or_enum<T>;

        template <class T, class = void>
        constexpr bool has_bidirectional_rend = false;
        template <class T>
        constexpr bool has_bidirectional_rend<T, enable_if_t<
            same_as<decltype(ranges::begin(declval<T&>())), decltype(ranges::end(declval<T&>()))>
            && bidirectional_iterator<decltype(ranges::begin(declval<T&>()))>
            && bidirectional_iterator<decltype(ranges::end(declval<T&>()))>
            >> = !has_member_rend<T> && !has_unqualified_rend<T>;

        template<class T>
        void rend(T&) = delete;
        template<class T>
        void rend(const T&) = delete;

        struct rend_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).rend())) ->
                enable_if_t<sentinel_for<decltype(t.rend()), decltype(ranges::rbegin(t))> && is_lvalue_or_borrowable<T>
                && has_member_rend<T>,
                decltype(AZStd::forward<T>(t).rend())>
            {
                return AZStd::forward<T>(t).rend();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(rend(AZStd::forward<T>(t)))) ->
                enable_if_t<sentinel_for<decltype(rend(t)), decltype(ranges::rbegin(t))> && is_lvalue_or_borrowable<T>
                && has_unqualified_rend<T>,
                decltype(rend(AZStd::forward<T>(t)))>
            {
                return rend(AZStd::forward<T>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::make_reverse_iterator(ranges::begin(AZStd::forward<T>(t))))) ->
                enable_if_t<has_bidirectional_rend<T>,
                decltype(AZStd::make_reverse_iterator(ranges::begin(AZStd::forward<T>(t))))>
            {
                return AZStd::make_reverse_iterator(ranges::begin(AZStd::forward<T>(t)));
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::rend_fn rend{};
    }

    namespace Internal
    {
        //! crbegin
        struct crbegin_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rbegin(static_cast<const T&>(t))))
                ->enable_if_t<is_lvalue_reference_v<T>, decltype(ranges::rbegin(static_cast<const T&>(t)))>
            {
                return ranges::rbegin(static_cast<const T&>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rbegin(static_cast<const T&>(t)))) ->
                enable_if_t<!is_lvalue_reference_v<T>, decltype(ranges::rbegin(static_cast<const T&&>(t)))>
            {
                return ranges::rbegin(static_cast<const T&&>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::crbegin_fn crbegin{};
    }

    namespace Internal
    {
        //! crend
        struct crend_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rend(static_cast<const T&>(t))))
                ->enable_if_t<is_lvalue_reference_v<T>, decltype(ranges::rend(static_cast<const T&>(t)))>
            {
                return ranges::rend(static_cast<const T&>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rend(static_cast<const T&>(t)))) ->
                enable_if_t<!is_lvalue_reference_v<T>, decltype(ranges::rend(static_cast<const T&&>(t)))>
            {
                return ranges::rend(static_cast<const T&&>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::crend_fn crend{};
    }

    namespace Internal
    {
        //! size
        template <class T, class = void>
        constexpr bool has_member_size = false;
        template <class T>
        constexpr bool has_member_size<T, void_t<decltype(declval<T&>().size())>> = true;

        template <class T, class = void>
        constexpr bool has_unqualified_size = false;
        template <class T>
        constexpr bool has_unqualified_size<T, void_t<decltype(size(declval<T&>()))>>
            = !has_member_size<T> && AZStd::Internal::is_class_or_enum<T>;

        template <class T, class = void>
        constexpr bool has_end_subtract_begin = false;
        template <class T>
        constexpr bool has_end_subtract_begin<T, void_t<decltype(ranges::end(declval<T&>()) - ranges::begin(declval<T&>()))>>
            = !has_member_size<T> && !has_unqualified_size<T>;

        template<class T>
        void size(T&) = delete;
        template<class T>
        void size(const T&) = delete;

        struct size_fn
        {
            template<class T>
            constexpr auto operator()(T&) const noexcept ->
                enable_if_t<is_array_v<T> && extent_v<T> != 0,
                decltype(extent_v<T>)>
            {
                return extent_v<T>;
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).size())) ->
                enable_if_t<!disable_sized_range<remove_cv_t<T>> && has_member_size<T>
                && AZStd::Internal::is_integer_like<decltype(AZStd::forward<T>(t).size())>,
                decltype(AZStd::forward<T>(t).size())>
            {
                return AZStd::forward<T>(t).size();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(size(AZStd::forward<T>(t)))) ->
                enable_if_t<!disable_sized_range<remove_cv_t<T>>
                && has_unqualified_size<T>
                && AZStd::Internal::is_integer_like<decltype(size(AZStd::forward<T>(t)))>,
                decltype(size(AZStd::forward<T>(t)))>
            {
                return size(AZStd::forward<T>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)))) ->
                enable_if_t<
                has_end_subtract_begin<T>
                && sized_sentinel_for<decltype(ranges::end(AZStd::forward<T>(t))), decltype(ranges::begin(AZStd::forward<T>(t)))>
                && forward_iterator<decltype(ranges::begin(AZStd::forward<T>(t)))>,
                AZStd::make_unsigned_t<decltype(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)))>>
            {
                using size_type = AZStd::make_unsigned_t<decltype(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)))>;
                return static_cast<size_type>(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::size_fn size{};
    }

    namespace Internal
    {
        //! ssize
        struct ssize_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::size(t))) ->
                enable_if_t<(sizeof(ptrdiff_t) > sizeof(make_signed_t<decltype(ranges::size(t))>)), ptrdiff_t>
            {
                return static_cast<ptrdiff_t>(ranges::size(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::size(t))) ->
                enable_if_t<sizeof(ptrdiff_t) <= sizeof(make_signed_t<decltype(ranges::size(t))>), make_signed_t<decltype(ranges::size(t))>>
            {
                using ssize_type = make_signed_t<decltype(ranges::size(t))>;
                return static_cast<ssize_type>(ranges::size(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::ssize_fn ssize{};
    }

    namespace Internal
    {
        //! empty
        template <class T, class = void>
        constexpr bool has_member_empty = false;
        template <class T>
        constexpr bool has_member_empty<T, enable_if_t<convertible_to<decltype(declval<T>().empty()), bool>>> = true;

        template <class T, class = void>
        constexpr bool has_size_compare_to_0 = false;
        template <class T>
        constexpr bool has_size_compare_to_0<T, enable_if_t<convertible_to<decltype(ranges::size(declval<T&>()) == 0), bool> >>
            = !has_member_empty<T>;

        template <class T, class = void>
        constexpr bool has_begin_compare_to_end = false;

        template <class T>
        constexpr bool has_begin_compare_to_end<T,
            enable_if_t<convertible_to<decltype(ranges::begin(declval<T&>()) == ranges::end(declval<T&>())), bool> >>
            = !has_member_empty<T> && !has_size_compare_to_0<T>;

        struct empty_fn
        {
            template<class T>
            [[nodiscard]] constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).empty())) ->
                enable_if_t<is_lvalue_or_borrowable<T> && has_member_empty<T>, bool>
            {
                return AZStd::forward<T>(t).empty();
            }

            template<class T>
            [[nodiscard]] constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::size(AZStd::forward<T>(t)) == 0)) ->
                enable_if_t<is_lvalue_or_borrowable<T> && has_size_compare_to_0<T>, bool>
            {
                return ranges::size(AZStd::forward<T>(t)) == 0;
            }

            template<class T>
            [[nodiscard]] constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::begin(AZStd::forward<T>(t)) == ranges::end(AZStd::forward<T>(t)))) ->
                enable_if_t<is_lvalue_or_borrowable<T> && has_begin_compare_to_end<T>, bool>
            {
                return ranges::begin(AZStd::forward<T>(t)) == ranges::end(AZStd::forward<T>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::empty_fn empty{};
    }

    namespace Internal
    {
        //! data
        template <class T, class = void>
        constexpr bool has_member_data = false;
        template <class T>
        constexpr bool has_member_data<T, void_t<decltype(declval<T>().data())>> = true;

        template <class T, class = void>
        constexpr bool has_qualified_ranges_begin = false;
        template <class T>
        constexpr bool has_qualified_ranges_begin<T, enable_if_t<contiguous_iterator<decltype(ranges::begin(declval<T&>()))>> >
            = !has_member_data<T>;

        struct data_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).data())) ->
                enable_if_t<is_lvalue_or_borrowable<T> && has_member_data<T>,
                decltype(AZStd::forward<T>(t).data())>
            {
                return AZStd::forward<T>(t).data();
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::to_address(ranges::begin(AZStd::forward<T>(t))))) ->
                enable_if_t<is_lvalue_or_borrowable<T> && has_qualified_ranges_begin<T>,
                decltype(AZStd::to_address(ranges::begin(AZStd::forward<T>(t))))>
            {
                return AZStd::to_address(ranges::begin(AZStd::forward<T>(t)));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::data_fn data{};
    }

    namespace Internal
    {
        //! cdata
        struct cdata_fn
        {
            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::data(static_cast<const T&>(t)))) ->
                enable_if_t<is_lvalue_reference_v<T>, decltype(ranges::data(static_cast<const T&>(t)))>
            {
                return ranges::data(static_cast<const T&>(t));
            }

            template<class T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::data(static_cast<const T&>(t)))) ->
                enable_if_t<!is_lvalue_reference_v<T>, decltype(ranges::data(static_cast<const T&&>(t)))>
            {
                return ranges::data(static_cast<const T&&>(t));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::cdata_fn cdata{};
    }
}

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class T, class = void>
        constexpr bool range_impl = false;
        template<class T>
        constexpr bool range_impl<T, void_t<
            decltype(ranges::begin(declval<T&>())), decltype(ranges::end(declval<T&>()))>> = true;
    }

    // Models range concept
    template<class T>
    /*concept*/ constexpr bool range = Internal::range_impl<T>;

    // sentinal type can now be defined after the range concept has been modeled
    template<class R>
    using sentinel_t = enable_if_t<range<R>, decltype(ranges::end(declval<R&>()))>;

    // Models borrowed range concept
    template<class T>
    /*concept*/ constexpr bool borrowed_range = range<T>
        && (is_lvalue_reference_v<T> || enable_borrowed_range<remove_cvref_t<T>>);

    struct dangling
    {
        constexpr dangling() = default;
        template <typename T>
        constexpr dangling(T&&...) noexcept {}
    };

    template<class R>
    using borrowed_iterator_t = conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

    // Models sized range concept
    namespace Internal
    {
        template<class T, class = void>
        constexpr bool sized_range_impl = false;
        template<class T>
        constexpr bool sized_range_impl<T, enable_if_t<range<T>
            && sfinae_trigger_v<decltype(ranges::size(declval<T&>()))> >> = true;
    }

    template<class T>
    /*concept*/ constexpr bool sized_range = Internal::sized_range_impl<T>;

    namespace Internal
    {
        template<class R, class T, class = void>
        constexpr bool output_range_impl = false;
        template<class R, class T>
        constexpr bool output_range_impl<R, T, enable_if_t<has_iterator_t<R>>> = range<R> && output_iterator<iterator_t<R>, T>;
    }

    template<class R, class T>
    /*concept*/ constexpr bool output_range = Internal::output_range_impl<R, T>;

    namespace Internal
    {
        template<class T, class = void>
        constexpr bool input_range_impl = false;
        template<class T>
        constexpr bool input_range_impl<T, enable_if_t<has_iterator_t<T>>> = range<T> && input_iterator<iterator_t<T>>;
    }

    template<class T>
    /*concept*/ constexpr bool input_range = Internal::input_range_impl<T>;

    namespace Internal
    {
        template<class T, class = void>
        constexpr bool forward_range_impl = false;
        template<class T>
        constexpr bool forward_range_impl<T, enable_if_t<has_iterator_t<T>>> = input_range<T> && forward_iterator<iterator_t<T>>;
    }

    template<class T>
    /*concept*/ constexpr bool forward_range = Internal::forward_range_impl<T>;

    namespace Internal
    {
        template<class T, class = void>
        constexpr bool bidirectional_range_impl = false;
        template<class T>
        constexpr bool bidirectional_range_impl<T, enable_if_t<has_iterator_t<T>>> = forward_range<T> && bidirectional_iterator<iterator_t<T>>;
    }

    template<class T>
    /*concept*/ constexpr bool bidirectional_range = Internal::bidirectional_range_impl<T>;

    namespace Internal
    {
        template<class T, class = void>
        constexpr bool random_access_range_impl = false;
        template<class T>
        constexpr bool random_access_range_impl<T, enable_if_t<has_iterator_t<T>>> = bidirectional_range<T> && random_access_iterator<iterator_t<T>>;
    }

    template<class T>
    /*concept*/ constexpr bool random_access_range = Internal::random_access_range_impl<T>;

    template<class R>
    using range_size_t = enable_if_t<sized_range<R>, decltype(ranges::size(declval<R&>()))>;
    template<class R>
    using range_difference_t = enable_if_t<range<R>, iter_difference_t<iterator_t<R>>>;
    template<class R>
    using range_value_t = enable_if_t<range<R>, iter_value_t<iterator_t<R>>>;
    template<class R>
    using range_reference_t = enable_if_t<range<R>, iter_reference_t<iterator_t<R>>>;
    template<class R>
    using range_rvalue_reference_t = enable_if_t<range<R>, iter_rvalue_reference_t<iterator_t<R>>>;


    namespace Internal
    {
        template<class T, class = void>
        constexpr bool contiguous_range_impl = false;
        template<class T>
        constexpr bool contiguous_range_impl<T, enable_if_t<random_access_range<T>
            && contiguous_iterator<iterator_t<T>>
            && same_as<decltype(ranges::data(declval<T&>())), add_pointer_t<range_reference_t<T>>> >> = true;
    }

    template<class T>
    /*concept*/ constexpr bool contiguous_range = Internal::contiguous_range_impl<T>;

    template<class T>
    /*concept*/ constexpr bool common_range = range<T> && same_as<iterator_t<T>, sentinel_t<T>>;
}

namespace AZStd::ranges
{
    // iterator operations
    // ranges::advance
    namespace Internal
    {
        struct advance_fn
        {
            template<class I>
            constexpr auto operator()(I& i, iter_difference_t<I> n) const ->
                enable_if_t<input_or_output_iterator<I>>
            {
                if constexpr (random_access_iterator<I>)
                {
                    i += n;
                }
                else
                {
                    for (; n > 0; ++i, --n) {}

                    // The Precondition is that if I is not a bidirectional iterator, n must be positive
                    if constexpr (bidirectional_iterator<I>)
                    {
                        for (; n < 0; --i, ++n) {}
                    }
                }
            }

            template<class I, class S>
            constexpr auto operator()(I& i, S bound) const ->
                enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>>
            {
                if constexpr (assignable_from<I&, S>)
                {
                    i = AZStd::move(bound);
                }
                else if constexpr (sized_sentinel_for<S, I>)
                {
                    operator()(i, bound - i);
                }
                else
                {
                    for (; i != bound; ++i) {}
                }
            }

            template<class I, class S>
            constexpr auto operator()(I& i, iter_difference_t<I> n, S bound) const ->
                enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>, iter_difference_t<I>>
            {
                if constexpr (sized_sentinel_for<S, I>)
                {
                    if (const auto dist = bound - i;
                        (n > 0 && n > dist) || (n < 0 && n < dist))
                    {
                        // advance is limited to the i reach bound
                        operator()(i, bound);
                        return n - dist;
                    }
                    else if (n != 0)
                    {
                        // advance is limited by the value of n
                        operator()(i, n);
                        return 0;
                    }

                    return 0;
                }
                else
                {
                    for (; i != bound && n > 0; ++i, --n) {}
                    if constexpr (bidirectional_iterator<I> && same_as<I, S>)
                    {
                        for (; i != bound && n < 0; --i, ++n) {}
                    }

                    return n;
                }
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::advance_fn advance{};
    }

    // ranges::distance
    namespace Internal
    {
        struct distance_fn
        {
            template<class I, class S>
            constexpr auto operator()(I first, S last) const ->
                enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I> && !sized_sentinel_for<S, I>,
                iter_difference_t<I>>
            {
                // Since S is not a sized sentinel, can only increment from first to last
                iter_difference_t<I> result{};
                for (; first != last; ++first, ++result) {}

                return result;
            }

            template<class I, class S>
            constexpr auto operator()(const I& first, const S& last) const ->
                enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I> && sized_sentinel_for<S, I>,
                iter_difference_t<I>>
            {
                return last - first;
            }

            template<class R>
            constexpr auto operator()(R&& r) const ->
                enable_if_t<range<R>, range_difference_t<R>>
            {
                if constexpr (sized_range<R>)
                {
                    return ranges::size(r);
                }
                else
                {
                    operator()(ranges::begin(r), ranges::end(r));
                }
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::distance_fn distance{};
    }

    // ranges::next
    namespace Internal
    {
        struct next_fn
        {
            template<class I>
            constexpr auto operator()(I x) const ->
                enable_if_t<input_or_output_iterator<I>, I>
            {
                ++x;
                return x;
            }

            template<class I>
            constexpr auto operator()(I x, iter_difference_t<I> n) const ->
                enable_if_t<input_or_output_iterator<I>, I>
            {
                ranges::advance(x, n);
                return x;
            }

            template<class I, class S>
            constexpr auto operator()(I x, S bound) const ->
                enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>, I>
            {
                ranges::advance(x, bound);
                return x;
            }

            template<class I, class S>
            constexpr auto operator()(I x, iter_difference_t<I> n, S bound) const ->
                enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>, I>
            {
                ranges::advance(x, n, bound);
                return x;
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::next_fn next{};
    }

    //ranges::prev
    namespace Internal
    {
        struct prev_fn
        {
            template<class I>
            constexpr auto operator()(I x) const ->
                enable_if_t<bidirectional_iterator<I>, I>
            {
                --x;
                return x;
            }

            template<class I>
            constexpr auto operator()(I x, iter_difference_t<I> n) const ->
                enable_if_t<bidirectional_iterator<I>, I>
            {
                ranges::advance(x, -n);
                return x;
            }

            template<class I, class S>
            constexpr auto operator()(I x, iter_difference_t<I> n, S bound) const ->
                enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>, I>
            {
                ranges::advance(x, -n, bound);
                return x;
            }
        };
    }

    inline namespace customization_point_object
    {
        inline constexpr Internal::prev_fn prev{};
    }
}

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class T>
        constexpr bool is_initializer_list = false;
        template<class E>
        constexpr bool is_initializer_list<initializer_list<E>> = true;
    }

    //! views
    // view interface can be used with non-constant class types
    template<class D, class = void>
    class view_interface;
    template<class D>
    class view_interface< D, enable_if_t<is_class_v<D> && same_as<D, remove_cv_t<D>> >>
    {
    private:
        constexpr D& derived() noexcept
        {
            return static_cast<D&>(*this);
        }
        constexpr const D& derived() const noexcept
        {
            return static_cast<const D&>(*this);
        }

    public:
        template <class Derived = D>
        constexpr auto empty() -> enable_if_t<forward_range<Derived>, bool>
        {
            return ranges::begin(derived()) == ranges::end(derived());
        }
        template <class Derived = D>
        constexpr auto empty() const  -> enable_if_t<forward_range<const Derived>, bool>
        {
            return ranges::begin(derived()) == ranges::end(derived());
        }

        template <class Derived = D, class = void_t<decltype(ranges::empty(declval<Derived>()))>>
        constexpr explicit operator bool() const noexcept(noexcept(ranges::empty(derived())))
        {
            return !ranges::empty(derived());
        }

        template <class Derived = D>
        constexpr auto data() ->
            enable_if_t<contiguous_iterator<iterator_t<Derived>>, decltype(to_address(ranges::begin(derived())))>
        {
            return to_address(ranges::begin(derived()));
        }
        template <class Derived = D>
        constexpr auto data() const ->
            enable_if_t<contiguous_iterator<iterator_t<const Derived>>, decltype(to_address(ranges::begin(derived())))>
        {
            return to_address(ranges::begin(derived()));
        }

        template <class Derived = D>
        constexpr auto size() ->
            enable_if_t<forward_range<Derived> && sized_sentinel_for<sentinel_t<Derived>, iterator_t<Derived>>,
            decltype(ranges::end(derived()) - ranges::begin(derived()))>
        {
            return ranges::end(derived()) - ranges::begin(derived());
        }
        template <class Derived = D>
        constexpr auto size() const ->
            enable_if_t<forward_range<const Derived>&& sized_sentinel_for<sentinel_t<const Derived>, iterator_t<const Derived>>,
            decltype(ranges::end(derived()) - ranges::begin(derived()))>
        {
            return ranges::end(derived()) - ranges::begin(derived());
        }

        template <class Derived = D>
        constexpr auto front() ->
            enable_if_t<forward_range<Derived>, decltype(*ranges::begin(derived()))>
        {
            return *ranges::begin(derived());
        }
        template <class Derived = D>
        constexpr auto front() const ->
            enable_if_t<forward_range<const Derived>, decltype(*ranges::begin(derived()))>
        {
            return *ranges::begin(derived());
        }

        template <class Derived = D>
        constexpr auto back() ->
            enable_if_t<bidirectional_range<Derived> && common_range<Derived>, decltype(*ranges::prev(ranges::end(derived())))>
        {
            return *ranges::prev(ranges::end(derived()));
        }

        template <class Derived = D>
        constexpr auto back() const ->
            enable_if_t<bidirectional_range<const Derived>&& common_range<const Derived>, decltype(*ranges::prev(ranges::end(derived())))>
        {
            return *ranges::prev(ranges::end(derived()));
        }

        template<class R = D>
        constexpr auto operator[](range_difference_t<R> n) ->
            enable_if_t<random_access_range<R>, decltype(ranges::begin(derived())[n])>
        {
            return ranges::begin(derived())[n];
        }
        template<class R = const D>
        constexpr auto operator[](range_difference_t<R> n) const ->
            enable_if_t<random_access_range<R>, decltype(ranges::begin(derived())[n])>
        {
            return ranges::begin(derived())[n];
        }
    };

    struct view_base {};
    namespace Internal
    {
        template<class D>
        void derived_from_view_interface_template(view_interface<D>&);
        template<class T, class = void>
        inline constexpr bool is_derived_from_view_interface = false;
        template<class T>
        inline constexpr bool is_derived_from_view_interface<T,
            decltype(derived_from_view_interface_template(declval<T&>()))> = true;
    }
    template<class T>
    inline constexpr bool enable_view = derived_from<T, view_base> || Internal::is_derived_from_view_interface<T>;

    template<class T>
    /*concept*/ constexpr bool view = range<T> && movable<T> && enable_view<T>;

    template<class T>
    /*concept*/ constexpr bool viewable_range = range<T> &&
        ((view<remove_cvref_t<T>> && constructible_from<remove_cvref_t<T>, T>) ||
            (!view<remove_cvref_t<T>> &&
                (is_lvalue_reference_v<T> || (movable<remove_reference_t<T>> && !Internal::is_initializer_list<T>))));
}


namespace AZStd::ranges
{
#if __has_cpp_attribute(no_unique_address)
#define az_no_unique_address [[no_unique_address]]
#else
#define az_no_unique_address
#endif
    template<class I1, class I2>
    struct in_in_result
    {
        az_no_unique_address I1 in1;
        az_no_unique_address I2 in2;

        template<class II1, class II2, class = enable_if_t<convertible_to<const I1&, II1>&& convertible_to<const I2&, II2>> >
        constexpr operator in_in_result<II1, II2>() const&
        {
            return { in1, in2 };
        }

        template<class II1, class II2, class = enable_if_t<convertible_to<I1, II1>&& convertible_to<I2, II2>> >
        constexpr operator in_in_result<II1, II2>()&&
        {
            return { AZStd::move(in1), AZStd::move(in2) };
        }
    };

#undef az_no_unique_address

    template<class I1, class I2>
    using swap_ranges_result = in_in_result<I1, I2>;

    namespace Internal
    {
        struct swap_ranges_fn
        {
            template<class I1, class S1, class I2, class S2>
            constexpr auto operator()(I1 first1, S1 last1, I2 first2, S2 last2) const ->
                enable_if_t<input_iterator<I1>&& sentinel_for<S1, I1>
                && input_iterator<I2>&& sentinel_for<S2, I2>
                && indirectly_swappable<I1, I2>,
                swap_ranges_result<I1, I2>>
            {
                for (; !(first1 == last1 or first2 == last2); ++first1, ++first2)
                {
                    ranges::iter_swap(first1, first2);
                }
                return { AZStd::move(first1), AZStd::move(first2) };
            }

            template<class R1, class R2>
            constexpr auto operator()(R1&& r1, R2&& r2) const ->
                enable_if_t<input_range<R1>&& input_range<R2>
                && indirectly_swappable<iterator_t<R1>, iterator_t<R2>>,
                swap_ranges_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>>
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::swap_ranges_fn swap_ranges{};
    }
}

namespace AZStd::ranges::Internal
{
    // Implementation of ranges::swap customization point overload which calls ranges::swap_ranges
    // Must be done after the ranges::swap_ranges function has been declared
    // ranges::swap customization point https://eel.is/c++draft/concepts#concept.swappable-2.2
    template <class T, class U>
    constexpr auto swap_fn::operator()(T&& t, U&& u) const noexcept(noexcept((*this)(*t, *u)))
        ->enable_if_t<!is_class_or_enum_with_swap_adl<T, U>
        && is_array_v<T> && is_array_v<U> && (extent_v<T> == extent_v<U>)
        >
    {
        ranges::swap_ranges(t, u);
    }
}
