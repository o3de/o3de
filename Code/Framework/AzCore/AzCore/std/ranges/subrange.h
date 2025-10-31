/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/subrange_fwd.h>

#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/tuple.h>

// Specializing tuple in std:: namespace since tuple_size and tuple_element structs
// are alias templates inside of the AZStd:: namespace
namespace std
{
    AZ_PUSH_DISABLE_WARNING(, "-Wmismatched-tags")
    template<class I, class S, AZStd::ranges::subrange_kind K>
    struct tuple_size<AZStd::ranges::subrange<I, S, K>>
        : integral_constant<size_t, 2> {};
    template<class I, class S, AZStd::ranges::subrange_kind K>
    struct tuple_element<0, AZStd::ranges::subrange<I, S, K>>
    {
        using type = I;
    };
    template<class I, class S, AZStd::ranges::subrange_kind K>
    struct tuple_element<1, AZStd::ranges::subrange<I, S, K>>
    {
        using type = S;
    };
    template<class I, class S, AZStd::ranges::subrange_kind K>
    struct tuple_element<0, const AZStd::ranges::subrange<I, S, K>>
    {
        using type = I;
    };
    template<class I, class S, AZStd::ranges::subrange_kind K>
    struct tuple_element<1, const AZStd::ranges::subrange<I, S, K>>
    {
        using type = S;
    };
    AZ_POP_DISABLE_WARNING
}

namespace AZStd::ranges
{
    namespace Internal
    {
        template <typename From, typename To>
        /*concept*/ constexpr bool uses_nonqualification_pointer_conversion =
            is_pointer_v<From> && is_pointer_v<To> &&
            !convertible_to<remove_pointer_t<From>(*)[], remove_pointer_t<To>(*)[]>;

        template<class From, class To>
        /*concept*/ constexpr bool convertible_to_non_slicing =
            convertible_to<From, To> &&
            !uses_nonqualification_pointer_conversion<decay_t<From>, decay_t<To>>;

        template<class T, class = void>
        /*concept*/ constexpr bool pair_like = false;

        template<class T>
        /*concept*/ constexpr bool pair_like<T, enable_if_t<conjunction_v<
            bool_constant<!is_reference_v<T>>,
            sfinae_trigger<typename tuple_size<T>::type>,
            bool_constant<derived_from<tuple_size<T>, integral_constant<size_t, 2>>>,
            sfinae_trigger<
            tuple_element_t<0, remove_const_t<T>>,
            tuple_element_t<1, remove_const_t<T>>>,
            bool_constant<convertible_to<decltype(AZStd::get<0>(declval<T>())), const tuple_element_t<0, T>&>>,
            bool_constant<convertible_to<decltype(AZStd::get<1>(declval<T>())), const tuple_element_t<1, T>&>>>
            >> = true;

        template<class T, class U, class V, class = void>
        /*concept*/ constexpr bool pair_like_convertible_from = false;
        template<class T, class U, class V>
        /*concept*/ constexpr bool pair_like_convertible_from<T, U, V, enable_if_t<
            !range<T> && pair_like<T> && constructible_from<T, U, V>>> =
            convertible_to_non_slicing<U, tuple_element_t<0, T>> &&
            convertible_to<V, tuple_element_t<1, T>>;
    }

    template<class I, class S, subrange_kind K>
    class subrange<I, S, K, enable_if_t<conjunction_v<
        bool_constant<input_or_output_iterator<I>>,
        bool_constant<sentinel_for<S, I>>,
        bool_constant<(K == subrange_kind::sized || !sized_sentinel_for<S, I>)>>
        >> : public view_interface<subrange<I, S, K>>
    {
        static constexpr bool StoreSize = K == subrange_kind::sized && !sized_sentinel_for<S, I>;

    public:
        template<class I2 = I, class = enable_if_t<default_initializable<I2>>>
        constexpr subrange() {}

        template<class I2, class = enable_if_t<Internal::convertible_to_non_slicing<I2, I> && !StoreSize>>
        constexpr subrange(I2 i, S s)
            : m_begin(AZStd::move(i))
            , m_end(s)
        {
        }
        template<class I2, class = enable_if_t<Internal::convertible_to_non_slicing<I2, I> && K == subrange_kind::sized>>
        constexpr subrange(I2 i, S s, make_unsigned_t<iter_difference_t<I>> n)
            : m_begin(AZStd::move(i))
            , m_end(s)
        {
            if constexpr (StoreSize)
            {
                m_size = n;
            }
        }

        template<class R, class = enable_if_t<conjunction_v<
            bool_constant<Internal::different_from<R, subrange>>,
            bool_constant<borrowed_range<R>>,
            bool_constant<Internal::convertible_to_non_slicing<iterator_t<R>, I>>,
            bool_constant<convertible_to<sentinel_t<R>, S>>,
            bool_constant<(!StoreSize || sized_range<R>)>>
            >>
        constexpr subrange(R&& r)
            : m_begin{ ranges::begin(AZStd::forward<R>(r)) }
            , m_end{ ranges::end(AZStd::forward<R>(r)) }
        {
            if constexpr (StoreSize)
            {
                m_size = ranges::size(AZStd::forward<R>(r));
            }
        }

        template<class R, class = enable_if_t<conjunction_v<
            bool_constant<borrowed_range<R>>,
            bool_constant<Internal::convertible_to_non_slicing<iterator_t<R>, I>>,
            bool_constant<convertible_to<sentinel_t<R>, S>>,
            bool_constant<(K == subrange_kind::sized)>>
            >>
        constexpr subrange(R&& r, make_unsigned_t<iter_difference_t<I>> n)
            : subrange{ ranges::begin(r), ranges::end(r), n }
        {
        }

        template<class PairLike, class = enable_if_t<conjunction_v<
            bool_constant<Internal::different_from<PairLike, subrange>>,
            bool_constant<Internal::pair_like_convertible_from<PairLike, const I&, const S&>>>
            >>
        constexpr operator PairLike() const
        {
            return PairLike{ m_begin, m_end };
        }

        template<bool Enable = copyable<I>, class = enable_if_t<Enable>>
        constexpr I begin() const
        {
            return m_begin;
        }
        template<bool Enable = !copyable<I>, class = enable_if_t<Enable>>
        [[nodiscard]] constexpr I begin()
        {
            return AZStd::move(m_begin);
        }
        constexpr S end() const
        {
            return m_end;
        }

        constexpr bool empty() const
        {
            return m_begin == m_end;
        }
        template<bool Enable = (K == subrange_kind::sized), class = enable_if_t<Enable>>
        constexpr make_unsigned_t<iter_difference_t<I>> size() const
        {
            if constexpr (StoreSize)
            {
                return m_size;
            }
            else
            {
                using unsigned_difference_type = make_unsigned_t<iter_difference_t<I>>;
                return static_cast<unsigned_difference_type>(m_end - m_begin);
            }
        }

        template<bool Enable = forward_iterator<I>, class = enable_if_t<Enable>>
        [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1) const&
        {
            auto tmp = *this;
            tmp.advance(n);
            return tmp;
        }
        [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1)&&
        {
            advance(n);
            return AZStd::move(*this);
        }
        template<bool Enable = bidirectional_iterator<I>, class = enable_if_t<Enable>>
        [[nodiscard]] constexpr subrange prev(iter_difference_t<I> n = 1) const
        {
            auto tmp = *this;
            tmp.advance(-n);
            return tmp;
        }
        constexpr subrange& advance(iter_difference_t<I> n)
        {
            using unsigned_difference_type = AZStd::make_unsigned_t<iter_difference_t<I>>;
            if constexpr (bidirectional_iterator<I>)
            {
                if (n < 0)
                {
                    ranges::advance(m_begin, n);
                    if constexpr (StoreSize)
                    {
                        m_size += static_cast<unsigned_difference_type>(-n);
                    }
                    return *this;
                }
            }

            if constexpr (StoreSize)
            {
                auto actualAdvanceDist = n - ranges::advance(m_begin, n, m_end);
                m_size -= static_cast<unsigned_difference_type>(actualAdvanceDist);
            }
            else
            {
                ranges::advance(m_begin, n, m_end);
            }

            return *this;
        }

    private:
        I m_begin{};
        S m_end{};

        // Size member will not count against the size of the class when using no_unique_address
        struct SizeNotStored
        {
            constexpr SizeNotStored() noexcept = default;
        };
        using size_type = AZStd::conditional_t<StoreSize, make_unsigned_t<iter_difference_t<I>>, SizeNotStored>;
        AZ_NO_UNIQUE_ADDRESS size_type m_size{};
    };

    template<class I, class S, class = enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>>>
    subrange(I, S) -> subrange<I, S>;

    template<class I, class S, class = enable_if_t<input_or_output_iterator<I>&& sentinel_for<S, I>>>
    subrange(I, S, make_unsigned_t<iter_difference_t<I>>) -> subrange<I, S, subrange_kind::sized>;

    template<class R, class = enable_if_t<borrowed_range<R>> >
    subrange(R&&) -> subrange<iterator_t<R>, sentinel_t<R>,
        (sized_range<R> || sized_sentinel_for<sentinel_t<R>, iterator_t<R>>)
        ? subrange_kind::sized : subrange_kind::unsized>;

    template<class R, class = enable_if_t<borrowed_range<R>> >
    subrange(R&&, make_unsigned_t<range_difference_t<R>>) ->
        subrange<iterator_t<R>, sentinel_t<R>, subrange_kind::sized>;

    template<size_t N, class I, class S, subrange_kind K, class = enable_if_t<((N == 0 && copyable<I>) || N == 1)>>
    constexpr auto get(const subrange<I, S, K>& r)
    {
        if constexpr (N == 0)
        {
            return r.begin();
        }
        else
        {
            return r.end();
        }
    }

    template<size_t N, class I, class S, subrange_kind K, class = enable_if_t<(N < 2)>>
    constexpr auto get(subrange<I, S, K>&& r)
    {
        if constexpr (N == 0)
        {
            return r.begin();
        }
        else
        {
            return r.end();
        }
    }


    template<class I, class S, subrange_kind K>
    inline constexpr bool enable_borrowed_range<subrange<I, S, K>> = true;

}

namespace AZStd
{
    using ranges::get;
}
