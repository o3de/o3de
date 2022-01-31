/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/identity.h>
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/ranges/ranges_functional.h>

#if !defined(AZ_COMPILER_MSVC)
#define AZ_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define AZ_NO_UNIQUE_ADDRESS \
    AZ_PUSH_DISABLE_WARNING_MSVC(4848) \
    [[msvc::no_unique_address]] \
    AZ_POP_DISABLE_WARNING
#endif

namespace AZStd::ranges
{
    template<class T>
    struct min_max_result
    {
        AZ_NO_UNIQUE_ADDRESS T min;
        AZ_NO_UNIQUE_ADDRESS T max;

        template<class T2, class = enable_if_t<convertible_to<const T&, T2>>>
        constexpr operator min_max_result<T2>() const&
        {
            return { min, max };
        }

        template<class T2, class = enable_if_t<convertible_to<T, T2>>>
        constexpr operator min_max_result<T2>()&&
        {
            return { std::move(min), std::move(max) };
        }
    };
    template<class T>
    using minmax_result = min_max_result<T>;

    template<class I>
    using minmax_element_result = min_max_result<I>;

    namespace Internal
    {
        struct min_fn
        {
            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(const T& a, const T& b, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<indirect_strict_weak_order<Comp, projected<const T*, Proj>>, const T&>
            {
                return AZStd::invoke(comp, AZStd::invoke(proj, b), AZStd::invoke(proj, a)) ? b : a;
            }

            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<copyable<T>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<const T*, Proj>>>>, T>
            {
                AZ_Assert(r.size() > 0, "ranges::min cannot be invoked with an empty initializer_list");
                auto it = r.begin();
                auto last = r.end();
                T result{ *it };
                for (; it != last; ++it)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *it), AZStd::invoke(proj, result)))
                    {
                        result = *it;
                    }
                }
                return result;
            }

            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>,
                bool_constant<indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*>>>, range_value_t<R>>
            {
                AZ_Assert(ranges::distance(r) > 0, "ranges::min cannot be invoked with an empty range");
                auto it = ranges::begin(AZStd::forward<R>(r));
                auto last = ranges::end(AZStd::forward<R>(r));
                range_value_t<R> result{ *it };
                for (; it != last; ++it)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *it), AZStd::invoke(proj, result)))
                    {
                        result = *it;
                    }
                }
                return result;
            }
        };
    } // namespace Internal

    inline namespace customization_point_object
    {
        constexpr Internal::min_fn min{};
    }

    namespace Internal
    {
        struct max_fn
        {
            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(const T& a, const T& b, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<indirect_strict_weak_order<Comp, projected<const T*, Proj>>, const T&>
            {
                return AZStd::invoke(comp, AZStd::invoke(proj, a), AZStd::invoke(proj, b)) ? b : a;
            }

            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<copyable<T>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<const T*, Proj>>>>, T>
            {
                AZ_Assert(r.size() > 0, "ranges::max cannot be invoked with an empty initializer_list");
                auto it = r.begin();
                auto last = r.end();
                T result{ *it };
                for (; it != last; ++it)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, result), AZStd::invoke(proj, *it)))
                    {
                        result = *it;
                    }
                }
                return result;
            }

            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>,
                bool_constant<indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*>>>, range_value_t<R>>
            {
                AZ_Assert(ranges::distance(r) > 0, "ranges::max cannot be invoked with an empty range");
                auto it = ranges::begin(AZStd::forward<R>(r));
                auto last = ranges::end(AZStd::forward<R>(r));
                range_value_t<R> result{ *it };
                for (; it != last; ++it)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, result), AZStd::invoke(proj, *it)))
                    {
                        result = *it;
                    }
                }
                return result;
            }
        };
    } // namespace Internal

    inline namespace customization_point_object
    {
        constexpr Internal::max_fn max{};
    }


    namespace Internal
    {
        struct minmax_fn
        {
            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(const T& a, const T& b, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<indirect_strict_weak_order<Comp, projected<const T*, Proj>>, minmax_result<const T&>>
            {
                return AZStd::invoke(comp, AZStd::invoke(proj, b), AZStd::invoke(proj, a)) ?
                    minmax_result<const T&>{b, a} : minmax_result<const T&>{a, b};
            }

            template<class T, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<copyable<T>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<const T*, Proj>>>>,
                minmax_result<T>>
            {
                AZ_Assert(r.size() > 0, "ranges::minmax cannot be invoked with an empty initializer_list");
                auto first = r.begin();
                minmax_result<T> result{ *first, *first };
                auto last = r.end();
                for (; first != last; ++first)
                {
                    // Algorithm requires at most (3/2 * ranges::distance(r)) comparisons
                    if (auto prevIt = first++; first == last)
                    {
                        // Find the leftmost smallest element
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.min)))
                        {
                            result.min = *prevIt;
                        }
                        // Find the rightmost largest element
                        else if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.max)))
                        {
                            result.max = *prevIt;
                        }
                        break;
                    }
                    else
                    {
                        // For every two elements performs a transitive comparisons to calculate either new
                        // leftmost min or rightmost max using at most three comparisons
                        // (*first < *prevIt) && (*first < result.min), then *first = leftmost min
                        // (*first < *prevIt) && (*prevIt >= result.max), then *prevIt = rightmost max
                        // else
                        // (*first >= *prevIt) && (*prevIt < result.min), then *prevIt = leftmost min
                        // (*first >= *prevIt) && (*first >= result.max), then *first = rightmost max
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *prevIt)))
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, result.min)))
                            {
                                result.min = *first;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.max)))
                            {
                                result.max = *prevIt;
                            }
                        }
                        else
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.min)))
                            {
                                result.min = *prevIt;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, result.max)))
                            {
                                result.max = *first;
                            }
                        }
                    }
                }
                return result;
            }
            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>,
                bool_constant<indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*>>>,
                minmax_result<range_value_t<R>>>
            {
                AZ_Assert(ranges::distance(r) > 0, "ranges::minmax cannot be invoked with an empty range");
                auto first = ranges::begin(AZStd::forward<R>(r));
                minmax_result<range_value_t<R>> result{ *first, *first };
                auto last = ranges::end(AZStd::forward<R>(r));
                for (; first != last; ++first)
                {
                    // Algorithm requires at most (3/2 * ranges::distance(r)) comparisons
                    if (auto prevIt = first++; first == last)
                    {
                        // Find the leftmost smallest element
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.min)))
                        {
                            result.min = *prevIt;
                        }
                        // Find the rightmost largest element
                        else if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.max)))
                        {
                            result.max = *prevIt;
                        }
                        break;
                    }
                    else
                    {
                        // For every two elements performs a transitive comparisons to calculate either new
                        // leftmost min or rightmost max using at most three comparisons
                        // (*first < *prevIt) && (*first < result.min), then *first = leftmost min
                        // (*first < *prevIt) && (*prevIt >= result.max), then *prevIt = rightmost max
                        // else
                        // (*first >= *prevIt) && (*prevIt < result.min), then *prevIt = leftmost min
                        // (*first >= *prevIt) && (*first >= result.max), then *first = rightmost max
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *prevIt)))
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, result.min)))
                            {
                                result.min = *first;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.max)))
                            {
                                result.max = *prevIt;
                            }
                        }
                        else
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, result.min)))
                            {
                                result.min = *prevIt;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, result.max)))
                            {
                                result.max = *first;
                            }
                        }
                    }
                }
                return result;
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::minmax_fn minmax{};
    }

    namespace Internal
    {
        struct min_element_fn
        {
            template<class I, class S, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(I first, S last, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<I, Proj>>>
                >, I>
            {
                I result{ first };
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *result)))
                    {
                        result = first;
                    }
                }
                return result;
            }
            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>
                >, borrowed_iterator_t<R>>
            {
                auto it = ranges::begin(AZStd::forward<R>(r));
                auto last = ranges::end(AZStd::forward<R>(r));
                borrowed_iterator_t<R> result{ it };
                for (; it != last; ++it)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *it), AZStd::invoke(proj, *result)))
                    {
                        result = it;
                    }
                }
                return result;
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::min_element_fn min_element{};
    }

    namespace Internal
    {
        struct max_element_fn
        {
            template<class I, class S, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(I first, S last, Comp comp = {}, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<I, Proj>>>
                >, I>
            {
                I result{ first };
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *result), AZStd::invoke(proj, *first)))
                    {
                        result = first;
                    }
                }
                return result;
            }
            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>
                >, borrowed_iterator_t<R>>
            {
                auto first = ranges::begin(AZStd::forward<R>(r));
                auto last = ranges::end(AZStd::forward<R>(r));
                borrowed_iterator_t<R> result{ first };
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *result), AZStd::invoke(proj, *first)))
                    {
                        result = first;
                    }
                }
                return result;
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::max_element_fn max_element{};
    }

    namespace Internal
    {
        // C++ Standard footnote
        // minmax_element returns the rightmost iterator with the largest value
        // and is intentionally different from max_element which returns
        // the leftmost iterator with the largest value
        // https://eel.is/c++draft/algorithms#footnoteref-225
        struct minmax_element_fn
        {
            template<
                class I,
                class S,
                class Proj = identity,
                class Comp = ranges::less>
                constexpr auto operator()(I first, S last, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<I, Proj>>>
                >, minmax_element_result<I>>
            {
                minmax_element_result<I> result{ first, first };
                for (; first != last; ++first)
                {
                    // Algorithm requires at most (3/2 * ranges::distance(r)) comparisons
                    if (auto prevIt = first++; first == last)
                    {
                        // Find the leftmost smallest element
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.min)))
                        {
                            result.min = prevIt;
                        }
                        // Find the rightmost largest element
                        else if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.max)))
                        {
                            result.max = prevIt;
                        }
                        break;
                    }
                    else
                    {
                        // For every two elements performs a transitive comparisons to calculate either new
                        // leftmost min or rightmost max using at most three comparisons
                        // (*first < *prevIt) && (*first < result.min), then *first = leftmost min
                        // (*first < *prevIt) && (*prevIt >= result.max), then *prevIt = rightmost max
                        // else
                        // (*first >= *prevIt) && (*prevIt < result.min), then *prevIt = leftmost min
                        // (*first >= *prevIt) && (*first >= result.max), then *first = rightmost max
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *prevIt)))
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *result.min)))
                            {
                                result.min = first;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.max)))
                            {
                                result.max = prevIt;
                            }
                        }
                        else
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.min)))
                            {
                                result.min = prevIt;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *result.max)))
                            {
                                result.max = first;
                            }
                        }
                    }
                }
                return result;
            }
            template<class R, class Proj = identity, class Comp = ranges::less>
            constexpr auto operator()(R&& r, Comp comp = {}, Proj proj = {}) const
                ->enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>>
                >, minmax_element_result<borrowed_iterator_t<R>>>
            {
                auto first = ranges::begin(AZStd::forward<R>(r));
                minmax_element_result<borrowed_iterator_t<R>> result{ first, first };
                auto last = ranges::end(AZStd::forward<R>(r));
                for (; first != last; ++first)
                {
                    // Algorithm requires at most (3/2 * ranges::distance(r)) comparisons
                    if (auto prevIt = first++; first == last)
                    {
                        // Find the leftmost smallest element
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.min)))
                        {
                            result.min = prevIt;
                        }
                        // Find the rightmost largest element
                        else if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.max)))
                        {
                            result.max = prevIt;
                        }
                        break;
                    }
                    else
                    {
                        // For every two elements performs a transitive comparisons to calculate either new
                        // leftmost min or rightmost max using at most three comparisons
                        // (*first < *prevIt) && (*first < result.min), then *first = leftmost min
                        // (*first < *prevIt) && (*prevIt >= result.max), then *prevIt = rightmost max
                        // else
                        // (*first >= *prevIt) && (*prevIt < result.min), then *prevIt = leftmost min
                        // (*first >= *prevIt) && (*first >= result.max), then *first = rightmost max
                        if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *prevIt)))
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *result.min)))
                            {
                                result.min = first;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.max)))
                            {
                                result.max = prevIt;
                            }
                        }
                        else
                        {
                            if (AZStd::invoke(comp, AZStd::invoke(proj, *prevIt), AZStd::invoke(proj, *result.min)))
                            {
                                result.min = prevIt;
                            }
                            if (!AZStd::invoke(comp, AZStd::invoke(proj, *first), AZStd::invoke(proj, *result.max)))
                            {
                                result.max = first;
                            }
                        }
                    }
                }
                return result;
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::minmax_element_fn minmax_element{};
    }
} // namespace AZStd::ranges

#undef AZ_NO_UNIQUE_ADDRESS
