/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <algorithm>
#include <ranges>

#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/iterator/const_iterator.h>
#include <AzCore/std/ranges/subrange_fwd.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/remove_all_extents.h>

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
        template <class T>
        concept is_lvalue_or_borrowable =
            is_lvalue_reference_v<T>
            || enable_borrowed_range<remove_cv_t<T>>;

        //! begin
        template<class T>
        void begin(T&) = delete;
        template<class T>
        void begin(const T&) = delete;

        template <class T>
        concept has_member_begin =
            (!is_array_v<T>)
            && is_lvalue_or_borrowable<T>
            && requires(T& t) { requires input_or_output_iterator<decltype(t.begin())>; };

        template <class T>
        concept has_unqualified_begin =
            (!has_member_begin<T>)
            && AZStd::Internal::is_class_or_enum<T>
            && requires(T& t) { begin(t); };

        struct begin_fn
        {
            template<class T>
                requires is_array_v<T>
                    && requires { sizeof(remove_all_extents_t<T>); }
            constexpr auto operator()(T& t) const noexcept -> decltype(t + 0)
            {
                return t + 0;
            }

            template<class T>
                requires (!is_array_v<T>)
                    && is_lvalue_or_borrowable<T>
                    && has_member_begin<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).begin()))
            {
                return AZStd::forward<T>(t).begin();
            }

            template<class T>
                requires (!is_array_v<T>)
                    && is_lvalue_or_borrowable<T>
                    && has_unqualified_begin<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(begin(AZStd::forward<T>(t))))
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
        //! end
        template<class T>
        void end(T&) = delete;
        template<class T>
        void end(const T&) = delete;

        template <class T>
        concept has_member_end =
            requires(T& t) { t.end(); };

        template <class T>
        concept has_unqualified_end =
            (!has_member_end<T>)
            && AZStd::Internal::is_class_or_enum<T>
            && requires(T& t) { end(t); };

        struct end_fn
        {
            template<class T>
                requires is_array_v<T>
                    && (extent_v<T> != 0)
            constexpr auto operator()(T& t) const noexcept -> decltype(t + extent_v<T>)
            {
                return t + extent_v<T>;
            }

            template<class T>
                requires (!is_array_v<T>)
                    && is_lvalue_or_borrowable<T>
                    && has_member_end<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).end()))
            {
                return AZStd::forward<T>(t).end();
            }

            template<class T>
                requires (!is_array_v<T>)
                    && is_lvalue_or_borrowable<T>
                    && has_unqualified_end<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(end(AZStd::forward<T>(t))))
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
        //! rbegin
        template<class T>
        void rbegin(T&) = delete;
        template<class T>
        void rbegin(const T&) = delete;

        template <class T>
        concept has_member_rbegin =
            requires(T& t) { t.rbegin(); };

        template <class T>
        concept has_unqualified_rbegin =
            (!has_member_rbegin<T>)
            && ::AZStd::Internal::is_class_or_enum<T>
            && requires(T& t) { rbegin(t); };

        template <class T>
        concept has_bidirectional_rbegin =
            (!has_member_rbegin<T>)
            && (!has_unqualified_rbegin<T>)
            && requires(T& t) {
                requires same_as<decltype(ranges::begin(t)), decltype(ranges::end(t))>;
                requires bidirectional_iterator<decltype(ranges::begin(t))>;
                requires bidirectional_iterator<decltype(ranges::end(t))>;
            };

        struct rbegin_fn
        {
            template<is_lvalue_or_borrowable T>
                requires has_member_rbegin<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).rbegin()))
            {
                return AZStd::forward<T>(t).rbegin();
            }

            template<is_lvalue_or_borrowable T>
                requires has_unqualified_rbegin<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(rbegin(AZStd::forward<T>(t))))
            {
                return rbegin(AZStd::forward<T>(t));
            }

            template<has_bidirectional_rbegin T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::make_reverse_iterator(ranges::end(AZStd::forward<T>(t)))))
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
        template<class T>
        void rend(T&) = delete;
        template<class T>
        void rend(const T&) = delete;

        template <class T>
        concept has_member_rend =
            requires(T& t) { t.rend(); };

        template <class T>
        concept has_unqualified_rend =
            (!has_member_rend<T>)
            && ::AZStd::Internal::is_class_or_enum<T>
            && requires(T& t) { rend(t); };

        template <class T>
        concept has_bidirectional_rend =
            (!has_member_rend<T>)
            && (!has_unqualified_rend<T>)
            && requires(T& t) {
                requires same_as<decltype(ranges::begin(t)), decltype(ranges::end(t))>;
                requires bidirectional_iterator<decltype(ranges::begin(t))>;
                requires bidirectional_iterator<decltype(ranges::end(t))>;
            };

        struct rend_fn
        {
            template<is_lvalue_or_borrowable T>
                requires has_member_rend<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).rend()))
            {
                return AZStd::forward<T>(t).rend();
            }

            template<is_lvalue_or_borrowable T>
                requires has_unqualified_rend<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(rend(AZStd::forward<T>(t))))
            {
                return rend(AZStd::forward<T>(t));
            }

            template<has_bidirectional_rend T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::make_reverse_iterator(ranges::begin(AZStd::forward<T>(t)))))
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
        //! size
        template<class T>
        void size(T&) = delete;
        template<class T>
        void size(const T&) = delete;

        template <class T>
        concept has_member_size =
            requires(T& t) { t.size(); };

        template <class T>
        concept has_unqualified_size =
            (!has_member_size<T>)
            && ::AZStd::Internal::is_class_or_enum<T>
            && requires(T& t) { size(t); };

        template <class T>
        concept has_end_subtract_begin =
            (!has_member_size<T>)
            && (!has_unqualified_size<T>)
            && requires(T& t) { ranges::end(t) - ranges::begin(t); };

        template<class IntType>
        constexpr auto to_unsigned_like(IntType value)
        {
            using unsigned_t = make_unsigned_t<decltype(value)>;
            return static_cast<unsigned_t>(value);
        }

        struct size_fn
        {
            template<class T>
                requires is_array_v<T>
                    && (extent_v<T> != 0)
            constexpr auto operator()(T&) const noexcept -> decltype(extent_v<T>)
            {
                return extent_v<T>;
            }

            template<class T>
                requires (!disable_sized_range<remove_cv_t<T>>)
                    && has_member_size<T>
                    && ::AZStd::Internal::is_integer_like<decltype(declval<T&>().size())>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).size()))
                -> decltype(AZStd::forward<T>(t).size())
            {
                return AZStd::forward<T>(t).size();
            }

            template<class T>
                requires (!disable_sized_range<remove_cv_t<T>>)
                    && has_unqualified_size<T>
                    && ::AZStd::Internal::is_integer_like<decltype(size(declval<T&>()))>
            constexpr auto operator()(T&& t) const noexcept(noexcept(size(AZStd::forward<T>(t))))
                -> decltype(size(AZStd::forward<T>(t)))
            {
                return size(AZStd::forward<T>(t));
            }

            template<has_end_subtract_begin T>
                requires sized_sentinel_for<decltype(ranges::end(declval<T&>())), decltype(ranges::begin(declval<T&>()))>
                    && forward_iterator<decltype(ranges::begin(declval<T&>()))>
            constexpr auto operator()(T&& t) const
                noexcept(noexcept(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t))))
                -> AZStd::make_unsigned_t<decltype(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)))>
            {
                return to_unsigned_like(ranges::end(AZStd::forward<T>(t)) - ranges::begin(AZStd::forward<T>(t)));
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
                requires (sizeof(ptrdiff_t) > sizeof(make_signed_t<decltype(ranges::size(declval<T&>()))>))
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::size(AZStd::forward<T>(t)))) -> ptrdiff_t
            {
                return static_cast<ptrdiff_t>(ranges::size(AZStd::forward<T>(t)));
            }

            template<class T>
                requires (sizeof(ptrdiff_t) <= sizeof(make_signed_t<decltype(ranges::size(declval<T&>()))>))
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::size(AZStd::forward<T>(t))))
                -> make_signed_t<decltype(ranges::size(t))>
            {
                using ssize_type = make_signed_t<decltype(ranges::size(AZStd::forward<T>(t)))>;
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
        template <class T>
        concept has_member_empty =
            requires(T t) { requires convertible_to<decltype(t.empty()), bool>; };

        template <class T>
        concept has_size_compare_to_0 =
            !has_member_empty<T> &&
            requires(T& t) { requires convertible_to<decltype(ranges::size(t) == 0), bool>; };

        template <class T>
        concept has_begin_compare_to_end =
            (!has_member_empty<T>)
            && (!has_size_compare_to_0<T>)
            && requires(T& t) { requires convertible_to<decltype(ranges::begin(t) == ranges::end(t)), bool>; };

        struct empty_fn
        {
            template<is_lvalue_or_borrowable T>
                requires has_member_empty<T>
            [[nodiscard]] constexpr bool operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).empty()))
            {
                return AZStd::forward<T>(t).empty();
            }

            template<is_lvalue_or_borrowable T>
                requires has_size_compare_to_0<T>
            [[nodiscard]] constexpr bool operator()(T&& t) const noexcept(noexcept(ranges::size(AZStd::forward<T>(t)) == 0))
            {
                return ranges::size(AZStd::forward<T>(t)) == 0;
            }

            template<is_lvalue_or_borrowable T>
                requires has_begin_compare_to_end<T>
            [[nodiscard]] constexpr bool operator()(T&& t) const noexcept(noexcept(ranges::begin(AZStd::forward<T>(t)) == ranges::end(AZStd::forward<T>(t))))
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
        template <class T>
        concept has_member_data =
            requires(T t) { t.data(); };

        template <class T>
        concept has_qualified_ranges_begin =
            !has_member_data<T> &&
            requires(T& t) { requires contiguous_iterator<decltype(ranges::begin(t))>; };

        struct data_fn
        {
            template<is_lvalue_or_borrowable T>
                requires has_member_data<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::forward<T>(t).data()))
                -> decltype(AZStd::forward<T>(t).data())
            {
                return AZStd::forward<T>(t).data();
            }

            template<is_lvalue_or_borrowable T>
                requires has_qualified_ranges_begin<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(AZStd::to_address(ranges::begin(AZStd::forward<T>(t)))))
                -> decltype(AZStd::to_address(ranges::begin(AZStd::forward<T>(t))))
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
                requires is_lvalue_reference_v<T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::data(static_cast<const T&>(t))))
                -> decltype(ranges::data(static_cast<const T&>(t)))
            {
                return ranges::data(static_cast<const T&>(t));
            }

            template<class T>
                requires (!is_lvalue_reference_v<T>)
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::data(static_cast<const T&>(t))))
                -> decltype(ranges::data(static_cast<const T&&>(t)))
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
    using std::ranges::range;

    // sentinel type can now be defined after the range concept has been modeled
    template<range R>
    using sentinel_t = decltype(ranges::end(declval<R&>()));


    // const_iterator concept is now definable, with the range concept and iterator_t type alias available
    template<range R>
    using const_iterator_t = const_iterator<iterator_t<R>>;

    template<class T>
    concept borrowed_range =
        range<T>
        && (is_lvalue_reference_v<T> || enable_borrowed_range<remove_cvref_t<T>>);

    using std::ranges::dangling;

    template<range R>
    using borrowed_iterator_t = conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

    template<range R>
    using borrowed_subrange_t = conditional_t<borrowed_range<R>, subrange<iterator_t<R>>, dangling>;

    using std::ranges::sized_range;

    namespace Internal
    {
        template<class R, class T>
        concept output_range_impl =
            range<R>
            && requires(R& r) { requires output_iterator<decltype(ranges::begin(r)), T>; };
    }

    template<class R, class T>
    concept output_range = Internal::output_range_impl<R, T>;

    namespace Internal
    {
        template<class T>
        concept input_range_impl =
            range<T>
            && requires(T& t) { requires input_iterator<decltype(ranges::begin(t))>; };
    }

    template<class T>
    concept input_range = Internal::input_range_impl<T>;

    namespace Internal
    {
        template<class T>
        concept forward_range_impl =
            input_range<T>
            && requires(T& t) { requires forward_iterator<decltype(ranges::begin(t))>; };
    }

    template<class T>
    concept forward_range = Internal::forward_range_impl<T>;

    namespace Internal
    {
        template<class T>
        concept bidirectional_range_impl =
            forward_range<T>
            && requires(T& t) { requires bidirectional_iterator<decltype(ranges::begin(t))>; };
    }

    template<class T>
    concept bidirectional_range = Internal::bidirectional_range_impl<T>;

    namespace Internal
    {
        template<class T>
        concept random_access_range_impl =
            bidirectional_range<T>
            && requires(T& t) { requires random_access_iterator<decltype(ranges::begin(t))>; };
    }

    template<class T>
    concept random_access_range = Internal::random_access_range_impl<T>;

    template<sized_range R>
    using range_size_t = decltype(ranges::size(declval<R&>()));
    template<range R>
    using range_difference_t = iter_difference_t<iterator_t<R>>;
    template<range R>
    using range_value_t = iter_value_t<iterator_t<R>>;
    template<range R>
    using range_reference_t = iter_reference_t<iterator_t<R>>;
    template<range R>
    using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<R>>;


    namespace Internal
    {
        template<class T>
        concept contiguous_range_impl =
            random_access_range<T>
            && requires(T& t) {
                requires contiguous_iterator<decltype(ranges::begin(t))>;
                requires same_as<decltype(ranges::data(t)), add_pointer_t<decltype(*ranges::begin(t))>>;
            };
    }

    template<class T>
    concept contiguous_range = Internal::contiguous_range_impl<T>;

    template<class T>
    concept common_range =
        range<T>
        && same_as<iterator_t<T>, sentinel_t<T>>;

    template<class T>
    concept constant_range =
        input_range<T>
        && ::AZStd::Internal::constant_iterator<iterator_t<T>>;

    namespace Internal
    {
        template<input_range R>
        constexpr auto& possibly_const_range(R& r)
        {
            if constexpr (constant_range<const R> && !constant_range<R>)
            {
                return const_cast<const R&>(r);
            }
            else
            {
                return r;
            }
        }
    }
}

namespace AZStd::ranges
{
    // cbegin / cend can only be defined after possibly_const_range function is defined
    namespace Internal
    {
        //! cbegin
        struct cbegin_fn
        {
            template<is_lvalue_or_borrowable T>
            constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(ranges::begin(possibly_const_range(declval<T&>()))))
            {
                using iterator_type = decltype(ranges::begin(possibly_const_range(t)));
                return const_iterator<iterator_type>(ranges::begin(possibly_const_range(t)));
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
            template<is_lvalue_or_borrowable T>
            constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(ranges::end(possibly_const_range(declval<T&>()))))
            {
                using sentinel_type = decltype(ranges::end(possibly_const_range(t)));
                return const_sentinel<sentinel_type>(ranges::end(possibly_const_range(t)));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::cend_fn cend{};
    }

    namespace Internal
    {
        //! crbegin
        struct crbegin_fn
        {
            template<is_lvalue_or_borrowable T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rbegin(possibly_const_range(declval<T&>()))))
            {
                using iterator_type = decltype(ranges::rbegin(possibly_const_range(t)));
                return const_iterator<iterator_type>(ranges::rbegin(possibly_const_range(t)));
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
            template<is_lvalue_or_borrowable T>
            constexpr auto operator()(T&& t) const noexcept(noexcept(ranges::rend(possibly_const_range(declval<T&>()))))
            {
                using sentinel_type = decltype(ranges::rend(possibly_const_range(t)));
                return const_sentinel<sentinel_type>(ranges::rend(possibly_const_range(t)));
            }
        };
    }
    inline namespace customization_point_object
    {
        inline constexpr Internal::crend_fn crend{};
    }
}

namespace AZStd::ranges
{
    // iterator operations
    // ranges::advance
    namespace Internal
    {
        struct advance_fn
        {
            template<input_or_output_iterator I>
            constexpr void operator()(I& i, iter_difference_t<I> n) const
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

            template<input_or_output_iterator I, sentinel_for<I> S>
            constexpr void operator()(I& i, S bound) const
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

            template<input_or_output_iterator I, sentinel_for<I> S>
            constexpr iter_difference_t<I> operator()(I& i, iter_difference_t<I> n, S bound) const
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
            template<input_or_output_iterator I, sentinel_for<I> S>
                requires (!sized_sentinel_for<S, I>)
            constexpr iter_difference_t<I> operator()(I first, S last) const
            {
                // Since S is not a sized sentinel, can only increment from first to last
                iter_difference_t<I> result{};
                for (; first != last; ++first, ++result) {}

                return result;
            }

            template<input_or_output_iterator I, sentinel_for<I> S>
                requires sized_sentinel_for<S, I>
            constexpr iter_difference_t<I> operator()(const I& first, const S& last) const
            {
                return last - first;
            }

            template<range R>
            constexpr range_difference_t<R> operator()(R&& r) const
            {
                if constexpr (sized_range<R>)
                {
                    return ranges::size(r);
                }
                else
                {
                    return operator()(ranges::begin(r), ranges::end(r));
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
            template<input_or_output_iterator I>
            constexpr I operator()(I x) const
            {
                ++x;
                return x;
            }

            template<input_or_output_iterator I>
            constexpr I operator()(I x, iter_difference_t<I> n) const
            {
                ranges::advance(x, n);
                return x;
            }

            template<input_or_output_iterator I, sentinel_for<I> S>
            constexpr I operator()(I x, S bound) const
            {
                ranges::advance(x, bound);
                return x;
            }

            template<input_or_output_iterator I, sentinel_for<I> S>
            constexpr I operator()(I x, iter_difference_t<I> n, S bound) const
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
            template<bidirectional_iterator I>
            constexpr I operator()(I x) const
            {
                --x;
                return x;
            }

            template<bidirectional_iterator I>
            constexpr I operator()(I x, iter_difference_t<I> n) const
            {
                ranges::advance(x, -n);
                return x;
            }

            template<input_or_output_iterator I, sentinel_for<I> S>
            constexpr I operator()(I x, iter_difference_t<I> n, S bound) const
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

    struct view_base {};
    namespace Internal
    {
        template<class D>
        void derived_from_view_interface_template(view_interface<D>&);
        template<class T, class = void>
        inline constexpr bool is_derived_from_view_interface = false;
        template<class T>
        inline constexpr bool is_derived_from_view_interface<T,
            decltype(derived_from_view_interface_template(declval<remove_cvref_t<T>&>()))> = true;
    }
    template<class T>
    inline constexpr bool enable_view = derived_from<T, view_base> || Internal::is_derived_from_view_interface<T>;

    template<class T>
    concept view =
        range<T>
        && movable<T>
        && enable_view<T>;

    template<class T>
    concept viewable_range =
        range<T>
        && ((view<remove_cvref_t<T>> && constructible_from<remove_cvref_t<T>, T>) ||
         (!view<remove_cvref_t<T>> &&
          (is_lvalue_reference_v<T> ||
           (movable<remove_reference_t<T>> && !Internal::is_initializer_list<T>))));

    template<class D>
        requires is_class_v<D>
            && same_as<D, remove_cv_t<D>>
    class view_interface<D>
    {
    private:
        constexpr D& derived() noexcept
        {
            // Make sure that the derived type is complete
            static_assert(sizeof(D) > 0 && derived_from<D, view_interface> && view<D>);
            return static_cast<D&>(*this);
        }
        constexpr const D& derived() const noexcept
        {
            static_assert(sizeof(D) > 0 && derived_from<D, view_interface> && view<D>);
            return static_cast<const D&>(*this);
        }

    public:
        template <forward_range Derived = D>
        constexpr bool empty()
        {
            return ranges::begin(derived()) == ranges::end(derived());
        }
        template <class Derived = D>
            requires forward_range<const Derived>
        constexpr bool empty() const
        {
            return ranges::begin(derived()) == ranges::end(derived());
        }

        template <class Derived = D>
        constexpr explicit operator bool() const noexcept(noexcept(ranges::empty(static_cast<const Derived&>(*this))))
        {
            return !ranges::empty(derived());
        }

        template <class Derived = D>
            requires contiguous_iterator<iterator_t<Derived>>
        constexpr auto data()
            -> decltype(AZStd::to_address(ranges::begin(static_cast<Derived&>(*this))))
        {
            return to_address(ranges::begin(derived()));
        }
        template <class Derived = D>
            requires range<const Derived>
                && contiguous_iterator<iterator_t<const Derived>>
        constexpr auto data() const
            -> decltype(AZStd::to_address(ranges::begin(static_cast<const Derived&>(*this))))
        {
            return to_address(ranges::begin(derived()));
        }

        template <forward_range Derived = D>
            requires sized_sentinel_for<sentinel_t<Derived>, iterator_t<Derived>>
        constexpr auto size()
            -> decltype(ranges::end(static_cast<Derived&>(*this)) - ranges::begin(static_cast<Derived&>(*this)))
        {
            return ranges::end(derived()) - ranges::begin(derived());
        }
        template <class Derived = D>
            requires forward_range<const Derived>
                && sized_sentinel_for<sentinel_t<const Derived>, iterator_t<const Derived>>
        constexpr auto size() const
            -> decltype(ranges::end(static_cast<const Derived&>(*this)) - ranges::begin(static_cast<const Derived&>(*this)))
        {
            return ranges::end(derived()) - ranges::begin(derived());
        }

        template <forward_range Derived = D>
        constexpr auto front()
            -> decltype(*ranges::begin(static_cast<Derived&>(*this)))
        {
            return *ranges::begin(derived());
        }
        template <class Derived = D>
            requires forward_range<const Derived>
        constexpr auto front() const
            -> decltype(*ranges::begin(static_cast<const Derived&>(*this)))
        {
            return *ranges::begin(derived());
        }

        template <bidirectional_range Derived = D>
            requires common_range<Derived>
        constexpr auto back()
            -> decltype(*ranges::prev(ranges::end(static_cast<Derived&>(*this))))
        {
            return *ranges::prev(ranges::end(derived()));
        }

        template <class Derived = D>
            requires bidirectional_range<const Derived>
                && common_range<const Derived>
        constexpr auto back() const
            -> decltype(*ranges::prev(ranges::end(static_cast<const Derived&>(*this))))
        {
            return *ranges::prev(ranges::end(derived()));
        }

        template<random_access_range R = D>
        constexpr auto operator[](range_difference_t<R> n)
            -> decltype(ranges::begin(static_cast<R&>(*this))[n])
        {
            return ranges::begin(derived())[n];
        }
        template<random_access_range R = const D>
        constexpr auto operator[](range_difference_t<R> n) const
            -> decltype(ranges::begin(static_cast<R&>(*this))[n])
        {
            return ranges::begin(derived())[n];
        }
    };

    // Helper Concepts - https://eel.is/c++draft/ranges#range.utility.helpers
    namespace Internal
    {
        // Helper concepts that are used by range adaptor classes
        template<bool Const, class T>
        using maybe_const = conditional_t<Const, const T, T>;

        template<class R>
        concept simple_view =
            view<R>
            && range<const R>
            && requires(R& r, const R& cr) {
                requires same_as<decltype(ranges::begin(r)), decltype(ranges::begin(cr))>;
                requires same_as<decltype(ranges::end(r)), decltype(ranges::end(cr))>;
            };

        template<class I>
        concept has_arrow =
            input_iterator<I>
            && (is_pointer_v<I> || requires(I i) { i.operator->(); });

        template<class T, class U>
        concept different_from = ::AZStd::Internal::different_from<T, U>;
    }
}

namespace AZStd::ranges
{
    using std::ranges::in_in_result;
    using std::ranges::swap_ranges_result;

    namespace Internal
    {
        struct swap_ranges_fn
        {
            template<input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2>
                requires indirectly_swappable<I1, I2>
            constexpr swap_ranges_result<I1, I2> operator()(I1 first1, S1 last1, I2 first2, S2 last2) const
            {
                for (; !(first1 == last1 or first2 == last2); ++first1, ++first2)
                {
                    ranges::iter_swap(first1, first2);
                }
                return { AZStd::move(first1), AZStd::move(first2) };
            }

            template<input_range R1, input_range R2>
                requires indirectly_swappable<iterator_t<R1>, iterator_t<R2>>
            constexpr swap_ranges_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>
            operator()(R1&& r1, R2&& r2) const
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
    constexpr void swap_fn::operator()(T&& t, U&& u) const noexcept(noexcept((*this)(*t, *u)))
        requires (!is_class_or_enum_with_swap_adl<T, U>)
            && is_array_v<T>
            && is_array_v<U>
            && (extent_v<T> == extent_v<U>)
    {
        ranges::swap_ranges(t, u);
    }
}

// Opening AZStd::ranges::views namespace to provide access to it in AZStd
namespace AZStd::ranges::views{}

namespace AZStd
{
      namespace views = ranges::views;

      //! Adding C++23 from_range_t tag type
      //! https://eel.is/c++draft/range.utility.conv
      struct from_range_t {};
      inline constexpr from_range_t from_range;
}
