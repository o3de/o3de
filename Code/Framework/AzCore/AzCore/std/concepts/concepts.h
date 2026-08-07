/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <concepts>
#include <iterator>

#include <AzCore/std/concepts/concepts_assignable.h>
#include <AzCore/std/concepts/concepts_constructible.h>
#include <AzCore/std/concepts/concepts_copyable.h>
#include <AzCore/std/concepts/concepts_movable.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/iterator/iterator_primitives.h>
#include <AzCore/std/ranges/swap.h>
#include <AzCore/std/ranges/iter_move.h>
#include <AzCore/std/ranges/iter_swap.h>

#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/common_reference.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_object.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>
#include <AzCore/std/utility/move.h>

namespace AZStd
{
    // alias std::pointer_traits into the AZStd::namespace
    using std::pointer_traits;

    using std::input_iterator_tag;
    using std::output_iterator_tag;
    using std::forward_iterator_tag;
    using std::bidirectional_iterator_tag;
    using std::random_access_iterator_tag;
    using std::contiguous_iterator_tag;
}

namespace AZStd
{
    //! Implements the C++20 to_address function
    //! This obtains the address represented by ptr without forming a reference
    //! to the pointee type
    namespace Internal
    {
        // pointer_traits isn't SFINAE friendly https://cplusplus.github.io/LWG/lwg-active.html#3545
        // working around that by checking if type T has an element_type alias
        template <class T>
        concept pointer_traits_has_to_address =
            has_element_type_v<T>
            && requires { requires !is_void_v<decltype(pointer_traits<T>::to_address(declval<const T&>()))>; };

        // fancy pointer helper
        template <class T>
        concept to_address_fancy_pointer =
            requires { requires !is_void_v<decltype(declval<const T&>().operator->())>; }
            || pointer_traits_has_to_address<T>;

        struct to_address_fn
        {
        public:
            template <class T>
            constexpr T* operator()(T* ptr) const noexcept
            {
                static_assert(!AZStd::is_function_v<T>, "Invoking to_address on a function pointer is not allowed");
                return ptr;
            }

            //! Fancy pointer overload which delegates to using a specialization of pointer_traits<T>::to_address
            //! if that is a well-formed expression, otherwise it returns ptr->operator->()
            //! For example invoking `to_address(AZStd::reverse_iterator<const char*>(char_ptr))`
            //! Returns an element of type const char*
            template <class T>
                requires (!is_pointer_v<T>)
                    && (!is_array_v<T>)
                    && (!is_function_v<T>)
                    && to_address_fancy_pointer<T>
            constexpr auto operator()(const T& ptr) const noexcept
            {
                if constexpr (pointer_traits_has_to_address<T>)
                {
                    return pointer_traits<T>::to_address(ptr);
                }
                else
                {
                    return ptr.operator->();
                }
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::to_address_fn to_address{};
    }
}

namespace AZStd::Internal
{
    template<class T, class U>
    concept different_from = !same_as<remove_cvref_t<T>, remove_cvref_t<U>>;

    template <class It>
    concept is_class_or_enum =
        is_class_v<remove_cvref_t<It>>
        || is_enum_v<remove_cvref_t<It>>;
}

namespace AZStd
{
    using std::common_with;

    using std::derived_from;
}

namespace AZStd
{
    using std::signed_integral;
    using std::unsigned_integral;
}


namespace AZStd::Internal
{
    // boolean-testable concept (exposition only in the C++standard)
    template<class T>
    concept boolean_testable_impl = convertible_to<T, bool>;

    template<class T>
    concept boolean_testable =
        boolean_testable_impl<T>
        && boolean_testable_impl<decltype(!declval<T>())>;

    // weakly comparable ==, !=
    template<class T, class U>
    concept weakly_equality_comparable_with =
        boolean_testable<decltype(declval<const AZStd::remove_reference_t<T>&>() == declval<const AZStd::remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const AZStd::remove_reference_t<T>&>() != declval<const AZStd::remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const AZStd::remove_reference_t<U>&>() == declval<const AZStd::remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const AZStd::remove_reference_t<U>&>() != declval<const AZStd::remove_reference_t<T>&>())>;

    // partially ordered <, >, <=, >=
    template<class T, class U>
    concept partially_ordered_with_impl =
        boolean_testable<decltype(declval<const remove_reference_t<T>&>() < declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() > declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() <= declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() >= declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() < declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() > declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() <= declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() >= declval<const remove_reference_t<T>&>())>;
}

namespace AZStd
{
    using std::equality_comparable;
    using std::equality_comparable_with;
}

namespace AZStd
{
    using std::totally_ordered;
    using std::totally_ordered_with;
}

namespace AZStd
{
    using std::default_initializable;
}

namespace AZStd
{
    using std::semiregular;
    using std::regular;
}

// Iterator Concepts
namespace AZStd::Internal
{
    template <class T>
    concept is_integer_like =
        integral<T>
        && (!same_as<T, bool>);

    template <class T>
    concept is_signed_integer_like = signed_integral<T>;

    template <class T>
    concept weakly_incrementable_impl =
        movable<T>
        && requires(T& t) {
            requires is_signed_integer_like<iter_difference_t<T>>;
            { ++t } -> same_as<T&>;
            t++;
        };
}

namespace AZStd
{
    template <class T>
    concept weakly_incrementable = Internal::weakly_incrementable_impl<T>;
}

namespace AZStd::Internal
{
    template <class T>
    inline constexpr bool input_or_output_iterator_override = false;
}

namespace AZStd
{
    template <class T>
    concept input_or_output_iterator =
        Internal::input_or_output_iterator_override<remove_cvref_t<T>>
        || (!is_void_v<T> && weakly_incrementable<T>);
}

namespace AZStd::Internal
{
    template <class T>
    concept incrementable_impl =
        regular<T>
        && weakly_incrementable<T>
        && requires(T& t) { { t++ } -> same_as<T>; };
}

namespace AZStd
{
    template <class T>
    concept incrementable = Internal::incrementable_impl<T>;
}

namespace AZStd
{
    template<class S, class I>
    concept sentinel_for =
        semiregular<S>
        && input_or_output_iterator<I>
        && Internal::weakly_equality_comparable_with<S, I>;

    template<class S, class I>
    inline constexpr bool disable_sized_sentinel_for = false;
}

namespace AZStd::Internal
{
    template<class S, class I>
    concept sized_sentinel_for_impl =
        sentinel_for<S, I>
        && (!disable_sized_sentinel_for<remove_cv_t<S>, remove_cv_t<I>>)
        && requires(S s, I i)
        {
            { s - i } -> same_as<iter_difference_t<I>>;
            { i - s } -> same_as<iter_difference_t<I>>;
        };
}

namespace AZStd
{
    template<class S, class I>
    concept sized_sentinel_for = Internal::sized_sentinel_for_impl<S, I>;
}

namespace AZStd::Internal
{
    // ITER_TRAITS(I) general concept
    template<class I>
    using ITER_TRAITS = conditional_t<is_primary_template<iterator_traits<I>>, I, iterator_traits<I>>;

    // ITER_CONCEPT(I) general concept
    template<class I>
    concept use_traits_iterator_concept_for_concept =
        requires { typename ITER_TRAITS<I>::iterator_concept; };

    template<class I>
    concept use_traits_iterator_category_for_concept =
        (!use_traits_iterator_concept_for_concept<I>)
        && requires { typename ITER_TRAITS<I>::iterator_category; };

    template<class I>
    concept use_random_access_iterator_tag_for_concept =
        (!use_traits_iterator_concept_for_concept<I>)
        && (!use_traits_iterator_category_for_concept<I>)
        && requires { typename ITER_TRAITS<I>; };

    template<class I, class = void>
    struct iter_concept;

    template<use_traits_iterator_concept_for_concept I>
    struct iter_concept<I, void>
    {
        using type = typename ITER_TRAITS<I>::iterator_concept;
    };
    template<use_traits_iterator_category_for_concept I>
    struct iter_concept<I, void>
    {
        using type = typename ITER_TRAITS<I>::iterator_category;
    };

    template<use_random_access_iterator_tag_for_concept I>
    struct iter_concept<I, void>
    {
        using type = random_access_iterator_tag;
    };
    template<class I>
    using iter_concept_t = typename iter_concept<I>::type;
}



namespace AZStd::Internal
{
    template<class I>
    concept input_iterator_impl =
        input_or_output_iterator<I>
        && indirectly_readable<I>
        && requires { requires derived_from<iter_concept_t<I>, input_iterator_tag>; };
}

namespace AZStd
{
    template<class I>
    concept input_iterator = Internal::input_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class T>
    concept output_iterator_impl =
        input_or_output_iterator<I>
        && indirectly_writable<I, T>
        && requires(I& i) { *i++ = AZStd::declval<T>(); };
}

namespace AZStd
{
    template<class I, class T>
    concept output_iterator = Internal::output_iterator_impl<I, T>;
}

namespace AZStd::Internal
{
    template<class I>
    concept forward_iterator_impl =
        input_iterator<I>
        && incrementable<I>
        && sentinel_for<I, I>
        && requires { requires derived_from<Internal::iter_concept_t<I>, forward_iterator_tag>; };
}

namespace AZStd
{
    template<class I>
    concept forward_iterator = Internal::forward_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I>
    concept bidirectional_iterator_impl =
        forward_iterator<I>
        && requires(I& i) {
            requires derived_from<iter_concept_t<I>, bidirectional_iterator_tag>;
            { --i } -> same_as<I&>;
            { i-- } -> same_as<I>;
        };
}

namespace AZStd
{
    template<class I>
    concept bidirectional_iterator = Internal::bidirectional_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I>
    concept random_access_iterator_impl =
        bidirectional_iterator<I>
        && totally_ordered<I>
        && sized_sentinel_for<I, I>
        && requires(I i, const I j, const iter_difference_t<I> n) {
            requires derived_from<iter_concept_t<I>, random_access_iterator_tag>;
            { i += n } -> same_as<I&>;
            { j + n } -> same_as<I>;
            { n + j } -> same_as<I>;
            { i -= n } -> same_as<I&>;
            { j - n } -> same_as<I>;
            { j[n] } -> same_as<iter_reference_t<I>>;
        };
}

namespace AZStd
{
    template<class I>
    concept random_access_iterator = Internal::random_access_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I>
    concept contiguous_iterator_impl =
        random_access_iterator<I>
        && indirectly_readable<I>
        && requires {
            requires derived_from<iter_concept_t<I>, contiguous_iterator_tag>;
            requires is_lvalue_reference_v<iter_reference_t<I>>;
            requires same_as<iter_value_t<I>, remove_cvref_t<iter_reference_t<I>>>;
            requires same_as<decltype(to_address(declval<const I&>())), add_pointer_t<iter_reference_t<I>>>;
        };
}

namespace AZStd
{
    template<class I>
    concept contiguous_iterator = Internal::contiguous_iterator_impl<I>;
}

namespace AZStd
{
    using std::predicate;
    using std::relation;
    using std::equivalence_relation;
    using std::strict_weak_order;
}

namespace AZStd
{
    using std::indirectly_unary_invocable;
    using std::indirectly_regular_unary_invocable;
    using std::indirect_unary_predicate;
    using std::indirect_binary_predicate;
    using std::indirect_equivalence_relation;
    using std::indirect_strict_weak_order;

    namespace Internal
    {
        template<bool Invocable, class F, class... Is>
        struct indirect_result;
        template<class F, class... Is>
        struct indirect_result<true, F, Is...>
        {
            using type = invoke_result_t<F, iter_reference_t<Is>...>;
        };
    }
    template<class F, class... Is>
    using indirect_result_t = typename Internal::indirect_result<
            ((indirectly_readable<Is> && ...) && AZStd::invocable<F, iter_reference_t<Is>...>),
        F, Is...>::type;

    // https://eel.is/c++draft/iterators#projected
    using std::projected;
}
