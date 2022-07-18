/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/iterator.h>
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/transform_view.h>

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class Container, class = void>
        constexpr bool has_reserve = false;
        template<class Container>
        constexpr bool has_reserve<Container, enable_if_t<
            sfinae_trigger_v<decltype(declval<Container&>().reserve(declval<range_size_t<Container>>()))>
            >> = true;

        template<class Container, class = void>
        constexpr bool has_capacity = false;
        template<class Container>
        constexpr bool has_capacity<Container, enable_if_t<
            same_as<decltype(declval<Container&>().capacity()), range_size_t<Container>>
            >> = true;

        template<class Container, class = void>
        constexpr bool has_max_size = false;
        template<class Container>
        constexpr bool has_max_size<Container, enable_if_t<
            same_as<decltype(declval<Container&>().max_size()), range_size_t<Container>>
            >> = true;

        template<class Container, class = void>
        constexpr bool reservable_container = false;
        template<class Container>
        constexpr bool reservable_container<Container, enable_if_t<conjunction_v<
            bool_constant<sized_range<Container>>,
            bool_constant<has_reserve<Container>>,
            bool_constant<has_capacity<Container>>,
            bool_constant<has_max_size<Container>>
            >>> = true;

        template<class Container, class Ref, class = void>
        constexpr bool has_push_back = false;
        template<class Container, class Ref>
        constexpr bool has_push_back<Container, Ref, enable_if_t<
            sfinae_trigger_v<decltype(declval<Container&>().push_back(declval<Ref&&>()))>
            >> = true;

        template<class Container, class Ref, class = void>
        constexpr bool has_insert = false;
        template<class Container, class Ref>
        constexpr bool has_insert<Container, Ref, enable_if_t<
            sfinae_trigger_v<decltype(declval<Container&>().insert(declval<Container&>().end(), declval<Ref&&>()))>
            >> = true;

        template<class Container, class Ref, class = void>
        constexpr bool container_insertable = false;
        // Disjunction is used here as the condition is a logical-or
        // most of the concept conditions have been logical-and until now
        // https://eel.is/c++draft/range.utility.conv#general-4
        template<class Container, class Ref>
        constexpr bool container_insertable<Container, Ref, enable_if_t<disjunction_v<
            bool_constant<has_push_back<Container, Ref>>,
            bool_constant<has_insert<Container, Ref>>
            >>> = true;

        template<class Ref, class Container>
        constexpr auto container_inserter(Container& c)
        {
            if constexpr (has_push_back<Container, Ref>)
            {
                return back_inserter(c);
            }
            else
            {
                return inserter(c, c.end());
            }
        }


        template<template<class...> class C, class R, class... Args>
        struct container_range_direct_constructible
        {
            static constexpr false_type ValidExpr(...);
            template<bool Enable = sfinae_trigger_v<decltype(C(declval<R>(), declval<Args>()...))>>
            static constexpr true_type ValidExpr(int);

            static constexpr bool value = ValidExpr(0);
        };

        template<template<class...> class C, class R, class... Args>
        constexpr bool container_range_direct_constructible_v = container_range_direct_constructible<C, R, Args...>::value;

        template<template<class...> class C, class R, class... Args>
        struct container_range_tag_type_constructible
        {
            static constexpr false_type ValidExpr(...);
            template<bool Enable = sfinae_trigger_v<decltype(C(from_range, declval<R>(), declval<Args>()...))>>
            static constexpr true_type ValidExpr(int);

            static constexpr bool value = ValidExpr(0);
        };
        template<template<class...> class C, class R, class... Args>
        constexpr bool container_range_tag_type_constructible_v = container_range_tag_type_constructible<C, R, Args...>::value;
        
        template<template<class...> class C, class R, class... Args>
        struct container_range_common_iterator_constructible
        {
            static constexpr false_type ValidExpr(...);
            template<bool Enable = sfinae_trigger_v<decltype(C(declval<I>(), declval<I>(), declval<Args>()...))>>
            static constexpr true_type ValidExpr(int);

            static constexpr bool value = ValidExpr(0);
        };
        template<template<class...> class C, class I, class... Args>
        constexpr bool container_range_common_iterator_constructible_v = container_range_common_iterator_constructible<C, I, Args...>::value;

        

        template<template<class...> class C, class R, class... Args>
        inline auto DEDUCE_EXPR()
        {
            // Exposition only type for deducing the template argument for the container template C
            // https://eel.is/c++draft/range.utility.conv#to-2
            struct range_input_iterator
            {
                using iterator_category = input_iterator_tag;
                using value_type = range_value_t<R>;
                using difference_type = ptrdiff_t;
                using reference = range_reference_t<R>;
                using pointer = add_pointer_t<reference>;
                reference* operator*() const;
                pointer operator->() const;
                range_input_iterator& operator++();
                range_input_iterator operator++(int);
                bool operator==(const range_input_iterator&) const;
                bool operator!=(const range_input_iterator&) const;
            };
            if constexpr (container_range_direct_constructible_v<C, R>)
            {
                return C(declval<R>(), declval<Args>()...);
            }
            else if constexpr (container_range_tag_type_constructible_v<C, R, Args...>)
            {
                return C(from_range, declval<R>(), declval<Args>()...);
            }
            else if constexpr (container_range_common_iterator_constructible_v<C, range_input_iterator, Args...>)
            {
                return C(declval<range_input_iterator>(), declval<range_input_iterator>(), declval<Args>()...);
            }
        }

        struct to_fn
        {
            template<class C, class R, class... Args>
            [[nodiscard]] constexpr auto operator()(R&& r, Args&&... args) const
                -> enable_if_t<conjunction_v<
                bool_constant<input_range<R>>,
                bool_constant<!view<C>>
                >, C>
            {
                if constexpr (convertible_to<range_reference_t<R>, range_value_t<C>>)
                {
                    // Container C contains supports a constructor that can accept the range directly
                    if constexpr (constructible_from<C, R, Args...>)
                    {
                        return C(AZStd::forward<R>(r), AZStd::forward<Args>(args)...);
                    }
                    // Container C contains supports from_range_t tagged constructor that can construct
                    // the C using the range
                    else if constexpr (constructible_from<C, from_range_t, R, Args...>)
                    {
                        return C(from_range, AZStd::forward<R>(r), AZStd::forward<Args>(args)...);
                    }
                    // The Range is a common range(i.e has the same iterator and sentinel type)
                    // and the Container C has a legacy iterator constructor that construct C
                    // from begin and end iterators
                    else if constexpr (common_range<R> && input_iterator<iterator_t<R>>
                        && constructible_from<C, iterator_t<R>, sentinel_t<R>, Args...>)
                    {
                        return C(ranges::begin(r), ranges::end(r), AZStd::forward<Args>(args)...);
                    }
                    // Construct an empty container and then use the inserter adapter to forward arguments
                    else if constexpr (constructible_from<C, Args...> && container_insertable<C, range_reference_t<R>>)
                    {
                        C c(AZStd::forward<Args>(args)...);
                        // If the range is a sized_range(i.e supports a constant Big-O call to ranges::size(R)
                        // and the new container supports the reserve function for reserving spaces for elements
                        // Then use reserve to reserve the total number of elements needed
                        if constexpr (sized_range<R> && reservable_container<C>)
                        {
                            c.reserve(ranges::size(r));
                        }

                        ranges::copy(r, container_inserter<range_reference_t<R>>(C));

                    }
                    // Otherwise the construct is ill-formed
                    else
                    {
                        constexpr bool IllFormedCondition = !(constructible_from<C, Args...> && container_insertable<C, range_reference_t<R>>);
                        static_assert(IllFormedCondition, "The reference type of range R is not convertible to the value type of container C");
                    }
                }
                else if constexpr (input_range<range_reference_t<R>>)
                {
                    auto recursive_to = [](auto&& elem)
                    {
                        return to_fn{}.operator()<range_value_t<C>>(AZStd::forward<decltype(elem)>(elem));
                    };
                    operator()<C>(r | views::transform(recursive_to), AZStd::forward<Args>(args)...);
                }
                else
                {
                    constexpr bool IllFormedCondition = !input_range<range_reference_t<R>>;
                    static_assert(IllFormedCondition, "The reference type of range R is not convertible to the value type of container C");
                }
            }

            template<template<class...> class C, class R, class... Args,
                class = enable_if_t<input_range<R>>>
            [[nodiscard]] constexpr auto operator()(R&& r, Args&&... args) const
            {
                return C<decltype(DEDUCE_EXPR<C, R, Args...>())>(AZStd::forward<R>(r), AZStd::forward<Args>(args)...);
            }

            template<class C, class... Args, class = enable_if_t<!view<C>>>
            constexpr auto operator()(Args&&... args) const
            {
                auto to_forwarder = [](auto&& r, auto&&... boundArgs) constexpr
                {
                    return to_fn{}.operator()<C>(AZStd::forward<decltype(r)>(r), AZStd::forward<decltype(boundArgs)>(boundArgs)...);
                };
                return ::AZStd::ranges::views::Internal::range_adaptor_argument_forwarder{ to_forwarder, AZStd::forward<Args>(args)... };
            }
            template<template<class...> class C, class... Args>
            constexpr auto operator()(Args&&... args) const
            {
                auto to_forwarder = [](auto&& r, auto&&... boundArgs) constexpr
                {
                    return to_fn{}.operator()<C>(AZStd::forward<decltype(r)>(r), AZStd::forward<decltype(boundArgs)>(boundArgs)...);
                };
                return ::AZStd::ranges::views::Internal::range_adaptor_argument_forwarder{ to_forwarder, AZStd::forward<Args>(args)... };
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::to_fn to{};
    }
} // namespace AZStd::ranges
