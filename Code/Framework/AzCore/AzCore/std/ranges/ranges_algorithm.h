/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/identity.h>
#include <AzCore/std/ranges/iter_move.h>
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/ranges/ranges_functional.h>
#include <AzCore/std/ranges/subrange.h>
#include <AzCore/std/reference_wrapper.h>
#include <AzCore/std/utility/as_const.h>

namespace AZStd::ranges
{
    // Algorithm result types
    // https://eel.is/c++draft/algorithms#results
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
            return { AZStd::move(min), AZStd::move(max) };
        }
    };
    template<class T>
    using minmax_result = min_max_result<T>;

    template<class I>
    using minmax_element_result = min_max_result<I>;

    template<class I, class F>
    struct in_fun_result
    {
        AZ_NO_UNIQUE_ADDRESS I in;
        AZ_NO_UNIQUE_ADDRESS F fun;

        template<class I2, class F2, class = enable_if_t<convertible_to<const I&, I2> && convertible_to<const F&, F2>>>
        constexpr operator in_fun_result<I2, F2>() const&
        {
            return { in, fun };
        }

        template<class I2, class F2, enable_if_t<convertible_to<I, I2> && convertible_to<F, F2>>>
        constexpr operator in_fun_result<I2, F2>() &&
        {
            return { AZStd::move(in), AZStd::move(fun) };
        }
    };

    template<class I, class O>
    struct in_out_result
    {
        AZ_NO_UNIQUE_ADDRESS I in;
        AZ_NO_UNIQUE_ADDRESS O out;

        template<class I2, class O2, class = enable_if_t<convertible_to<const I&, I2> && convertible_to<const O&, O2>>>
        constexpr operator in_out_result<I2, O2>() const&
        {
            return { in, out };
        }

        template<class I2, class O2, class = enable_if_t<convertible_to<I, I2> && convertible_to<O, O2>>>
        constexpr operator in_out_result<I2, O2>()&&
        {
            return { AZStd::move(in), AZStd::move(out) };
        }
    };

    template<class I1, class I2, class O>
    struct in_in_out_result
    {
        AZ_NO_UNIQUE_ADDRESS I1 in1;
        AZ_NO_UNIQUE_ADDRESS I2 in2;
        AZ_NO_UNIQUE_ADDRESS O out;

        template<class II1, class II2, class OO, class = enable_if_t<
            convertible_to<const I1&, II1>
            && convertible_to<const I2&, II2>
            && convertible_to<const O&, OO>>>
        constexpr operator in_in_out_result<II1, II2, OO>() const&
        {
            return { in1, in2, out };
        }

        template<class II1, class II2, class OO, class = enable_if_t<
            convertible_to<I1, II1>
            && convertible_to<I2, II2>
            && convertible_to<O, OO>>>
        constexpr operator in_in_out_result<II1, II2, OO>() &&
        {
            return { AZStd::move(in1), AZStd::move(in2), AZStd::move(out) };
        }
    };

    template<class O, class T>
    struct out_value_result
    {
        AZ_NO_UNIQUE_ADDRESS O out;
        AZ_NO_UNIQUE_ADDRESS T value;

        template<class O2, class T2, class = enable_if_t<convertible_to<const O&, O2> && convertible_to<const T&, T2>>>
        constexpr operator out_value_result<O2, T2>() const&
        {
            return { out, value };
        }

        template<class O2, class T2, class = enable_if_t<convertible_to<O, O2> && convertible_to<T, T2>>>
        constexpr operator out_value_result<O2, T2>() &&
        {
            return { AZStd::move(out), AZStd::move(value) };
        }
    };

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
                auto it = ranges::begin(r);
                auto last = ranges::end(r);
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
                auto it = ranges::begin(r);
                auto last = ranges::end(r);
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
                auto first = ranges::begin(r);
                minmax_result<range_value_t<R>> result{ *first, *first };
                auto last = ranges::end(r);
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
                auto it = ranges::begin(r);
                auto last = ranges::end(r);
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
                auto first = ranges::begin(r);
                auto last = ranges::end(r);
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
                auto first = ranges::begin(r);
                minmax_element_result<borrowed_iterator_t<R>> result{ first, first };
                auto last = ranges::end(r);
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

    // find algorithms
    namespace Internal
    {
        struct find_fn
        {
            template<class I, class S, class T, class Proj = identity, class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*>>
                >>>
            constexpr I operator()(I first, S last, const T& value, Proj proj = {}) const
            {
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(proj, *first) == value)
                    {
                        return first;
                    }
                }

                return first;
            }
            template<class R, class T, class Proj = identity, class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T*>>
                >>>
            constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    value, AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_fn find{};
    }

    namespace Internal
    {
        struct find_if_fn
        {
            template<class I, class S, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>
                >>>
            constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                    {
                        return first;
                    }
                }

                return first;
            }
            template<class R,  class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_if_fn find_if{};
    }

    namespace Internal
    {
        struct find_if_not_fn
        {
            template<class I, class S, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>
                >>>
            constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                for (; first != last; ++first)
                {
                    if (!AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                    {
                        return first;
                    }
                }

                return first;
            }
            template<class R, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_iterator_t<R>
                operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_if_not_fn find_if_not{};
    }

    namespace Internal
    {
        struct find_first_of_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<forward_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
                constexpr I1 operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                    Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                for (; first1 != last1; ++first1)
                {
                    for (I2 elementIt = first2; elementIt != last2; ++elementIt)
                    {
                        if (AZStd::invoke(pred, AZStd::invoke(proj1, *first1), AZStd::invoke(proj2, *elementIt)))
                        {
                            return first1;
                        }
                    }
                }

                return first1;
            }
            template<class R1, class R2,class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<forward_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
            >>>
            constexpr borrowed_iterator_t<R1> operator()(R1&& r1, R2&& r2,
                    Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                        AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_first_of_fn find_first_of{};
    }

    // ranges::find_last algorithms
    namespace Internal
    {
        struct find_last_fn
        {
            template<class I, class S, class T, class Proj = identity, class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*>>
                >>>
            constexpr subrange<I> operator()(I first, S last, const T& value, Proj proj = {}) const
            {
                if constexpr (bidirectional_iterator<I> && same_as<I, S>)
                {
                    for (I iter{ last }; iter!= first; --iter)
                    {
                        if (AZStd::invoke(proj, *ranges::prev(iter)) == value)
                        {
                            return { ranges::prev(iter), last};
                        }
                    }

                    return { last, last };
                }
                else
                {
                    optional<I> foundIter;
                    for (; first != last; ++first)
                    {
                        if (AZStd::invoke(proj, *first) == value)
                        {
                            foundIter = first;
                        }
                    }

                    // sentinel S might not be assignable to an iterator I
                    // so use the first iterator that has reached the end instead
                    return foundIter ? subrange<I>{ *foundIter, first } : subrange<I>{ first, first };
                }

            }
            template<class R, class T, class Proj = identity, class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T*>>
                >>>
            constexpr borrowed_subrange_t<R> operator()(R&& r, const T& value, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    value, AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_last_fn find_last{};
    }

    namespace Internal
    {
        struct find_last_if_fn
        {
            template<class I, class S, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>
                >>>
            constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                if constexpr (bidirectional_iterator<I> && same_as<I, S>)
                {
                    for (I iter{ last }; iter != first; --iter)
                    {
                        if (AZStd::invoke(pred, AZStd::invoke(proj, *ranges::prev(iter))))
                        {
                            return { ranges::prev(iter), last };
                        }
                    }

                    return { last, last };
                }
                else
                {
                    optional<I> foundIter;
                    for (; first != last; ++first)
                    {
                        if (AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                        {
                            foundIter = first;
                        }
                    }

                    return foundIter ? subrange<I>{ *foundIter, first } : subrange<I>{ first, first };
                }
            }
            template<class R, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_last_if_fn find_last_if{};
    }

    namespace Internal
    {
        struct find_last_if_not_fn
        {
            template<class I, class S, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>
                >>>
            constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                if constexpr (bidirectional_iterator<I> && same_as<I, S>)
                {
                    for (I iter{ last }; iter != first; --iter)
                    {
                        if (!AZStd::invoke(pred, AZStd::invoke(proj, *ranges::prev(iter))))
                        {
                            return { ranges::prev(iter), last };
                        }
                    }

                    return { last, last };
                }
                else
                {
                    optional<I> foundIter;
                    for (; first != last; ++first)
                    {
                        if (!AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                        {
                            foundIter = first;
                        }
                    }

                    return foundIter ? subrange<I>{ *foundIter, first } : subrange<I>{ first, first };
                }
            }
            template<class R, class Proj = identity, class Pred, class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_subrange_t<R>
                operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_last_if_not_fn find_last_if_not{};
    }


    // ranges::mismatch
    template<class I1, class I2>
    using mismatch_result = in_in_result<I1, I2>;

    namespace Internal
    {
        struct mismatch_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
            constexpr mismatch_result<I1, I2> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                for (; first1 != last1 && first2 != last2; ++first1, ++first2)
                {
                    if (!AZStd::invoke(pred, AZStd::invoke(proj1, *first1), AZStd::invoke(proj2, *first2)))
                    {
                        return { first1, first2 };
                    }
                }

                return { first1, first2 };
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
            constexpr mismatch_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>> operator()(R1&& r1, R2&& r2,
                Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::mismatch_fn mismatch{};
    }

    namespace Internal
    {
        struct equal_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
                constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                    Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                if constexpr (sized_sentinel_for<S1, I1> && sized_sentinel_for<S2, I2>)
                {
                    if (ranges::distance(first1, last1) != ranges::distance(first2, last2))
                    {
                        return false;
                    }
                }

                for (; first1 != last1 && first2 != last2; ++first1, ++first2)
                {
                    if (!AZStd::invoke(pred, AZStd::invoke(proj1, *first1), AZStd::invoke(proj2, *first2)))
                    {
                        return false;
                    }
                }

                return first1 == last1 && first2 == last2;
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
                constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::equal_fn equal{};
    }

    namespace Internal
    {
        struct lexicographical_compare_fn
        {
            template<class I1, class S1, class I2, class S2, class Comp = ranges::less, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<I1, Proj1>, projected<I2, Proj2>>>
                >>>
                constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                    Comp compd = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                for (; first1 != last1 && first2 != last2; ++first1, ++first2)
                {
                    if (AZStd::invoke(compd, AZStd::invoke(proj1, *first1), AZStd::invoke(proj2, *first2)))
                    {
                        return true;
                    }
                    if (AZStd::invoke(compd, AZStd::invoke(proj2, *first2), AZStd::invoke(proj1, *first1)))
                    {
                        return false;
                    }
                }

                // Each element in min(last1 - first1, last2 - first2) are not lexicographically less than each other
                // The first sequence legnth is then check to see if it is smaller than the second sequence length
                return first1 == last1 && first2 != last2;
            }

            template<class R1, class R2, class Comp = ranges::less, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<indirect_strict_weak_order<Comp, projected<iterator_t<R1>, Proj1>, projected<iterator_t<R2>, Proj2>>>
                >>>
                constexpr bool operator()(R1&& r1, R2&& r2, Comp compd = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(compd), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::lexicographical_compare_fn lexicographical_compare{};
    }

    namespace Internal
    {
        struct search_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<forward_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
            constexpr subrange<I1> operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {},
                Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                do
                {
                    I1 it1 = first1;
                    I2 it2 = first2;
                    for (;; ++it1, ++it2)
                    {
                        if (it2 == last2)
                        {
                            // Reached the end of the second iteator sequence
                            // Therefore the search has succeeded. return the matching range from the first sequence
                            return { first1, it1 };
                        }
                        if (it1 == last1)
                        {
                            // The search has failed to find the second iterator sequence within the first
                            return { last1, last1 };
                        }
                        if (!AZStd::invoke(pred, AZStd::invoke(proj1, *it1), AZStd::invoke(proj2, *it2)))
                        {
                            // Mismatch found break out of iteration of loop
                            break;
                        }
                    }
                    // Increment to the next element in the range of [first1, last1) and restart the search
                    ++first1;
                } while (true);
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R1>>,
                bool_constant<forward_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
            constexpr borrowed_subrange_t<R1>
                operator()(R1&& r1, R2&& r2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                        AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }

        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::search_fn search{};
    }

    namespace Internal
    {
        struct search_n_fn
        {
            template<class I, class S, class T, class Pred = equal_to, class Proj = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirectly_comparable<I, const T*, Pred, Proj>>
                >>>
                constexpr subrange<I> operator()(I first, S last, iter_difference_t<I> count,
                    const T& value, Pred pred = {}, Proj proj = {}) const
            {
                for (; first != last; ++first)
                {
                    iter_difference_t<I> foundCount{};
                    I searchFirst = first;
                    for (; foundCount != count && first != last; ++first, ++foundCount)
                    {
                        if (!AZStd::invoke(pred, AZStd::invoke(proj, *first), value))
                        {
                            break;
                        }
                    }
                    if (foundCount == count)
                    {
                        // count consecutive elements matching value have been found
                        return { searchFirst, first };
                    }
                    if (first == last)
                    {
                        // search has failed to find count matching elements over the range.
                        break;
                    }
                }

                return { last, last };
            }
            template<class R, class T, class Pred = equal_to, class Proj = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirectly_comparable<iterator_t<R>, const T*, Pred, Proj>>
                >>>
                constexpr borrowed_subrange_t<R> operator()(R&& r, range_difference_t<R> count,
                    const T& value, Pred pred = {}, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r),
                    AZStd::move(count), value, AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::search_n_fn search_n{};
    }

    // ranges::find_end_fn
    // use ranges::search
    namespace Internal
    {
        struct find_end_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<forward_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
                constexpr subrange<I1> operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                if (first2 == last2)
                {
                    return { last1, last1 };
                }
                auto foundSubrange = ranges::search(first1, last1, first2, last2, pred, proj1, proj2);
                if (foundSubrange.empty())
                {
                    return foundSubrange;
                }
                do
                {
                    auto nextIt = ranges::next(foundSubrange.begin());
                    auto nextSubrange = ranges::search(nextIt, last1, first2, last2, pred, proj1, proj2);
                    if (nextSubrange.empty())
                    {
                        return foundSubrange;
                    }

                    foundSubrange = AZStd::move(nextSubrange);

                } while (true);
            }
            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R1>>,
                bool_constant<forward_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
                constexpr borrowed_subrange_t<R1> operator()(R1&& r1, R2&& r2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                        AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::find_end_fn find_end{};
    }

    // ranges::all_of
    // ranges::any_of
    // ranges::none_of
    namespace Internal
    {
        struct all_of_fn
        {
            template<class I, class S, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>>
            >>
            constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                return ranges::find_if_not(first, last, AZStd::ref(pred), AZStd::ref(proj)) == last;
            }

            template<class R, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<ranges::iterator_t<R>, Proj>>>>
            >>
            constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), AZStd::ref(pred), AZStd::ref(proj));
            }
        };
    } // namespace Internal
    inline namespace customization_point_object
    {
        constexpr Internal::all_of_fn all_of;
    } // namespace customization_point_object


    namespace Internal
    {
        struct any_of_fn
        {
            template<class I, class S, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>>
            >>
            constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                return ranges::find_if(first, last, AZStd::ref(pred), AZStd::ref(proj)) != last;
            }

            template<class R, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<ranges::iterator_t<R>, Proj>>>>
            >>
            constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), AZStd::ref(pred), AZStd::ref(proj));
            }
        };

    } // namespace Internal

    inline namespace customization_point_object
    {
        inline constexpr Internal::any_of_fn any_of;
    } // namespace customization_point_object

    namespace Internal
    {
        struct none_of_fn
        {
            template<class I, class S, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>>
            >>
            constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                return ranges::find_if(first, last, AZStd::ref(pred), AZStd::ref(proj)) == last;
            }

            template<class R, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<ranges::iterator_t<R>, Proj>>>>
            >>
            constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), AZStd::ref(pred), AZStd::ref(proj));
            }
        };

    } // namespace Internal

    inline namespace customization_point_object
    {
        inline constexpr Internal::none_of_fn none_of;
    } // namespace customization_point_object


    // ranges::for_each
    // ranges::for_each_n
    template<class I, class F>
    using for_each_result = in_fun_result<I, F>;
    template<class I, class F>
    using for_each_n_result = in_fun_result<I, F>;

    namespace Internal
    {
        struct for_each_fn
        {
            template<class I, class S, class Proj = identity, class Fun,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirectly_unary_invocable<Fun, projected<I, Proj>>>
                >> >
            constexpr for_each_result<I, Fun> operator()(I first, S last, Fun f, Proj proj = {}) const
            {
                for (; first != last; ++first)
                {
                    AZStd::invoke(f, AZStd::invoke(proj, *first));
                }

                return { AZStd::move(first), AZStd::move(f) };
            }

            template<class R, class Proj = identity, class Fun,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirectly_unary_invocable<Fun, projected<iterator_t<R>, Proj>>>
                >> >
            constexpr for_each_result<borrowed_iterator_t<R>, Fun> operator()(R&& r, Fun f, Proj proj = {}) const
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(f), AZStd::move(proj));
            }
        };

        struct for_each_n_fn
        {
            template<class I, class Proj = identity, class Fun,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<indirectly_unary_invocable<Fun, projected<I, Proj>>>
                >> >
            constexpr for_each_n_result<I, Fun> operator()(I first, iter_difference_t<I> n, Fun f, Proj proj = {}) const
            {
                for (; n > 0; ++first, --n)
                {
                    AZStd::invoke(f, AZStd::invoke(proj, *first));
                }

                return { first, AZStd::move(f) };
            }

        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::for_each_fn for_each{};
        constexpr Internal::for_each_n_fn for_each_n{};
    }


    // ranges::count
    // ranges::count_if
    namespace Internal
    {
        struct count_fn
        {
            template<class I, class S, class T, class Proj = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*>>
                >> >
            constexpr iter_difference_t<I> operator()(I first, S last, const T& value, Proj proj = {}) const
            {
                iter_difference_t<I> counter{};
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(proj, *first) == value)
                    {
                        ++counter;
                    }
                }

                return counter;
            }

            template<class R, class T, class Proj = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, const T*>>
                >> >
            constexpr range_difference_t<R> operator()(R&& r, const T& value, Proj proj = {}) const
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), value, AZStd::move(proj));
            }
        };

        struct count_if_fn
        {
            template<class I, class S, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>
                >> >
            constexpr iter_difference_t<I> operator()(I first, S last, Pred pred, Proj proj = {}) const
            {
                iter_difference_t<I> counter{};
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                    {
                        ++counter;
                    }
                }

                return counter;
            }

            template<class R, class Proj = identity, class Pred,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>
                >> >
            constexpr range_difference_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(pred), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::count_fn count{};
        constexpr Internal::count_if_fn count_if{};
    }

    // Mutating Sequence Operations
    // https://eel.is/c++draft/algorithms#alg.modifying.operations

    // ranges::copy
    // ranges::copy_if
    // ranges::copy_n
    // ranges::copy_backward
    template<class I, class O>
    using copy_result = in_out_result<I, O>;
    template<class I, class O>
    using copy_if_result = in_out_result<I, O>;
    template<class I, class O>
    using copy_n_result = in_out_result<I, O>;
    template<class I1, class I2>
    using copy_backward_result = in_out_result<I1, I2>;

    namespace Internal
    {
        template<class I, class O, class = void>
        constexpr bool can_use_memcpy = false;

        // memcpy can be used instead of copy assignment
        // when both the input and output iter_value_t are the same,
        // the output iterator is trivially copyable
        // and both iterators are contiguous
        template<class I, class O>
        constexpr bool can_use_memcpy<I, O, enable_if_t<
            same_as<iter_value_t<I>, iter_value_t<O>>
            && is_trivially_copyable_v<iter_value_t<O>>
            && contiguous_iterator<I>
            && contiguous_iterator<O>
            >> = true;

        struct copy_fn
        {
            template<class I, class S, class O>
            constexpr auto operator()(I first, S last, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirectly_copyable<I, O>>
                >, copy_result<I, O>>
            {
                if constexpr (can_use_memcpy<I, O>)
                {
                    const size_t numElements = ranges::distance(first, last);
                    // Specialized copy for contiguous iterators which are trivially copyable
                    if (numElements > 0)
                    {
#if az_has_builtin_memcpy
                        __builtin_memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I>));
#else
                        if (az_builtin_is_constant_evaluated())
                        {
                            for (; first != last; ++first, ++result)
                            {
                                *result = *first;
                            }
                        }
                        else
                        {
                            AZ_Assert(to_address(result) < to_address(first) || to_address(result) >= to_address(first) + numElements,
                                "AZStd::ranges::copy results in memory overlap. AZStd::ranges::copy_backward should be used instead");
                            ::memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I>));
                            ranges::advance(result, numElements);
                        }
#endif
                    }
                }
                else
                {
                    for (; first != last; ++first, ++result)
                    {
                        *result = *first;
                    }
                }

                return { AZStd::move(last), AZStd::move(result) };
            }

            template<class R, class O>
            constexpr auto operator()(R&& r, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirectly_copyable<iterator_t<R>, O>>
                >, copy_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(result));
            }
        };

        struct copy_if_fn
        {
            template<class I, class S, class O, class Proj = identity, class Pred>
            constexpr auto operator()(I first, S last, O result, Pred pred, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirect_unary_predicate<Pred, projected<I, Proj>>>,
                bool_constant<indirectly_copyable<I, O>>
                >, copy_if_result<I, O>>
            {
                for (; first != last; ++first)
                {
                    if (AZStd::invoke(pred, AZStd::invoke(proj, *first)))
                    {
                        *result = *first;
                        ++result;
                    }
                }

                return { AZStd::move(last), AZStd::move(result) };
            }

            template<class R, class O, class Proj = identity, class Pred>
            constexpr auto operator()(R&& r, O result, Pred pred, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirect_unary_predicate<Pred, projected<iterator_t<R>, Proj>>>,
                bool_constant<indirectly_copyable<iterator_t<R>, O>>
                >, copy_if_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(result),
                    AZStd::move(pred), AZStd::move(proj));
            }
        };

        struct copy_n_fn
        {
            template<class I, class O>
            constexpr auto operator()(I first, iter_difference_t<I> n, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirectly_copyable<I, O>>
                >, copy_n_result<I, O>>
            {
                if constexpr (can_use_memcpy<I, O>)
                {
                    // Specialized copy for contiguous iterators which are trivially copyable
#if az_has_builtin_memcpy
                    __builtin_memcpy(to_address(result), to_address(first), n * sizeof(iter_value_t<I>));
#else
                    if (az_builtin_is_constant_evaluated())
                    {
                        for (; n > 0; --n, ++first, ++result)
                        {
                            *result = *first;
                        }
                    }
                    else
                    {
                        AZ_Assert(to_address(result) < to_address(first) || to_address(result) >= to_address(first) + n,
                            "AZStd::ranges::copy_n results in memory overlap. AZStd::ranges::copy_backward should be used instead");
                        ::memcpy(to_address(result), to_address(first), n * sizeof(iter_value_t<I>));
                        ranges::advance(first, n);
                        ranges::advance(result, n);
                    }
#endif
                }
                else
                {
                    for (; n > 0; --n, ++first, ++result)
                    {
                        *result = *first;
                    }
                }

                return { AZStd::move(first), AZStd::move(result) };
            }
        };

        struct copy_backward_fn
        {
            template<class I1, class S1, class O>
            constexpr auto operator()(I1 first, S1 last, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<bidirectional_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<bidirectional_iterator<O>>,
                bool_constant<indirectly_copyable<I1, O>>
                >, copy_backward_result<I1, O>>
            {
                if constexpr (can_use_memcpy<I1, O>)
                {
                    // Specialized copy for contiguous iterators which are trivially copyable
                    if (const size_t numElements = ranges::distance(first, last); numElements > 0)
                    {
#if az_has_builtin_memmove
                        result -= numElements;
                        __builtin_memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I1>));
#else
                        if (az_builtin_is_constant_evaluated())
                        {
                            for (I1 iter{ last }; iter != first;)
                            {
                                *--result = *--iter;
                            }
                        }
                        else
                        {
                            result -= numElements;
                            AZ_Assert(to_address(result) + numElements <= to_address(first) || to_address(result) + numElements > to_address(first) + numElements,
                                "AZStd::ranges::copy_backward results in memory overlap. AZStd::ranges::copy should be used instead");
                            ::memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I1>));
                        }
#endif
                    }
                }
                else
                {
                    for (I1 iter{ last }; iter != first;)
                    {
                        *--result = *--iter;
                    }
                }

                return { AZStd::move(last), AZStd::move(result) };
            }

            template<class R, class O>
            constexpr auto operator()(R&& r, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<bidirectional_range<R>>,
                bool_constant<bidirectional_iterator<O>>,
                bool_constant<indirectly_copyable<iterator_t<R>, O>>
                >, copy_backward_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(result));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::copy_fn copy{};
        constexpr Internal::copy_if_fn copy_if{};
        constexpr Internal::copy_n_fn copy_n{};
        constexpr Internal::copy_backward_fn copy_backward{};
    }

    // ranges::move
    // ranges::move_backward
    template<class I, class O>
    using move_result = in_out_result<I, O>;
    template<class I1, class I2>
    using move_backward_result = in_out_result<I1, I2>;

    namespace Internal
    {
        struct move_fn
        {
            template<class I, class S, class O>
            constexpr auto operator()(I first, S last, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirectly_movable<I, O>>
                >, move_result<I, O>>
            {
                if constexpr (can_use_memcpy<I, O>)
                {
                    const size_t numElements = ranges::distance(first, last);
                    // Specialized copy for contiguous iterators which are trivially copyable
                    if (numElements > 0)
                    {
#if az_has_builtin_memcpy
                        __builtin_memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I>));
#else
                        if (az_builtin_is_constant_evaluated())
                        {
                            for (; first != last; ++first, ++result)
                            {
                                *result = AZStd::ranges::iter_move(first);
                            }
                        }
                        else
                        {
                            AZ_Assert(to_address(result) < to_address(first) || to_address(result) >= to_address(first) + numElements,
                                "AZStd::ranges::copy results in memory overlap. AZStd::ranges::copy_backward should be used instead");
                            ::memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I>));
                            ranges::advance(result, numElements);
                        }
#endif
                    }
                }
                else
                {
                    for (; first != last; ++first, ++result)
                    {
                        *result = AZStd::ranges::iter_move(first);
                    }
                }

                return { AZStd::move(last), AZStd::move(result) };
            }

            template<class R, class O>
            constexpr auto operator()(R&& r, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<indirectly_movable<iterator_t<R>, O>>
                >, move_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(result));
            }
        };

        struct move_backward_fn
        {
            template<class I1, class S1, class O>
            constexpr auto operator()(I1 first, S1 last, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<bidirectional_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<bidirectional_iterator<O>>,
                bool_constant<indirectly_movable<I1, O>>
                >, move_backward_result<I1, O>>
            {
                if constexpr (can_use_memcpy<I1, O>)
                {
                    // Specialized copy for contiguous iterators which are trivially copyable
                    if (const size_t numElements = ranges::distance(first, last); numElements > 0)
                    {
#if az_has_builtin_memmove
                        result -= numElements;
                        __builtin_memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I1>));
#else
                        if (az_builtin_is_constant_evaluated())
                        {
                            for (I1 iter{ last }; iter != first;)
                            {
                                *--result = AZStd::ranges::iter_move(--iter);
                            }
                        }
                        else
                        {
                            result -= numElements;
                            AZ_Assert(to_address(result) + numElements <= to_address(first) || to_address(result) + numElements > to_address(first) + numElements,
                                "AZStd::ranges::copy_backward results in memory overlap. AZStd::ranges::copy should be used instead");
                            ::memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<I1>));
                        }
#endif
                    }
                }
                else
                {
                    for (I1 iter{ last }; iter != first;)
                    {
                        *--result = AZStd::ranges::iter_move(--iter);
                    }
                }

                return { AZStd::move(last), AZStd::move(result) };
            }

            template<class R, class O>
            constexpr auto operator()(R&& r, O result) const
                -> enable_if_t<conjunction_v<
                bool_constant<bidirectional_range<R>>,
                bool_constant<bidirectional_iterator<O>>,
                bool_constant<indirectly_movable<iterator_t<R>, O>>
                >, move_backward_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), AZStd::move(result));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::move_fn move{};
        constexpr Internal::move_backward_fn move_backward{};
    }

    // ranges::transform
    template<class I, class O>
    using unary_transform_result = in_out_result<I, O>;
    template<class I1, class I2, class O>
    using binary_transform_result = in_in_out_result<I1, I2, O>;

    namespace Internal
    {
        struct transform_fn
        {
            template<class I, class S, class O, class F, class Proj = identity>
            constexpr auto operator()(I first1, S last1, O result, F op, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<copy_constructible<F>>,
                bool_constant<indirectly_writable<O, indirect_result_t<F&, projected<I, Proj>>>>
                >, unary_transform_result<I, O>>
            {
                for (; first1 != last1; ++first1, ++result)
                {
                    *result = AZStd::invoke(op, AZStd::invoke(proj, *first1));
                }

                return { first1, result };
            }

            template<class R, class O, class F, class Proj = identity>
            constexpr auto operator()(R&& r, O result, F op, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<copy_constructible<F>>,
                bool_constant<indirectly_writable<O, indirect_result_t<F&, projected<iterator_t<R>, Proj>>>>
                >, unary_transform_result<borrowed_iterator_t<R>, O>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), result,
                    AZStd::move(op), AZStd::move(proj));
            }

            template<class I1, class S1, class I2, class S2, class O, class F,
                class Proj1 = identity, class Proj2 = identity>
            constexpr auto operator()(I1 first1, S1 last1, I2 first2, S2 last2, O result, F binary_op,
                Proj1 proj1 = {}, Proj2 proj2 = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<copy_constructible<F>>,
                bool_constant<indirectly_writable<O, indirect_result_t<F&, projected<I1, Proj1>, projected<I2, Proj2>>>>
                >, binary_transform_result<I1, I2, O>>
            {
                for (; first1 != last1 && first2 != last2; ++first1, ++first2, ++result)
                {
                    *result = AZStd::invoke(binary_op, AZStd::invoke(proj1, *first1), AZStd::invoke(proj2, *first2));
                }

                return { first1, first2, result };
            }

            template<class R1, class R2, class O, class F,
                class Proj1 = identity, class Proj2 = identity>
            constexpr auto operator()(R1&& r1, R2&& r2, O result, F binary_op,
                Proj1 proj1 = {}, Proj2 proj2 = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<weakly_incrementable<O>>,
                bool_constant<copy_constructible<F>>,
                bool_constant<indirectly_writable<O,
                    indirect_result_t<F&, projected<iterator_t<R1>, Proj1>, projected<iterator_t<R2>, Proj2>>>>
                >, binary_transform_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>>
            {
                return operator()(AZStd::ranges::begin(r1), AZStd::ranges::end(r1),
                    AZStd::ranges::begin(r2), AZStd::ranges::end(r2), AZStd::move(result),
                    AZStd::move(binary_op), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::transform_fn transform{};
    }

    namespace Internal
    {
        struct reverse_fn
        {
            template<class I, class S>
            constexpr auto operator()(I first, S last) const
                ->enable_if_t<conjunction_v<
                bool_constant<bidirectional_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<permutable<I>>
                >, I>
            {
                for (iter_difference_t<I> i{}; i < ranges::distance(first, last) / 2; ++i)
                {
                    ranges::iter_swap(first + i, (last - i) - 1);
                }

                return last;
            }

            template<class R>
            constexpr auto operator()(R&& r) const
                ->enable_if_t<conjunction_v<
                bool_constant<bidirectional_range<R>>,
                bool_constant<permutable<iterator_t<R>>>
                >, borrowed_iterator_t<R>>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r));
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::reverse_fn reverse{};
    }

    // ranges::contains
    // ranges::contains_subrange
    namespace Internal
    {
        struct contains_fn
        {
            template<class I, class S, class T, class Proj = identity>
            constexpr auto operator()(I first, S last, const T& value, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*>>
                >, bool>
            {
                return ranges::find(AZStd::move(first), last, value, AZStd::move(proj)) != last;
            }

            template<class R, class T, class Proj = identity>
            constexpr auto operator()(R&& r, const T& value, Proj proj = {}) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T*>>
                >, bool>
            {
                return operator()(AZStd::ranges::begin(r), AZStd::ranges::end(r), value, AZStd::move(proj));
            }
        };

        struct contains_subrange_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<forward_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
            constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {},
                Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return first2 == last2 || !ranges::search(AZStd::move(first1), AZStd::move(last1),
                    AZStd::move(first2), AZStd::move(last2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2)).empty();
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R1>>,
                bool_constant<forward_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
            constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::contains_fn contains{};
        constexpr Internal::contains_subrange_fn contains_subrange{};
    }

    // ranges::starts_with
    // ranges::ends_with
    namespace Internal
    {
        struct starts_with_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
            constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                    Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                // check that the second iterator has reached the end
                return ranges::mismatch(AZStd::move(first1), last1, AZStd::move(first2), last2,
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2)).in2 == last2;
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
            constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {},
                    Proj1 proj1 = {}, Proj2 proj2 = {}) const
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };

        struct ends_with_fn
        {
            template<class I1, class S1, class I2, class S2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_iterator<I1>>,
                bool_constant<sentinel_for<S1, I1>>,
                bool_constant<input_iterator<I2>>,
                bool_constant<sentinel_for<S2, I2>>,
                bool_constant<indirectly_comparable<I1, I2, Pred, Proj1, Proj2>>
                >>>
            constexpr auto operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                    Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const
                -> enable_if_t<(forward_iterator<I1> || sized_sentinel_for<S1, I1>) &&
                    (forward_iterator<I2> || sized_sentinel_for<S2, I2>), bool>
            {
                const auto firstDist = ranges::distance(first1, last1);
                const auto secondDist = ranges::distance(first2, last2);
                // Skip past all elements in the first1 iterator until it is secondDist from the last1
                return firstDist >= secondDist && ranges::equal(ranges::next(AZStd::move(first1), firstDist - secondDist), last1,
                    AZStd::move(first2), last2,
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }

            template<class R1, class R2, class Pred = equal_to, class Proj1 = identity, class Proj2 = identity,
                class = enable_if_t<conjunction_v<
                bool_constant<input_range<R1>>,
                bool_constant<input_range<R2>>,
                bool_constant<indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>>
                >>>
            constexpr auto operator()(R1&& r1, R2&& r2,
                    Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const
                -> enable_if_t<(forward_range<R1> || sized_range<R1>) &&
                    (forward_range<R2> || sized_range<R2>), bool>
            {
                return operator()(ranges::begin(r1), ranges::end(r1),
                    ranges::begin(r2), ranges::end(r2),
                    AZStd::move(pred), AZStd::move(proj1), AZStd::move(proj2));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::starts_with_fn starts_with{};
        constexpr Internal::ends_with_fn ends_with{};
    }

    //! Constrained binary search algorithms
    // ranges::lower_bound
    namespace Internal
    {
        struct lower_bound_fn
        {
            template<class I, class S, class T, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<I, Proj>>>
                >>>
            constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                I iter = first;
                for (auto rangeCount = AZStd::ranges::distance(first, last); rangeCount > 0; iter = first)
                {
                    // Perform a binary search by cutting the count in half and searching one half
                    // of a sorted parition
                    auto partitionSize = rangeCount / 2;
                    ranges::advance(iter, partitionSize, last);
                    if (AZStd::invoke(comp, AZStd::invoke(proj, *iter), value))
                    {
                        // The element is less than the value, therefore check the right
                        // for an element that is not less than the value
                        first = ranges::next(iter);
                        // +1 is needed to point to the element after the
                        // the check element;
                        rangeCount = rangeCount - (partitionSize + 1);
                    }
                    else
                    {
                        // The element is not less than the value, therefore check the left
                        // for a better lower_bound candidate(a smaller element that is still not less than value)
                        // i.e if value = 5 and the range is [1, 6, 10, 15, 20].
                        // Then the first candidate of 10 is not less than 5, but 6 is still the best candidate
                        rangeCount = partitionSize;
                    }
                }

                return first;
            }

            template<class T, class R, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), value,
                    AZStd::move(comp), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::lower_bound_fn lower_bound{};
    }

    // ranges::upper_bound
    namespace Internal
    {
        struct upper_bound_fn
        {
            template<class I, class S, class T, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<I, Proj>>>
                >>>
            constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                I iter = first;
                for (auto rangeCount = AZStd::ranges::distance(first, last); rangeCount > 0; iter = first)
                {
                    // Perform a binary search by cutting the count in half and searching one half
                    // of a sorted parition
                    auto partitionSize = rangeCount / 2;
                    ranges::advance(iter, partitionSize, last);
                    if (!AZStd::invoke(comp, value, AZStd::invoke(proj, *iter)))
                    {
                        // The element is not greater than the value, therefore check the right
                        // for an element that is greater than the value
                        first = ranges::next(iter);
                        // +1 is needed to point to the element after the
                        // the check element;
                        rangeCount = rangeCount - (partitionSize + 1);
                    }
                    else
                    {
                        // The element is not greater than the value, therefore check the left
                        // for a better upper_bound candidate(a smaller element that is still greater)
                        // i.e if value = 5 and the range is [1, 6, 10, 15, 20].
                        // Then the first candidate of 10 is greater than 5, but 6 is still the best candidate
                        rangeCount = partitionSize;
                    }
                }

                return first;
            }

            template<class T, class R, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), value,
                    AZStd::move(comp), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::upper_bound_fn upper_bound{};
    }

    // ranges::equal_range
    namespace Internal
    {
        struct equal_range_fn
        {
            template<class I, class S, class T, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<I, Proj>>>
                >>>
            constexpr subrange<I> operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                return { ranges::lower_bound(first, last, value, comp, proj),
                    ranges::upper_bound(first, last, value, comp, proj) };
            }

            template<class T, class R, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr borrowed_subrange_t<R> operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), value,
                    AZStd::move(comp), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::equal_range_fn equal_range{};
    }

    // ranges::binary_search
    namespace Internal
    {
        struct binary_search_fn
        {
            template<class I, class S, class T, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_iterator<I>>,
                bool_constant<sentinel_for<S, I>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<I, Proj>>>
                >>>
            constexpr bool operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                auto lowerBoundIter = ranges::lower_bound(first, last, value, comp, proj);
                    return lowerBoundIter != last && !AZStd::invoke(comp, value, AZStd::invoke(proj, *lowerBoundIter));
            }

            template<class T, class R, class Proj = identity, class Comp = ranges::less,
                class = enable_if_t<conjunction_v<
                bool_constant<forward_range<R>>,
                bool_constant<indirect_strict_weak_order<Comp, const T*, projected<iterator_t<R>, Proj>>>
                >>>
            constexpr bool operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const
            {
                return operator()(ranges::begin(r), ranges::end(r), value,
                    AZStd::move(comp), AZStd::move(proj));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::binary_search_fn binary_search{};
    }

    //! Constrained numeric algorithms
    // ranges::iota

    template<class O, class T>
    using iota_result = out_value_result<O, T>;

    namespace Internal
    {
        struct iota_fn
        {
            template<class O, class S, class T,
                class = enable_if_t<conjunction_v<
                bool_constant<input_or_output_iterator<O>>,
                bool_constant<sentinel_for<S, O>>,
                bool_constant<weakly_incrementable<T>>,
                bool_constant<indirectly_writable<O, const T&>>
                >>>
            constexpr iota_result<O, T> operator()(O first, S last, T value) const
            {
                while( first != last)
                {
                    *first = as_const(value);
                    ++first;
                    ++value;
                }

                return { AZStd::move(first), AZStd::move(value) };
            }

            template<class T, class R,
                class = enable_if_t<conjunction_v<
                bool_constant<weakly_incrementable<T>>,
                bool_constant<output_range<R, const T&>>
                >>>
            constexpr iota_result<borrowed_iterator_t<R>, T> operator()(R&& r, T value) const
            {
                return operator()(ranges::begin(r), ranges::end(r), AZStd::move(value));
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::iota_fn iota{};
    }
} // namespace AZStd::ranges
