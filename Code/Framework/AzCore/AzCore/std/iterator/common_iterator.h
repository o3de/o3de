/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/iterator/iterator_primitives.h>

namespace AZStd
{
    // Bring in std utility functions into AZStd namespace
    using std::forward;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool has_operator_arrow = false;
    template<class I>
    inline constexpr bool has_operator_arrow<I, enable_if_t<
        sfinae_trigger_v<decltype(declval<I>().operator->())>
        >> = true;

    template <class T, class = void>
    /*concept*/ constexpr bool can_reference_post_increment = false;
    template <class T>
    constexpr bool can_reference_post_increment<T, enable_if_t<can_reference<decltype(*declval<T>()++)>>> = true;
}

namespace AZStd
{
    template<class I, class S, class = void>
    class common_iterator;

    template<class I, class S>
    class common_iterator<I, S, enable_if_t<conjunction_v<
        bool_constant<input_or_output_iterator<I>>,
        bool_constant<sentinel_for<S, I>>,
        bool_constant<!same_as<I, S>>,
        bool_constant<copyable<I>>
    >
    >>
    {
    public:
        template<bool Enable = default_initializable<I>, class = enable_if<Enable>>
        constexpr common_iterator() {}
        constexpr common_iterator(I i)
            : m_iterSentinel{ in_place_type<I>, AZStd::move(i) }
        {
        }
        constexpr common_iterator(S s)
            : m_iterSentinel{ in_place_type<S>, AZStd::move(s) }
        {
        }
        template<class I2, class S2, class = enable_if_t<conjunction_v<
            bool_constant<convertible_to<const I2&, I>>,
            bool_constant<convertible_to<const S2&, S>>
        >
        >>
        constexpr common_iterator(const common_iterator<I2, S2>& other)
            : m_iterSentinel {
            [&]()
            {
            #if __cpp_constexpr_dynamic_alloc >= 201907L
                AZ_Assert(!other.m_iterSentinel.valueless_by_exception(), "common_iterator variant must have an alternative");
            #endif
                if (other.m_iterSentinel.index() == 0)
                {
                    // Initialize from the iterator alternative of the other common iterator
                    return variant<I, S>{ in_place_index<0>, get<0>(other.m_iterSentinel) };
                }
                else
                {
                    // Initialize from the sentinel alternative of the other common iterator
                    return variant<I, S>{ in_place_index<1>, get<1>(other.m_iterSentinel) };
                }
            }() }
        {
        }

        template<class I2, class S2, class = enable_if_t<conjunction_v<
            bool_constant<convertible_to<const I2&, I>>,
            bool_constant<convertible_to<const S2&, S>>,
            bool_constant<assignable_from<I&, const I2&>>,
            bool_constant<assignable_from<S&, const S2&>>
        >
        >>
        constexpr common_iterator& operator=(const common_iterator<I2, S2>& other)
        {
            AZ_Assert(!other.m_iterSentinel.valueless_by_exception(), "common_iterator variant must have an alternative");
            if (m_iterSentinel.index() == other.m_iterSentinel.index())
            {
                if (other.m_iterSentinel.index() == 0)
                {
                    get<0>(m_iterSentinel) = get<0>(other.m_iterSentinel);
                }
                else
                {
                    get<0>(m_iterSentinel) = get<0>(other.m_iterSentinel);
                }
            }
            else
            {
                if (other.m_iterSentinel.index() == 0)
                {
                    m_iterSentinel.template emplace<0>(get<0>(other.m_iterSentinel));
                }
                else
                {
                    m_iterSentinel.template emplace<1>(get<0>(other.m_iterSentinel));
                }
            }

            return *this;
        }

        constexpr decltype(auto) operator*()
        {
            AZ_Assert(holds_alternative<I>(m_iterSentinel), "Attempting to deference the sentinel value");
            return *get<0>(m_iterSentinel);
        }
        template<bool Enable = Internal::dereferenceable<const I>, class = enable_if_t<Enable>>
        constexpr decltype(auto) operator*() const
        {
            AZ_Assert(holds_alternative<I>(m_iterSentinel), "Attempting to deference the sentinel value");
            return *get<0>(m_iterSentinel);
        }
        template<bool Enable = conjunction_v<
            bool_constant<indirectly_readable<const I>>,
            disjunction<
                bool_constant<Internal::has_operator_arrow<const I>>,
                is_reference<iter_reference_t<I>>,
                bool_constant<constructible_from<iter_value_t<I>, iter_reference_t<I>>>
                >
            >, class = enable_if_t<Enable>>
        constexpr decltype(auto) operator->() const
        {
            AZ_Assert(holds_alternative<I>(m_iterSentinel), "arrow operator cannot invoked on sentinel value");
            if constexpr (AZStd::is_pointer_v<I> || Internal::has_operator_arrow<const I>)
            {
                return get<I>(m_iterSentinel);
            }
            else if constexpr (is_reference_v<iter_reference_t<I>>)
            {
                auto&& tmp = *get<I>(m_iterSentinel);
                return AZStd::addressof(tmp);
            }
            else
            {
                class proxy
                {
                    iter_value_t<I> m_keep;
                public:
                    constexpr proxy(iter_reference_t<I>&& wrappedIter)
                        : m_keep(AZStd::move(wrappedIter)) {}
                    constexpr const iter_value_t<I>* operator->() const noexcept
                    {
                        return addressof(m_keep);
                    }
                };

                return proxy(*get<I>(m_iterSentinel));
            }
        }


        constexpr common_iterator& operator++()
        {
            AZ_Assert(holds_alternative<I>(m_iterSentinel), "pre-increment cannot be invoked on the sentinel value");
            ++get<I>(m_iterSentinel);
            return *this;
        }
        constexpr decltype(auto) operator++(int)
        {
            AZ_Assert(holds_alternative<I>(m_iterSentinel), "pre-increment cannot be invoked on the sentinel value");
            if constexpr (forward_iterator<I>)
            {
                auto oldThis = *this;
                ++get<I>(m_iterSentinel);
                return oldThis;
            }
            else if constexpr (Internal::can_reference_post_increment<I>
                || !(indirectly_readable<I> && constructible_from<iter_value_t<I>, iter_reference_t<I>> && move_constructible<iter_value_t<I>>))
            {
                return get<I>(m_iterSentinel)++;
            }
            else
            {
                class postfix_proxy
                {
                    iter_value_t<I> m_keep;
                public:
                    constexpr postfix_proxy(iter_reference_t<I>&& wrappedIter)
                        : m_keep(AZStd::forward<iter_reference_t<I>>(wrappedIter)) {}
                    constexpr const iter_value_t<I>& operator*() const noexcept
                    {
                        return m_keep;
                    }
                };

                postfix_proxy p(**this);
                ++* this;
                return p;
            }
        }

        template<class I2, class S2>
        friend constexpr auto operator==(const common_iterator& x, const common_iterator<I2, S2>& y)
            -> enable_if_t<conjunction_v<
            bool_constant<sentinel_for<S2, I>>,
            bool_constant<sentinel_for<S, I2>>,
            bool_constant<!equality_comparable_with<I, I2>>
        >, bool>
        {
        #if __cpp_constexpr_dynamic_alloc >= 201907L
            AZ_Assert(!x.m_iterSentinel.valueless_by_exception() && !y.m_iterSentinel.valueless_by_exception(),
                "common_iterator variant must have an alternative");
        #endif
            if (x.m_iterSentinel.index() == y.m_iterSentinel.index())
            {
                return true;
            }

            if (x.m_iterSentinel.index() == 0)
            {
                return get<0>(x.m_iterSentinel) == get<1>(y.m_iterSentinel);
            }
            else
            {
                return get<1>(x.m_iterSentinel) == get<0>(y.m_iterSentinel);
            }
        }
        template<class I2, class S2>
        friend constexpr auto operator!=(const common_iterator& x, const common_iterator<I2, S2>& y)
            -> enable_if_t<conjunction_v<
            bool_constant<sentinel_for<S2, I>>,
            bool_constant<sentinel_for<S, I2>>,
            bool_constant<!equality_comparable_with<I, I2>>
        >, bool>
        {
            return !operator==(x, y);
        }

        template<class I2, class S2>
        friend constexpr auto operator==(const common_iterator& x, const common_iterator<I2, S2>& y)
            -> enable_if_t<conjunction_v<
            bool_constant<sentinel_for<S2, I>>,
            bool_constant<sentinel_for<S, I2>>,
            bool_constant<equality_comparable_with<I, I2>>
        >, bool>
        {
        #if __cpp_constexpr_dynamic_alloc >= 201907L
            AZ_Assert(!x.m_iterSentinel.valueless_by_exception() && !y.m_iterSentinel.valueless_by_exception(),
                "common_iterator variant must have an alternative");
        #endif

            if (x.m_iterSentinel.index() == y.m_iterSentinel.index())
            {
                if (x.m_iterSentinel.index() == 1)
                {
                    // sentinel values always compare equal to each other
                    return true;
                }
                else
                {
                    return get<0>(x.m_iterSentinel) == get<0>(y.m_iterSentinel);
                }
            }

            if (x.m_iterSentinel.index() == 0)
            {
                return get<0>(x.m_iterSentinel) == get<1>(y.m_iterSentinel);
            }
            else
            {
                return get<1>(x.m_iterSentinel) == get<0>(y.m_iterSentinel);
            }
        }
        template<class I2, class S2>
        friend constexpr auto operator!=(const common_iterator& x, const common_iterator<I2, S2>& y)
            -> enable_if_t<conjunction_v<
            bool_constant<sentinel_for<S2, I>>,
            bool_constant<sentinel_for<S, I2>>,
            bool_constant<equality_comparable_with<I, I2>>
        >, bool>
        {
            return !operator==(x, y);
        }

        template<class I2, class S2>
        friend constexpr auto operator-(const common_iterator& x, const common_iterator<I2, S2>& y)
            -> enable_if_t<conjunction_v<
            bool_constant<sized_sentinel_for<I2, I>>,
            bool_constant<sized_sentinel_for<S2, I>>,
            bool_constant<sized_sentinel_for<S, I2>>
        >, iter_difference_t<I2>>
        {
        #if __cpp_constexpr_dynamic_alloc >= 201907L
            AZ_Assert(!x.m_iterSentinel.valueless_by_exception() && !y.m_iterSentinel.valueless_by_exception(),
                "common_iterator variant must have an alternative");
        #endif
            if (x.m_iterSentinel.index() == y.m_iterSentinel.index())
            {
                if (x.m_iterSentinel.index() == 1)
                {
                    // sentinel values do not have a numerical difference
                    return 0;
                }
                else
                {
                    return get<0>(x.m_iterSentinel) - get<0>(y.m_iterSentinel);
                }
            }

            if (x.m_iterSentinel.index() == 0)
            {
                return get<0>(x.m_iterSentinel) - get<1>(y.m_iterSentinel);
            }
            else
            {
                return get<1>(x.m_iterSentinel) - get<0>(y.m_iterSentinel);
            }
        }

        friend constexpr auto iter_move(const common_iterator& i)
            noexcept(noexcept(ranges::iter_move(declval<const I&>())))
            -> enable_if_t<input_iterator<I>, iter_rvalue_reference_t<I>>
        {
            AZ_Assert(holds_alternative<I>(i.m_iterSentinel), "iter_move cannot be invoked on sentinel value");
            return ranges::iter_move(get<I>(i.m_iterSentinel));
        }

        template<class I2, class S2, enable_if_t<indirectly_swappable<I2, I>>>
        friend constexpr void iter_swap(const common_iterator& x, const common_iterator<I2, S2>& y)
            noexcept(noexcept(ranges::iter_swap(declval<const I&>(), declval<const I2&>())))
        {
            AZ_Assert(holds_alternative<I>(x.m_iterSentinel) && holds_alternative<I>(y.m_iterSentinel),
                "iter_swap requires both common_iterators alternatives be set to a valid deferenceable iterator");
            return ranges::iter_swap(get<I>(x.m_iterSentinel), get<I>(y.m_iterSentinel));
        }

    private:
        //! Because AZStd::variant under the hood invokes AZStd::construct_at, which uses placement new
        //! this makes the emplace calls in this class non-constexpr
        //! fallback to use a struct in C++17 with a bool, since it is simple to deal with than a union
#if __cpp_constexpr_dynamic_alloc >= 201907L
        variant<I, S> m_iterSentinel{};
#else
        template<class Iter, class Sen>
        struct IterVariant
        {
            constexpr IterVariant() = default;

            template<size_t Index, class... Args>
            constexpr IterVariant(in_place_index_t<Index>, Args&&... args)
            {
                if constexpr (Index == 0 || Index == 1)
                {
                    emplace<Index>(AZStd::forward<Args>(args)...);
                }

                static_assert(Index < 2, "IterVariant can only be constructed with Index < 2");
            }
            template<class T, class... Args>
            constexpr IterVariant(in_place_type_t<T>, Args&&... args)
            {
                if constexpr (AZStd::same_as<T, Iter>)
                {
                    emplace<0>(AZStd::forward<Args>(args)...);
                }
                else if constexpr (AZStd::same_as<T, Sen>)
                {
                    emplace<1>(AZStd::forward<Args>(args)...);
                }

                static_assert(AZStd::same_as<T, Iter> || AZStd::same_as<T, Sen>,
                    "type must match either Iter or Sen");
            }

            constexpr bool operator==(const IterVariant& other) const
            {
                return m_elementIndex != other.m_elementIndex ? false
                    : m_elementIndex == 0
                    ? m_iter == other.m_iter
                    : m_sentinel == other.m_sentinel;
            }
            constexpr bool operator!=(const IterVariant& other) const
            {
                return !operator==(other);
            }

            template<size_t Index, class... Args>
            constexpr auto& emplace(Args&&... args)
            {
                m_elementIndex = Index;
                if constexpr (Index == 0)
                {
                    m_iter = Iter(AZStd::forward<Args>(args)...);
                    return m_iter;
                }
                else if constexpr (Index == 1)
                {
                    m_sentinel = Sen(AZStd::forward<Args>(args)...);
                    return m_sentinel;
                }

                static_assert(Index < 2, "Attempting to emplace an Index out of range of the common_iterator variant");
            }

            constexpr size_t index() const
            {
                return m_elementIndex;
            }

            template<size_t Index>
            constexpr auto& get() &
            {
                AZ_Assert(Index == index(), "Attempting to retrieve an inactive alternative");
                if constexpr (Index == 0)
                {
                    return m_iter;
                }
                else
                {
                    return m_sentinel;
                }
            }
            template<size_t Index>
            constexpr const auto& get() const &
            {
                AZ_Assert(Index == index(), "Attempting to retrieve an inactive alternative");
                if constexpr (Index == 0)
                {
                    return m_iter;
                }
                else
                {
                    return m_sentinel;
                }
            }
            template<size_t Index>
            constexpr auto&& get() &&
            {
                AZ_Assert(Index == index(), "Attempting to retrieve an inactive alternative");
                if constexpr (Index == 0)
                {
                    return AZStd::move(m_iter);
                }
                else
                {
                    return AZStd::move(m_sentinel);
                }
            }
            template<size_t Index>
            constexpr const auto&& get() const &&
            {
                AZ_Assert(Index == index(), "Attempting to retrieve an inactive alternative");
                if constexpr (Index == 0)
                {
                    return AZStd::move(m_iter);
                }
                else
                {
                    return AZStd::move(m_sentinel);
                }
            }

            template<class T>
            constexpr T& get() &
            {
                if constexpr (AZStd::same_as<T, Iter>)
                {
                    AZ_Assert(m_elementIndex == 0, "Attempting to retrieve an inactive alternative");
                    return m_iter;
                }
                else if constexpr (AZStd::same_as<T, Sen>)
                {
                    AZ_Assert(m_elementIndex == 1, "Attempting to retrieve an inactive alternative");
                    return m_sentinel;
                }

                static_assert(AZStd::same_as<T, Iter> || AZStd::same_as<T, Sen>,
                    "type-based value accessor must either supply the Iter or Sen type");
            }
            template<class T>
            constexpr const T& get() const&
            {
                if constexpr (AZStd::same_as<T, Iter>)
                {
                    AZ_Assert(m_elementIndex == 0, "Attempting to retrieve an inactive alternative");
                    return m_iter;
                }
                else if constexpr (AZStd::same_as<T, Sen>)
                {
                    AZ_Assert(m_elementIndex == 1, "Attempting to retrieve an inactive alternative");
                    return m_sentinel;
                }

                static_assert(AZStd::same_as<T, Iter> || AZStd::same_as<T, Sen>,
                    "type-based value accessor must either supply the Iter or Sen type");
            }
            template<class T>
            constexpr T&& get() &&
            {
                if constexpr (AZStd::same_as<T, Iter>)
                {
                    AZ_Assert(m_elementIndex == 0, "Attempting to retrieve an inactive alternative");
                    return AZStd::move(m_iter);
                }
                else if constexpr (AZStd::same_as<T, Sen>)
                {
                    AZ_Assert(m_elementIndex == 1, "Attempting to retrieve an inactive alternative");
                    return AZStd::move(m_sentinel);
                }

                static_assert(AZStd::same_as<T, Iter> || AZStd::same_as<T, Sen>,
                    "type-based value accessor must either supply the Iter or Sen type");
            }
            template<class T>
            constexpr const T&& get() const&&
            {
                if constexpr (AZStd::same_as<T, Iter>)
                {
                    AZ_Assert(m_elementIndex == 0, "Attempting to retrieve an inactive alternative");
                    return AZStd::move(m_iter);
                }
                else if constexpr (AZStd::same_as<T, Sen>)
                {
                    AZ_Assert(m_elementIndex == 1, "Attempting to retrieve an inactive alternative");
                    return AZStd::move(m_sentinel);
                }

                static_assert(AZStd::same_as<T, Iter> || AZStd::same_as<T, Sen>,
                    "type-based value accessor must either supply the Iter or Sen type");
            }

        private:
            Iter m_iter{};
            Sen m_sentinel{};
            uint8_t m_elementIndex{};
        };

        template<size_t Index, class Iter, class Sen>
        static constexpr auto& get(IterVariant<Iter, Sen>& elementIter)
        {
            return elementIter.template get<Index>();
        }
        template<size_t Index, class Iter, class Sen>
        static constexpr const auto& get(const IterVariant<Iter, Sen>& elementIter)
        {
            return elementIter.template get<Index>();
        }
        template<size_t Index, class Iter, class Sen>
        static constexpr auto&& get(IterVariant<Iter, Sen>&& elementIter)
        {
            return AZStd::move(elementIter).template get<Index>();
        }

        template<class T, class Iter, class Sen>
        static constexpr auto& get(IterVariant<Iter, Sen>& elementIter)
        {
            return elementIter.template get<T>();
        }
        template<class T, class Iter, class Sen>
        static constexpr const auto& get(const IterVariant<Iter, Sen>& elementIter)
        {
            return elementIter.template get<T>();
        }
        template<class T, class Iter, class Sen>
        static constexpr auto&& get(IterVariant<Iter, Sen>&& elementIter)
        {
            return AZStd::move(elementIter).template get<T>();
        }

        template<class T, class Iter, class Sen>
        static constexpr bool holds_alternative(const IterVariant<Iter, Sen>& elementIter)
        {
            if constexpr (AZStd::same_as<T, Iter>)
            {
                return elementIter.index() == 0;
            }
            else if constexpr (AZStd::same_as<T, Sen>)
            {
                return elementIter.index() == 1;
            }
            else
            {
                return false;
            }
        }

        IterVariant<I, S> m_iterSentinel;
#endif
    };

    template<class I, class S>
    struct incrementable_traits<common_iterator<I, S>>
    {
        using difference_type = iter_difference_t<I>;
    };
}

namespace AZStd::Internal
{
    struct common_iterator_iterator_trait_requirements_fulfilled{};
}

namespace AZStd
{
    template<class I, class S>
    struct iterator_traits<common_iterator<I, S>>
        : enable_if_t<input_iterator<I>, Internal::common_iterator_iterator_trait_requirements_fulfilled>
    {
    private:
        template<class I2, class = void>
        struct get_pointer_type_alias
        {
            using type = void;
        };
        template<class I2>
        struct get_pointer_type_alias<I2, enable_if_t<Internal::has_operator_arrow<I2>>>
        {
            using type = decltype(declval<I>().operator->());
        };

        template<class I2, class = void>
        struct get_iterator_category
        {
            using type = input_iterator_tag;
        };

        template<class I2>
        struct get_iterator_category<I2, enable_if_t<
            derived_from<typename iterator_traits<I2>::iterator_category, forward_iterator_tag>
        >>
        {
            using type = forward_iterator_tag;
        };
    public:
        using iterator_concept = AZStd::conditional_t<forward_iterator<I>, forward_iterator_tag,
            input_iterator_tag>;
        using iterator_category = typename get_iterator_category<I>::type;
        using value_type = iter_value_t<I>;
        using difference_type = iter_difference_t<I>;
        using pointer = typename get_pointer_type_alias<I>::type;
        using reference = iter_reference_t<I>;

    };
}
