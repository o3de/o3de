/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>
#include <AzCore/std/utility/move.h>

namespace AZStd
{
    // alias std::pointer_traits into the AZStd::namespace
    using std::pointer_traits;

    // Alias re-declarations from iterator.h
    /// Identifying tag for input iterators.
    using input_iterator_tag = std::input_iterator_tag;
    /// Identifying tag for output iterators.
    using output_iterator_tag = std::output_iterator_tag;
    /// Identifying tag for forward iterators.
    using forward_iterator_tag = std::forward_iterator_tag;
    /// Identifying tag for bidirectional iterators.
    using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
    /// Identifying tag for random-access iterators.
    using random_access_iterator_tag = std::random_access_iterator_tag;
    /// Identifying tag for contagious iterators
    struct contiguous_iterator_tag;
}

namespace AZStd::Internal
{
    // Variadic template which maps types to true For SFINAE
    template <class... Args>
    using sfinae_trigger = true_type;
    template <class... Args>
    constexpr bool sfinae_trigger_v = true;
}

namespace AZStd
{
    //! Implements the C++20 to_address function
    //! This obtains the address represented by ptr without forming a reference
    //! to the pointee type
    namespace Internal
    {
        template <class T, class = void>
        constexpr bool pointer_traits_has_to_address_v = false;

        // pointer_traits isn't SFINAE friendly https://cplusplus.github.io/LWG/lwg-active.html#3545
        // working around that by checking if type T has an element_type alias
        template <class T>
        constexpr bool pointer_traits_has_to_address_v<T, enable_if_t<has_element_type_v<T>>>
            = !is_void_v<decltype(pointer_traits<T>::to_address(declval<const T&>()))>;

        // fancy pointer helper
        template <class T, class = void>
        inline constexpr bool to_address_fancy_pointer_v = false;

        template <class T>
        inline constexpr bool to_address_fancy_pointer_v<T, enable_if_t<
            !is_void_v<decltype(declval<const T&>().operator->())>>> = true;

        template <class T>
        inline constexpr bool to_address_fancy_pointer_v<T, enable_if_t<
            pointer_traits_has_to_address_v<T>>> = true;

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
            template <class T, class = enable_if_t<conjunction_v<
                bool_constant<!is_pointer_v<T>>,
                bool_constant<!is_array_v<T>>,
                bool_constant<!is_function_v<T>>,
                bool_constant<to_address_fancy_pointer_v<T>>
                >>>
            constexpr auto operator()(const T& ptr) const noexcept
            {
                if constexpr (pointer_traits_has_to_address_v<T>)
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
    /*concept*/ constexpr bool different_from = !same_as<remove_cvref_t<T>, remove_cvref_t<U>>;

    template <class It, class = void>
    constexpr bool is_class_or_enum = false;
    template <class It>
    constexpr bool is_class_or_enum<It, enable_if_t<disjunction_v<
        is_class<remove_cvref_t<It>>, is_enum<remove_cvref_t<It>> >>> = true;


    template<class T, class U, class = void>
    constexpr bool common_with_impl = false;
    template<class T, class U>
    constexpr bool common_with_impl<T, U, enable_if_t<conjunction_v<
        bool_constant<same_as<common_type_t<T, U>, common_type_t<U, T>>>,
        sfinae_trigger<decltype(static_cast<common_type_t<T, U>>(declval<T>()))>,
        sfinae_trigger<decltype(static_cast<common_type_t<T, U>>(declval<U>()))>,
        bool_constant<common_reference_with<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>>>,
        bool_constant<common_reference_with<add_lvalue_reference_t<common_type_t<T, U>>,
            common_reference_t<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>>>>
        >>> = true;
}

namespace AZStd
{
    template<class T, class U>
    /*concept*/ constexpr bool common_with = Internal::common_with_impl<T, U>;


    template<class Derived, class Base>
    /*concept*/ constexpr bool derived_from = conjunction_v<is_base_of<Base, Derived>,
        is_convertible<const volatile Derived*, const volatile Base*>>;
}

namespace AZStd
{
    template <class T>
    /*concept*/ constexpr bool signed_integral = conjunction_v<bool_constant<integral<T>>, is_signed<T>>;
    template <class T>
    /*concept*/ constexpr bool unsigned_integral = conjunction_v<bool_constant<integral<T>>, bool_constant<!signed_integral<T>>>;
}


namespace AZStd::Internal
{
    // boolean-testable concept (exposition only in the C++standard)
    template<class T>
    constexpr bool boolean_testable_impl = convertible_to<T, bool>;

    template<class T, class = void>
    constexpr bool boolean_testable = false;
    template<class T>
    constexpr bool boolean_testable<T, enable_if_t<conjunction_v<bool_constant<boolean_testable_impl<T>>,
        bool_constant<boolean_testable_impl<decltype(!declval<T>())>> >>> = true;

    // weakly comparable ==, !=
    template<class T, class U, class = void>
    constexpr bool weakly_equality_comparable_with = false;
    template<class T, class U>
    constexpr bool weakly_equality_comparable_with<T, U, enable_if_t<conjunction_v<
        bool_constant<boolean_testable<decltype(declval<const AZStd::remove_reference_t<T>&>() == declval<const AZStd::remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const AZStd::remove_reference_t<T>&>() != declval<const AZStd::remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const AZStd::remove_reference_t<U>&>() == declval<const AZStd::remove_reference_t<T>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const AZStd::remove_reference_t<U>&>() != declval<const AZStd::remove_reference_t<T>&>())>>
        >>> = true;

    // partially ordered <, >, <=, >=
    template<class, class U, class = void>
    constexpr bool partially_ordered_with_impl = false;
    template<class T, class U>
    constexpr bool partially_ordered_with_impl<T, U, enable_if_t<conjunction_v<
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<T>&>() < declval<const remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<T>&>() > declval<const remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<T>&>() <= declval<const remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<T>&>() >= declval<const remove_reference_t<U>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<U>&>() < declval<const remove_reference_t<T>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<U>&>() > declval<const remove_reference_t<T>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<U>&>() <= declval<const remove_reference_t<T>&>())>>,
        bool_constant<boolean_testable<decltype(declval<const remove_reference_t<U>&>() >= declval<const remove_reference_t<T>&>())>>
        >>> = true;
}

namespace AZStd
{
    template<class T>
    /*concept*/ constexpr bool equality_comparable = Internal::weakly_equality_comparable_with<T, T>;
}

namespace AZStd::Internal
{
    // equally_comparable + partially ordered
    template<class, class U, class = void>
    constexpr bool equally_comparable_with_impl = false;
    template<class T, class U>
    constexpr bool equally_comparable_with_impl<T, U, enable_if_t<conjunction_v<
        bool_constant<equality_comparable<T>>,
        bool_constant<equality_comparable<U>>,
        bool_constant<common_reference_with<const remove_reference_t<T>&, const remove_reference_t<U>&>>,
        bool_constant<equality_comparable<common_reference_t<const remove_reference_t<T>&, const remove_reference_t<U>&>>>,
        bool_constant<Internal::weakly_equality_comparable_with<T, U>>
        >>> = true;
}

namespace AZStd
{
    template<class T, class U>
    /*concept*/ constexpr bool equality_comparable_with = Internal::equally_comparable_with_impl<T, U>;

    template<class T, class U>
    /*concept*/ constexpr bool partially_ordered_with = Internal::partially_ordered_with_impl<T, U>;

    template<class T>
    /*concept*/ constexpr bool totally_ordered = equality_comparable<T> && partially_ordered_with<T, T>;
}

namespace AZStd::Internal
{
    // equally_comparable + partially ordered
    template<class, class U, class = void>
    constexpr bool totally_ordered_with_impl = false;
    template<class T, class U>
    constexpr bool totally_ordered_with_impl<T, U, enable_if_t<
        totally_ordered<T> &&
        totally_ordered<U> &&
        equality_comparable_with<T, U> &&
        totally_ordered<common_reference_t<const remove_reference_t<T>&, const remove_reference_t<U>&>> &&
        partially_ordered_with<T, U>
        >> = true;
}

namespace AZStd
{
    template<class T, class U>
    /*concept*/ constexpr bool totally_ordered_with = Internal::totally_ordered_with_impl<T, U>;
}

namespace AZStd::Internal
{
    template<class T, class = void>
    inline constexpr bool is_default_initializable = false;
    template<class T>
    inline constexpr bool is_default_initializable<T, void_t<decltype(::new T)>> = true;

    template<class T, class = void>
    constexpr bool default_initializable_impl = false;
    template<class T>
    constexpr bool default_initializable_impl<T, enable_if_t<conjunction_v<
        bool_constant<constructible_from<T>>,
        sfinae_trigger<decltype(T{})>,
        bool_constant<is_default_initializable<T>>
        >>> = true;
}

namespace AZStd
{
    // default_initializable
    template<class T>
    /*concept*/ constexpr bool default_initializable = Internal::default_initializable_impl<T>;
}


namespace AZStd
{
    // semiregular
    template<class T>
    /*concept*/ constexpr bool semiregular = conjunction_v<bool_constant<copyable<T>>, bool_constant<default_initializable<T>>>;

    // regular
    template<class T>
    /*concept*/ constexpr bool regular = conjunction_v<bool_constant<semiregular<T>>, bool_constant<equality_comparable<T>>>;
}

// Iterator Concepts
namespace AZStd::Internal
{
    template <class T>
    constexpr bool is_integer_like = conjunction_v<bool_constant<integral<T>>, bool_constant<!same_as<T, bool>>>;

    template <class T>
    constexpr bool is_signed_integer_like = signed_integral<T>;

    template <class T, class = void>
    constexpr bool weakly_incrementable_impl = false;
    template <class T>
    constexpr bool weakly_incrementable_impl<T, enable_if_t<conjunction_v<
        bool_constant<movable<T>>,
        bool_constant<is_signed_integer_like<iter_difference_t<T>>>,
        bool_constant<same_as<decltype(++declval<T&>()), T&>>,
        sfinae_trigger<decltype(declval<T&>()++)>
        >>> = true;
}

namespace AZStd
{
    // models weakly_incrementable concept
    template <class T>
    /*concept*/ constexpr bool weakly_incrementable = Internal::weakly_incrementable_impl<T>;

    // models input_or_output_iterator concept
    template <class T>
    /*concept*/ constexpr bool input_or_output_iterator = conjunction_v<bool_constant<!is_void_v<T>>,
        bool_constant<weakly_incrementable<T>>>;
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool incrementable_impl = false;
    template <class T>
    constexpr bool incrementable_impl<T, enable_if_t<conjunction_v<
        bool_constant<regular<T>>,
        bool_constant<weakly_incrementable<T>>,
        bool_constant<same_as<decltype(declval<T&>()++), T>>
        >>> = true;
}

namespace AZStd
{
    template <class T>
    /*concept*/ constexpr bool incrementable = Internal::incrementable_impl<T>;
}

namespace AZStd
{
    template<class S, class I>
    /*concept*/ constexpr bool sentinel_for = conjunction_v<
        bool_constant<semiregular<S>>,
        bool_constant<input_or_output_iterator<I>>,
        bool_constant<Internal::weakly_equality_comparable_with<S, I>>>;

    template<class S, class I>
    inline constexpr bool disable_sized_sentinel_for = false;
}

namespace AZStd::Internal
{
    template<class S, class I, class = void>
    /*concept*/ constexpr bool sized_sentinel_for_impl = false;
    template<class S, class I>
    /*concept*/ constexpr bool sized_sentinel_for_impl<S, I, enable_if_t<conjunction_v<
        bool_constant<sentinel_for<S, I>>,
        bool_constant<!disable_sized_sentinel_for<remove_cv_t<S>, remove_cv_t<I>>>,
        bool_constant<same_as<decltype(declval<S>() - declval<I>()), iter_difference_t<I>>>,
        bool_constant<same_as<decltype(declval<I>() - declval<S>()), iter_difference_t<I>>>
        >>> = true;
}

namespace AZStd
{
    template<class S, class I>
    /*concept*/ constexpr bool sized_sentinel_for = Internal::sized_sentinel_for_impl<S, I>;
}

namespace AZStd::Internal
{
    // ITER_TRAITS(I) general concept
    template<class I>
    using ITER_TRAITS = conditional_t<is_primary_template_v<iterator_traits<I>>, I, iterator_traits<I>>;

    // ITER_CONCEPT(I) general concept
    template<class I, class = void>
    constexpr bool use_traits_iterator_concept_for_concept = false;
    template<class I>
    constexpr bool use_traits_iterator_concept_for_concept<I, void_t<typename ITER_TRAITS<I>::iterator_concept>> = true;

    template<class I, class = void>
    constexpr bool use_traits_iterator_category_for_concept = false;
    template<class I>
    constexpr bool use_traits_iterator_category_for_concept<I, enable_if_t<conjunction_v<
        sfinae_trigger<typename ITER_TRAITS<I>::iterator_category>,
        bool_constant<!use_traits_iterator_concept_for_concept<I>> >>> = true;

    template<class I, class = void>
    constexpr bool use_random_access_iterator_tag_for_concept = false;
    template<class I>
    constexpr bool use_random_access_iterator_tag_for_concept<I, enable_if_t<conjunction_v<
        sfinae_trigger<ITER_TRAITS<I>>,
        bool_constant<!use_traits_iterator_concept_for_concept<I>>,
        bool_constant<!use_traits_iterator_category_for_concept<I>> >>> = true;

    template<class I, class = void>
    struct iter_concept;

    template<class I>
    struct iter_concept<I, enable_if_t<use_traits_iterator_concept_for_concept<I>>>
    {
        using type = typename ITER_TRAITS<I>::iterator_concept;
    };
    template<class I>
    struct iter_concept<I, enable_if_t<use_traits_iterator_category_for_concept<I>>>
    {
        using type = typename ITER_TRAITS<I>::iterator_category;
    };

    template<class I>
    struct iter_concept<I, enable_if_t<use_random_access_iterator_tag_for_concept<I>>>
    {
        using type = random_access_iterator_tag;
    };
    template<class I>
    using iter_concept_t = typename iter_concept<I>::type;
}



namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool input_iterator_impl = false;
    template<class I>
    constexpr bool input_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<input_or_output_iterator<I>>,
        bool_constant<derived_from<iter_concept_t<I>, input_iterator_tag>>,
        bool_constant<indirectly_readable<I>>
        >>> = true;
}

namespace AZStd
{
    // input iterator
    template<class I>
    /*concept*/ constexpr bool input_iterator = Internal::input_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class T, class = void>
    constexpr bool output_iterator_impl = false;
    template<class I, class T>
    constexpr bool output_iterator_impl<I, T, enable_if_t<conjunction_v<
        bool_constant<input_or_output_iterator<I>>,
        bool_constant<indirectly_writable<I, T>>,
        sfinae_trigger<decltype(*declval<I&>()++ = AZStd::declval<T>())>
        >>> = true;
}

namespace AZStd
{
    // output iterator
    template<class I, class T>
    /*concept*/ constexpr bool output_iterator = Internal::output_iterator_impl<I, T>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool forward_iterator_impl = false;
    template<class I>
    constexpr bool forward_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<input_iterator<I>>,
        bool_constant<derived_from<Internal::iter_concept_t<I>, forward_iterator_tag>>,
        bool_constant<incrementable<I>>,
        bool_constant<sentinel_for<I, I>> >>> = true;
}

namespace AZStd
{
    // forward_iterator
    template<class I>
    /*concept*/ constexpr bool forward_iterator = Internal::forward_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool bidirectional_iterator_impl = false;
    template<class I>
    constexpr bool bidirectional_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<forward_iterator<I>>,
        bool_constant<derived_from<iter_concept_t<I>, bidirectional_iterator_tag>>,
        bool_constant<same_as<decltype(--declval<I&>()), I&>>,
        bool_constant<same_as<decltype(declval<I&>()--), I>> >>> = true;
}

namespace AZStd
{
    // bidirectional iterator
    template<class I>
    /*concept*/ constexpr bool bidirectional_iterator = Internal::bidirectional_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool random_access_iterator_impl = false;
    template<class I>
    constexpr bool random_access_iterator_impl<I, enable_if_t < conjunction_v <
        bool_constant<bidirectional_iterator<I>>,
        bool_constant<derived_from<iter_concept_t<I>, random_access_iterator_tag>>,
        bool_constant<totally_ordered<I>>,
        bool_constant<sized_sentinel_for<I, I>>,
        bool_constant<same_as<decltype(declval<I&>() += declval<const iter_difference_t<I>>()), I&>>,
        bool_constant<same_as<decltype(declval<const I>() + declval<const iter_difference_t<I>>()), I>>,
        bool_constant<same_as<decltype(declval<iter_difference_t<I>>() + declval<const I>()), I>>,
        bool_constant<same_as<decltype(declval<I&>() -= declval<const iter_difference_t<I>>()), I&>>,
        bool_constant<same_as<decltype(declval<const I>() - declval<const iter_difference_t<I>>()), I>>,
        bool_constant<same_as<decltype(declval<const I&>()[declval<iter_difference_t<I>>()]), iter_reference_t<I>>>
        >>> = true;
}

namespace AZStd
{
    template<class I>
    /*concept*/ constexpr bool random_access_iterator = Internal::random_access_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool contiguous_iterator_impl = false;
    template<class I>
    constexpr bool contiguous_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<random_access_iterator<I>>,
        bool_constant<derived_from<iter_concept_t<I>, contiguous_iterator_tag>>,
        bool_constant<is_lvalue_reference_v<iter_reference_t<I>>>,
        bool_constant<indirectly_readable<I>>,
        bool_constant<same_as<iter_value_t<I>, remove_cvref_t<iter_reference_t<I>>>>,
        bool_constant<same_as<decltype(to_address(declval<const I&>())), add_pointer_t<iter_reference_t<I>>>>
        >>> = true;
}

namespace AZStd
{
    // contiguous iterator
    template<class I>
    /*concept*/ constexpr bool contiguous_iterator = Internal::contiguous_iterator_impl<I>;
}

namespace AZStd::Internal
{
    // cpp17 iterator concepts
    // https://eel.is/c++draft/iterator.traits#2
    template<class I, class = void>
    constexpr bool cpp17_iterator_impl = false;
    template<class I>
    constexpr bool cpp17_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<can_reference<decltype(*declval<I&>())>>,
        bool_constant<same_as<decltype(++declval<I&>()), I&>>,
        bool_constant<can_reference<decltype(*declval<I&>()++)>>,
        bool_constant<copyable<I>>
        >>> = true;
}

namespace AZStd
{
    // cpp17-iterator concept
    template<class I>
    /*concept*/ constexpr bool cpp17_iterator = Internal::cpp17_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool cpp17_input_iterator_impl = false;
    template<class I>
    constexpr bool cpp17_input_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<cpp17_iterator<I>>,
        bool_constant<equality_comparable<I>>,
        sfinae_trigger<typename incrementable_traits<I>::difference_type>,
        sfinae_trigger<typename indirectly_readable_traits<I>::value_type>,
        sfinae_trigger<typename incrementable_traits<I>::difference_type>,
        sfinae_trigger<common_reference_t<iter_reference_t<I>&&,
            typename indirectly_readable_traits<I>::value_type&>>,
        sfinae_trigger<common_reference_t<decltype(*declval<I&>()++)&&,
            typename indirectly_readable_traits<I>::value_type&>>,
        bool_constant<signed_integral<typename incrementable_traits<I>::difference_type>>
        >>> = true;
}

namespace AZStd
{
    // cpp17-input-iterator concept
    template<class I>
    /*concept*/ constexpr bool cpp17_input_iterator = Internal::cpp17_input_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool cpp17_forward_iterator_impl = false;
    template<class I>
    constexpr bool cpp17_forward_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<cpp17_input_iterator<I>>,
        bool_constant<constructible_from<I>>,
        bool_constant<is_lvalue_reference_v<iter_reference_t<I>>>,
        bool_constant<same_as<remove_cvref_t<iter_reference_t<I>>, typename indirectly_readable_traits<I>::value_type>>,
        bool_constant<convertible_to<decltype(declval<I&>()++), const I&>>,
        bool_constant<same_as<decltype(*declval<I&>()++), iter_reference_t<I>>>
        >>> = true;
}

namespace AZStd
{
    // cpp17-forward-iterator concept
    template<class I>
    /*concept*/ constexpr bool cpp17_forward_iterator = Internal::cpp17_forward_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool cpp17_bidirectional_iterator_impl = false;
    template<class I>
    constexpr bool cpp17_bidirectional_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<cpp17_forward_iterator<I>>,
        bool_constant<same_as<decltype(--declval<I&>()), I&>>,
        bool_constant<convertible_to<decltype(declval<I&>()--), const I&>>,
        bool_constant<same_as<decltype(*declval<I&>()--), iter_reference_t<I>>>
        >>> = true;
}

namespace AZStd
{
    // cpp17-bidirectional-iterator concept
    template<class I>
    /*concept*/ constexpr bool cpp17_bidirectional_iterator = Internal::cpp17_bidirectional_iterator_impl<I>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool cpp17_random_access_iterator_impl = false;
    template<class I>
    constexpr bool cpp17_random_access_iterator_impl<I, enable_if_t<conjunction_v<
        bool_constant<cpp17_bidirectional_iterator<I>>,
        bool_constant<totally_ordered<I>>,
        bool_constant<same_as<decltype(declval<I&>() += declval<typename incrementable_traits<I>::difference_type>()), I&>>,
        bool_constant<same_as<decltype(declval<I&>() -= declval<typename incrementable_traits<I>::difference_type>()), I&>>,
        bool_constant<same_as<decltype(declval<I>() + declval<typename incrementable_traits<I>::difference_type>()), I>>,
        bool_constant<same_as<decltype(declval<typename incrementable_traits<I>::difference_type>() + declval<I>()), I>>,
        bool_constant<same_as<decltype(declval<I>() - declval<typename incrementable_traits<I>::difference_type>()), I>>,
        bool_constant<same_as<decltype(declval<I>() - declval<I>()), typename incrementable_traits<I>::difference_type>>,
        bool_constant<same_as<decltype(declval<I&>()[declval<typename incrementable_traits<I>::difference_type>()]), iter_reference_t<I>>>
        >>> = true;
}

namespace AZStd
{
    // cpp17-random_access-iterator concept
    template<class I>
    /*concept*/ constexpr bool cpp17_random_access_iterator = Internal::cpp17_random_access_iterator_impl<I>;
}

namespace AZStd::Internal
{
    // models the predicate concept
    template <bool, class F, class... Args>
    constexpr bool predicate_impl = false;
    template <class F, class... Args>
    constexpr bool predicate_impl<true, F, Args...> = Internal::boolean_testable<invoke_result_t<F, Args...>>;
}

namespace AZStd
{
    // models the predicate concept
    template <class F, class... Args>
    /*concept*/ constexpr bool predicate = Internal::predicate_impl<regular_invocable<F, Args...>, F, Args...>;

    // models the relation concept
    template <class R, class T, class U>
    /*concept*/ constexpr bool relation = conjunction_v<
        bool_constant<predicate<R, T, T>>,
        bool_constant<predicate<R, U, U>>,
        bool_constant<predicate<R, T, U>>,
        bool_constant<predicate<R, U, T>>>;

    // models the equivalence_relation concept
    // Note: semantically this is different than relation
    // since it can't be enforced at compile time
    template <class R, class T, class U>
    /*concept*/ constexpr bool equivalence_relation = relation<R, T, U>;

    // models the strict_weak_order concept
    // Note: semantically this is different than relation
    // since it can't be enforced at compile time
    template <class R, class T, class U>
    /*concept*/ constexpr bool strict_weak_order = relation<R, T, U>;
}

namespace AZStd
{
    // https://eel.is/c++draft/iterators#indirectcallable.indirectinvocable
    template<class F, class I, class = void>
    /*concept*/ constexpr bool indirectly_unary_invocable = false;
    template<class F, class I>
    /*concept*/ constexpr bool indirectly_unary_invocable<F, I, enable_if_t<conjunction_v<
        bool_constant<indirectly_readable<I>>,
        bool_constant<copy_constructible<F>>,
        bool_constant<invocable<F&, iter_value_t<I>&>>,
        bool_constant<invocable<F&, iter_reference_t<I>>>,
        bool_constant<invocable<F&, iter_common_reference_t<I>>>,
        bool_constant<common_reference_with<invoke_result_t<F&, iter_value_t<I>&>, invoke_result_t<F&, iter_reference_t<I>>>>
        >>> = true;

    template<class F, class I, class = void>
    /*concept*/ constexpr bool indirectly_regular_unary_invocable = false;
    template<class F, class I>
    /*concept*/ constexpr bool indirectly_regular_unary_invocable<
        F,
        I,
        enable_if_t<conjunction_v<
            bool_constant<indirectly_readable<I>>,
            bool_constant<copy_constructible<F>>,
            bool_constant<regular_invocable<F&, iter_value_t<I>&>>,
            bool_constant<regular_invocable<F&, iter_reference_t<I>>>,
            bool_constant<regular_invocable<F&, iter_common_reference_t<I>>>,
            bool_constant<common_reference_with<invoke_result_t<F&, iter_value_t<I>&>, invoke_result_t<F&, iter_reference_t<I>>>>>>> = true;

    template<class F, class I, class = void>
    /*concept*/ constexpr bool indirect_unary_predicate = false;
    template<class F, class I>
    /*concept*/ constexpr bool indirect_unary_predicate<
        F,
        I,
        enable_if_t<conjunction_v<
            bool_constant<indirectly_readable<I>>,
            bool_constant<copy_constructible<F>>,
            bool_constant<predicate<F&, iter_value_t<I>&>>,
            bool_constant<predicate<F&, iter_reference_t<I>>>,
            bool_constant<predicate<F&, iter_common_reference_t<I>>>>>> = true;

    template<class F, class I1, class I2, class = void>
    /*concept*/ constexpr bool indirect_binary_predicate = false;
    template<class F, class I1, class I2>
    /*concept*/ constexpr bool indirect_binary_predicate<
        F,
        I1,
        I2,
        enable_if_t<conjunction_v<
            bool_constant<indirectly_readable<I1>>,
            bool_constant<indirectly_readable<I2>>,
            bool_constant<copy_constructible<F>>,
            bool_constant<predicate<F&, iter_value_t<I1>&, iter_value_t<I2>&>>,
            bool_constant<predicate<F&, iter_value_t<I1>&, iter_reference_t<I2>>>,
            bool_constant<predicate<F&, iter_reference_t<I1>, iter_value_t<I2>&>>,
            bool_constant<predicate<F&, iter_reference_t<I1>, iter_reference_t<I2>>>,
            bool_constant<predicate<F&, iter_common_reference_t<I1>, iter_common_reference_t<I2>>>>>> = true;

    template<class F, class I1, class I2 = I1, class = void>
    /*concept*/ constexpr bool indirect_equivalence_relation = false;
    template<class F, class I1, class I2>
    /*concept*/ constexpr bool indirect_equivalence_relation<
        F,
        I1,
        I2,
        enable_if_t<conjunction_v<
            bool_constant<indirectly_readable<I1>>,
            bool_constant<indirectly_readable<I2>>,
            bool_constant<copy_constructible<F>>,
            bool_constant<equivalence_relation<F&, iter_value_t<I1>&, iter_value_t<I2>&>>,
            bool_constant<equivalence_relation<F&, iter_value_t<I1>&, iter_reference_t<I2>>>,
            bool_constant<equivalence_relation<F&, iter_reference_t<I1>, iter_value_t<I2>&>>,
            bool_constant<equivalence_relation<F&, iter_reference_t<I1>, iter_reference_t<I2>>>,
            bool_constant<equivalence_relation<F&, iter_common_reference_t<I1>, iter_common_reference_t<I2>>>>>> = true;

    template<class F, class I1, class I2 = I1, class = void>
    /*concept*/ constexpr bool indirect_strict_weak_order = false;
    template<class F, class I1, class I2>
    /*concept*/ constexpr bool indirect_strict_weak_order<
        F,
        I1,
        I2,
        enable_if_t<conjunction_v<
            bool_constant<indirectly_readable<I1>>,
            bool_constant<indirectly_readable<I2>>,
            bool_constant<copy_constructible<F>>,
            bool_constant<strict_weak_order<F&, iter_value_t<I1>&, iter_value_t<I2>&>>,
            bool_constant<strict_weak_order<F&, iter_value_t<I1>&, iter_reference_t<I2>>>,
            bool_constant<strict_weak_order<F&, iter_reference_t<I1>, iter_value_t<I2>&>>,
            bool_constant<strict_weak_order<F&, iter_reference_t<I1>, iter_reference_t<I2>>>,
            bool_constant<strict_weak_order<F&, iter_common_reference_t<I1>, iter_common_reference_t<I2>>>>>> = true;

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
    using indirect_result_t = typename Internal::indirect_result<conjunction_v<
            bool_constant<indirectly_readable<Is>>...,
            bool_constant<AZStd::invocable<F, iter_reference_t<Is>...>>>,
        F, Is...>::type;

    // https://eel.is/c++draft/iterators#projected
    template<class I, class Proj, class = void>
    struct projected;

    template<class I, class Proj>
    struct projected<
        I,
        Proj,
        enable_if_t<conjunction_v<bool_constant<indirectly_readable<I>>, bool_constant<indirectly_regular_unary_invocable<Proj, I>>>>>
    {
        using value_type = remove_cvref_t<indirect_result_t<Proj&, I>>;
        indirect_result_t<Proj&, I> operator*() const; // not defined
    };

    template <class I, class Proj>
    struct incrementable_traits<projected<I, Proj>, enable_if_t<weakly_incrementable<I>>>
    {
        using difference_type = iter_difference_t<I>;
    };
}
