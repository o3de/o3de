/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/invoke.h>
#include <AzCore/std/iterator/iterator_primitives.h>
#include <AzCore/std/ranges/iter_move.h>

#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/common_reference.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_floating_point.h>
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
    template <typename T, typename = void>
    constexpr bool pointer_traits_has_to_address_v = false;

    template <typename T>
    constexpr bool pointer_traits_has_to_address_v<T, enable_if_t<
        is_void_v<void_t<decltype(pointer_traits<T>::to_address(declval<const T&>()))>>> > = true;


    // pointer_traits isn't SFINAE friendly https://cplusplus.github.io/LWG/lwg-active.html#3545
    // So working around that by checking if type T has an element_type alias
    template <typename T, typename = void>
    constexpr bool pointer_traits_valid_and_has_to_address_v = false;
    template <typename T>
    constexpr bool pointer_traits_valid_and_has_to_address_v<T, enable_if_t<has_element_type_v<T>> >
        = pointer_traits_has_to_address_v<T>;
}

namespace AZStd
{
    //! Implements the C++20 to_address function
    //! This obtains the address represented by ptr without forming a reference
    //! to the pointee type
    template <typename T>
    constexpr T* to_address(T* ptr) noexcept
    {
        static_assert(!AZStd::is_function_v<T>, "Invoking to address on a function pointer is not allowed");
        return ptr;
    }
    //! Fancy pointer overload which delegates to using a specialization of pointer_traits<T>::to_address
    //! if that is a well-formed expression, otherwise it returns ptr->operator->()
    //! For example invoking `to_address(AZStd::reverse_iterator<const char*>(char_ptr))`
    //! Returns an element of type const char*
    template <typename T>
    constexpr auto to_address(const T& ptr) noexcept
    {
        if constexpr (AZStd::Internal::pointer_traits_valid_and_has_to_address_v<T>)
        {
            return pointer_traits<T>::to_address(ptr);
        }
        else
        {
            return to_address(ptr.operator->());
        }
    }
}

namespace AZStd::Internal
{
    // Variadic template which maps types to true For SFINAE
    template <class... Args>
    constexpr bool sfinae_trigger_v = true;

    template <class It, class = void>
    constexpr bool is_class_or_enum = false;
    template <class It>
    constexpr bool is_class_or_enum<It, enable_if_t<
        (is_class_v<remove_cvref_t<It>> || is_enum_v<remove_cvref_t<It>>)>> = true;

    template<class LHS, class RHS, class = void>
    constexpr bool assignable_from_impl = false;
    template<class LHS, class RHS>
    constexpr bool assignable_from_impl<LHS, RHS, enable_if_t<is_lvalue_reference_v<LHS>
        && common_reference_with<const remove_reference_t<LHS>&, const remove_reference_t<RHS>&>
        && same_as<decltype(declval<LHS>() = declval<RHS>()), LHS> >> = true;


    template<class T, class U, class = void>
    constexpr bool common_with_impl = false;
    template<class T, class U>
    constexpr bool common_with_impl<T, U, enable_if_t<
        same_as<common_type_t<T, U>, common_type_t<U, T>>
        && sfinae_trigger_v<decltype(static_cast<common_type_t<T, U>>(declval<T>()))>
        && sfinae_trigger_v<decltype(static_cast<common_type_t<T, U>>(declval<U>()))>
        && common_reference_with<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>>
        && common_reference_with<add_lvalue_reference_t<common_type_t<T, U>>, common_reference_t<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>>>
        >> = true;
}

namespace AZStd
{
    template<class T, class U>
    /*concept*/ constexpr bool common_with = Internal::common_with_impl<T, U>;

    template<class LHS, class RHS>
    /*concept*/ constexpr bool assignable_from = Internal::assignable_from_impl<LHS, RHS>;

    template<class T, class... Args>
    /*concept*/ constexpr bool constructible_from = destructible<T> && is_constructible_v<T, Args...>;

    template<class T>
    /*concept*/ constexpr bool move_constructible = constructible_from<T, T> && convertible_to<T, T>;

    template<class Derived, class Base>
    /*concept*/ constexpr bool derived_from = is_base_of_v<Base, Derived> && is_convertible_v<const volatile Derived*, const volatile Base*>;
}

namespace AZStd::ranges::Internal
{
    template <class T, class U, class = void>
    constexpr bool is_class_or_enum_with_swap_adl = false;
    template <class T, class U>
    constexpr bool is_class_or_enum_with_swap_adl<T, U, enable_if_t<
        (is_class_v<remove_cvref_t<T>> || is_enum_v<remove_cvref_t<T>>
            || is_class_v<remove_cvref_t<U>> || is_enum_v<remove_cvref_t<T>>)
        && is_void_v<void_t<decltype(swap(declval<T&>(), declval<U&>()))>>
        >> = true;

    template <class T>
    void swap(T&, T&) = delete;

    struct swap_fn
    {
        template <class T, class U>
        constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(swap(AZStd::forward<T>(t), AZStd::forward<U>(u))))
            ->enable_if_t<is_class_or_enum_with_swap_adl<T, U>>
        {
            swap(AZStd::forward<T>(t), AZStd::forward<U>(u));
        }

        // ranges::swap customization point https://eel.is/c++draft/concepts#concept.swappable-2.2
        // Implemented in ranges.h as to prevent circular dependency.
        // ranges::swap_ranges depends on the range concepts that can't be defined here
        template <class T, class U>
        constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept((*this)(*t, *u)))
            ->enable_if_t<!is_class_or_enum_with_swap_adl<T, U>
            && is_array_v<T> && is_array_v<U> && (extent_v<T> == extent_v<U>)
            >;

        template <class T>
        constexpr auto operator()(T& t1, T& t2) const noexcept(noexcept(is_nothrow_move_constructible_v<T>&& is_nothrow_move_assignable_v<T>))
            ->enable_if_t<move_constructible<T>&& assignable_from<T&, T>>
        {
            auto temp(AZStd::move(t1));
            t1 = AZStd::move(t2);
            t2 = AZStd::move(temp);
        }
    };
}

namespace AZStd::ranges
{
    inline namespace customization_point_object
    {
        inline constexpr auto swap = Internal::swap_fn{};
    }
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool swappable_impl = false;
    template <class T>
    constexpr bool swappable_impl<T, void_t<decltype(AZStd::ranges::swap(declval<T&>(), declval<T&>()))>> = true;

    template <class T, class U, class = void>
    constexpr bool swappable_with_impl = false;
    template <class T, class U>
    constexpr bool swappable_with_impl<T, U, enable_if_t<common_reference_with<T, U>
        && sfinae_trigger_v<
        decltype(AZStd::ranges::swap(declval<T&>(), declval<T&>())),
        decltype(AZStd::ranges::swap(declval<U&>(), declval<U&>())),
        decltype(AZStd::ranges::swap(declval<T&>(), declval<U&>())),
        decltype(AZStd::ranges::swap(declval<U&>(), declval<T&>()))>>> = true;
}
namespace AZStd
{
    template <class T>
    /*concept*/ constexpr bool signed_integral = integral<T> && is_signed_v<T>;
    template <class T>
    /*concept*/ constexpr bool unsigned_integral = integral<T> && !signed_integral<T>;

    template<class T>
    /*concept*/ constexpr bool swappable = Internal::swappable_impl<T>;

    template<class T, class U>
    /*concept*/ constexpr bool swappable_with = Internal::swappable_with_impl<T, U>;
}


namespace AZStd::Internal
{
    // boolean-testable concept (exposition only in the C++standard)
    template<class T>
    constexpr bool boolean_testable_impl = convertible_to<T, bool>;

    template<class T, class = void>
    constexpr bool boolean_testable = false;
    template<class T>
    constexpr bool boolean_testable<T, enable_if_t<boolean_testable_impl<T> && boolean_testable_impl<decltype(!declval<T>())>>> = true;

    // weakly comparable ==, !=
    template<class T, class U, class = void>
    constexpr bool weakly_equality_comparable_with = false;
    template<class T, class U>
    constexpr bool weakly_equality_comparable_with<T, U, enable_if_t<
        boolean_testable<decltype(declval<AZStd::remove_reference_t<T>&>() == declval<AZStd::remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<AZStd::remove_reference_t<T>&>() != declval<AZStd::remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<AZStd::remove_reference_t<U>&>() == declval<AZStd::remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<AZStd::remove_reference_t<U>&>() != declval<AZStd::remove_reference_t<T>&>())>
        >> = true;

    // partially ordered <, >, <=, >=
    template<class, class U, class = void>
    constexpr bool partially_ordered_with_impl = false;
    template<class T, class U>
    constexpr bool partially_ordered_with_impl<T, U, enable_if_t<
        boolean_testable<decltype(declval<const remove_reference_t<T>&>() < declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() > declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() <= declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<T>&>() >= declval<const remove_reference_t<U>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() < declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() > declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() <= declval<const remove_reference_t<T>&>())>
        && boolean_testable<decltype(declval<const remove_reference_t<U>&>() >= declval<const remove_reference_t<T>&>())>
        >> = true;
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
    constexpr bool equally_comparable_with_impl<T, U, enable_if_t<equality_comparable<T>
        && equality_comparable<U>
        && common_reference_with<const remove_reference_t<T>&, const remove_reference_t<U>&>
        && equality_comparable<common_reference_t<const remove_reference_t<T>&, const remove_reference_t<U>&>>
        && Internal::weakly_equality_comparable_with<T, U>
        >> = true;
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
    constexpr bool totally_ordered_with_impl<T, U, enable_if_t<totally_ordered<T>&& totally_ordered<U>
        && equality_comparable_with<T, U>
        && totally_ordered<common_reference_t<const remove_reference_t<T>&, const remove_reference_t<U>&>>
        && partially_ordered_with<T, U>
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
    constexpr bool default_initializable_impl < T, enable_if_t < constructible_from<T>
        && sfinae_trigger_v<decltype(T{}) > && Internal::is_default_initializable<T> >> = true;

    template <class T, class = void>
    constexpr bool movable_impl = false;
    template <class T>
    constexpr bool movable_impl<T, enable_if_t<is_object_v<T> && move_constructible<T> &&
        assignable_from<T&, T> && swappable<T>> > = true;

    template <class T, class = void>
    constexpr bool copy_constructible_impl = false;
    template <class T>
    constexpr bool copy_constructible_impl<T, enable_if_t<move_constructible<T> &&
        constructible_from<T, T&> && convertible_to<T&, T> &&
        constructible_from<T, const T&> && convertible_to<const T&, T> &&
        constructible_from<T, const T> && convertible_to<const T, T>> > = true;
}

namespace AZStd
{
    // movable
    template <class T>
    /*concept*/ constexpr bool movable = Internal::movable_impl<T>;

    // default_initializable
    template<class T>
    /*concept*/ constexpr bool default_initializable = Internal::default_initializable_impl<T>;

    //  copy constructible
    template<class T>
    /*concept*/ constexpr bool copy_constructible = Internal::copy_constructible_impl<T>;
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool copyable_impl = false;
    template <class T>
    constexpr bool copyable_impl<T, enable_if_t<copy_constructible<T> && movable<T> && assignable_from<T&, T&> &&
        assignable_from<T&, const T&> && assignable_from<T&, const T>> > = true;
}

namespace AZStd
{
    // copyable
    template<class T>
    /*concept*/ constexpr bool copyable = Internal::copyable_impl<T>;

    // semiregular
    template<class T>
    /*concept*/ constexpr bool semiregular = copyable<T> && default_initializable<T>;

    // regular
    template<class T>
    /*concept*/ constexpr bool regular = semiregular<T> && equality_comparable<T>;
}

// Iterator Concepts
namespace AZStd::Internal
{
    template <class T>
    constexpr bool is_integer_like = integral<T> && !same_as<T, bool>;

    template <class T>
    constexpr bool is_signed_integer_like = signed_integral<T>;

    template <class T, class = void>
    constexpr bool weakly_incrementable_impl = false;
    template <class T>
    constexpr bool weakly_incrementable_impl<T, enable_if_t<movable<T>
        && is_signed_integer_like<iter_difference_t<T>>
        && same_as<decltype(++declval<T&>()), T&>
        && sfinae_trigger_v<decltype(declval<T&>()++)> >> = true;
}

namespace AZStd
{
    // models weakly_incrementable concept
    template <class T>
    /*concept*/ constexpr bool weakly_incrementable = Internal::weakly_incrementable_impl<T>;

    // models input_or_output_iterator concept
    template <class T>
    /*concept*/ constexpr bool input_or_output_iterator = !is_void_v<T>
        && weakly_incrementable<T>;
}

namespace AZStd::Internal
{
    template <class T, class = void>
    constexpr bool incrementable_impl = false;
    template <class T>
    constexpr bool incrementable_impl<T, enable_if_t<regular<T>
        && weakly_incrementable<T>
        && same_as<decltype(declval<T&>()++), T> >> = true;
}

namespace AZStd
{
    template <class T>
    /*concept*/ constexpr bool incrementable = Internal::incrementable_impl<T>;
}

namespace AZStd
{
    template<class S, class I>
    /*concept*/ constexpr bool sentinel_for = semiregular<S> &&
        input_or_output_iterator<I> &&
        Internal::weakly_equality_comparable_with<S, I>;
    template<class S, class I>
    inline constexpr bool disable_sized_sentinel_for = false;
}

namespace AZStd::Internal
{
    template<class S, class I, class = void>
    /*concept*/ constexpr bool sized_sentinel_for_impl = false;
    template<class S, class I>
    /*concept*/ constexpr bool sized_sentinel_for_impl<S, I, enable_if_t<
        sentinel_for<S, I>
        && !disable_sized_sentinel_for<remove_cv_t<S>, remove_cv_t<I>>
        && same_as<decltype(declval<S>() - declval<I>()), iter_difference_t<I>>
        && same_as<decltype(declval<I>() - declval<S>()), iter_difference_t<I>> >> = true;
}

namespace AZStd
{
    template<class S, class I>
    /*concept*/ constexpr bool sized_sentinel_for = Internal::sized_sentinel_for_impl<S, I>;

    template<class I>
    struct iterator_traits;
}

namespace AZStd::Internal
{
    // ITER_CONCEPT(I) general concept
    template<class I, class = void>
    constexpr bool use_traits_iterator_concept_for_concept = false;
    template<class I>
    constexpr bool use_traits_iterator_concept_for_concept<I, void_t<typename iterator_traits<I>::iterator_concept>> = true;

    template<class I, class = void>
    constexpr bool use_traits_iterator_category_for_concept = false;
    template<class I>
    constexpr bool use_traits_iterator_category_for_concept<I,
        void_t<typename iterator_traits<I>::iterator_category>> = !use_traits_iterator_concept_for_concept<I>;

    template<class I, class = void>
    constexpr bool use_random_access_iterator_tag_for_concept = false;
    template<class I>
    constexpr bool use_random_access_iterator_tag_for_concept<I,
        void_t<iterator_traits<I>>> = !use_traits_iterator_concept_for_concept<I>
        && !use_traits_iterator_category_for_concept<I>;

    template<class I, class = void>
    struct iter_concept;

    template<class I>
    struct iter_concept<I, enable_if_t<use_traits_iterator_concept_for_concept<I>>>
    {
        using type = typename iterator_traits<I>::iterator_concept;
    };
    template<class I>
    struct iter_concept<I, enable_if_t<use_traits_iterator_category_for_concept<I>>>
    {
        using type = typename iterator_traits<I>::iterator_category;
    };

    template<class I>
    struct iter_concept<I, enable_if_t<use_random_access_iterator_tag_for_concept<I>>>
    {
        using type = random_access_iterator_tag;
    };
    template<class I>
    using iter_concept_t = typename iter_concept<I>::type;
}

namespace AZStd
{
    // indirectly readable
    template <class In>
    /*concept*/ constexpr bool indirectly_readable = Internal::indirectly_readable_impl<remove_cvref_t<In>>;
}

namespace AZStd::Internal
{
    // model the indirectly writable concept
    template <class Out, class T, class = void>
    constexpr bool indirectly_writable_impl = false;

    template <class Out, class T>
    constexpr bool indirectly_writable_impl<Out, T, void_t<
        decltype(*declval<Out&>() = declval<T>()),
        decltype(*declval<Out>() = declval<T>()),
        decltype(const_cast<const iter_reference_t<Out>&&>(*declval<Out&>()) = declval<T>()),
        decltype(const_cast<const iter_reference_t<Out>&&>(*declval<Out>()) = declval<T>())>
        > = true;
}
namespace AZStd
{
    // indirectly writable
    template <class Out, class T>
    /*concept*/ constexpr bool indirectly_writable = Internal::indirectly_writable_impl<Out, T>;

    // indirectly movable
    template<class In, class Out>
    /*concept*/ constexpr bool indirectly_movable = indirectly_readable<In> && indirectly_writable<Out, iter_rvalue_reference_t<In>>;
}

namespace AZStd::Internal
{
    template<class In, class Out, class = void>
    constexpr bool indirectly_movable_storage_impl = false;

    template<class In, class Out>
    constexpr bool indirectly_movable_storage_impl<In, Out, enable_if_t<
        indirectly_movable<In, Out> &&
        indirectly_writable<Out, iter_value_t<In>> &&
        movable<iter_value_t<In>> &&
        constructible_from<iter_value_t<In>, iter_rvalue_reference_t<In>> &&
        assignable_from<iter_value_t<In>&, iter_rvalue_reference_t<In>>> > = true;
}

namespace AZStd
{
    template<class In, class Out>
    /*concept*/ constexpr bool indirectly_movable_storable = Internal::indirectly_movable_storage_impl<In, Out>;
}

namespace AZStd::Internal
{
    template<class In, class Out, class = void>
    constexpr bool indirectly_copyable_impl = false;

    template<class In, class Out>
    constexpr bool indirectly_copyable_impl<In, Out, enable_if_t<
        indirectly_readable<In> &&
        indirectly_writable<Out, iter_reference_t<In>>> > = true;
}

namespace AZStd
{
    // indirectly copyable
    template<class In, class Out>
    /*concept*/ constexpr bool indirectly_copyable = Internal::indirectly_copyable_impl<In, Out>;
}
namespace AZStd::Internal
{
    template<class In, class Out, class = void>
    constexpr bool indirectly_copyable_storable_impl = false;

    template<class In, class Out>
    constexpr bool indirectly_copyable_storable_impl<In, Out, enable_if_t<
        indirectly_copyable<In, Out> &&
        indirectly_writable<Out, iter_value_t<In>&> &&
        indirectly_writable<Out, const iter_value_t<In>&> &&
        indirectly_writable<Out, iter_value_t<In>&&> &&
        indirectly_writable<Out, const iter_value_t<In>&&> &&
        copyable<iter_value_t<In>> &&
        constructible_from<iter_value_t<In>, iter_reference_t<In>> &&
        assignable_from<iter_value_t<In>&, iter_reference_t<In>>> > = true;
}

namespace AZStd
{
    template<class In, class Out>
    /*concept*/ constexpr bool indirectly_copyable_storable = Internal::indirectly_copyable_storable_impl<In, Out>;
}

namespace AZStd::ranges::Internal
{
    template<class I1, class I2>
    void iter_swap(I1, I2) = delete;

    template <class I1, class I2, class = void>
    constexpr bool iter_swap_adl = false;

    template <class I1, class I2>
    constexpr bool iter_swap_adl<I1, I2, void_t<decltype(iter_swap(declval<I1>(), declval<I2>()))>> = true;

    template <class I1, class I2, class = void>
    constexpr bool is_class_or_enum_with_iter_swap_adl = false;

    template <class I1, class I2>
    constexpr bool is_class_or_enum_with_iter_swap_adl<I1, I2, enable_if_t<iter_swap_adl<I1, I2>
        && (is_class_v<remove_cvref_t<I1>> || is_enum_v<remove_cvref_t<I1>>)
        && (is_class_v<remove_cvref_t<I2>> || is_enum_v<remove_cvref_t<I2>>)>> = true;

    struct iter_swap_fn
    {
        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<is_class_or_enum_with_iter_swap_adl<I1, I2>
            >
        {
            iter_swap(AZStd::forward<I1>(i1), AZStd::forward<I1>(i2));
        }
        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<!is_class_or_enum_with_iter_swap_adl<I1, I2>
            && indirectly_readable<I1>
            && indirectly_readable<I2>
            && swappable_with<iter_reference_t<I1>, iter_reference_t<I2>>
            >
        {
            ranges::swap(*i1, *i2);
        }

        template <class I1, class I2>
        constexpr auto operator()(I1&& i1, I2&& i2) const
            ->enable_if_t<!is_class_or_enum_with_iter_swap_adl<I1, I2>
            && indirectly_movable_storable<I1, I2>
            && indirectly_movable_storable<I2, I1>
            >
        {
            *AZStd::forward<I1>(i1) = iter_exchange_move(AZStd::forward<I2>(i2), AZStd::forward<I1>(i1));
        }

    private:
        template<class X, class Y>
        static constexpr iter_value_t<X> iter_exchange_move(X&& x, Y&& y)
            noexcept(noexcept(iter_value_t<X>(iter_move(x))) && noexcept(*x = iter_move(y)))
        {
            iter_value_t<X> old_value(iter_move(x));
            *x = iter_move(y);
            return old_value;
        }
    };
}

namespace AZStd::ranges
{
    inline namespace customization_point_object
    {
        inline constexpr Internal::iter_swap_fn iter_swap{};
    }
}


namespace AZStd::Internal
{
    template <class I1, class I2, class = void>
    constexpr bool indirectly_swappable_impl = false;
    template <class I1, class I2>
    constexpr bool indirectly_swappable_impl<I1, I2, enable_if_t<
        indirectly_readable<I1>&& indirectly_readable<I2>
        && sfinae_trigger_v<
        decltype(AZStd::ranges::iter_swap(declval<I1>(), declval<I1>())),
        decltype(AZStd::ranges::iter_swap(declval<I2>(), declval<I2>())),
        decltype(AZStd::ranges::iter_swap(declval<I1>(), declval<I2>())),
        decltype(AZStd::ranges::iter_swap(declval<I2>(), declval<I1>()))>>> = true;
}

namespace AZStd
{
    template<class I1, class I2 = I1>
    /*concept*/ constexpr bool indirectly_swappable = Internal::indirectly_swappable_impl<I1, I2>;
}

namespace AZStd::Internal
{
    template<class I, class = void>
    constexpr bool input_iterator_impl = false;
    template<class I>
    constexpr bool input_iterator_impl<I, enable_if_t<input_or_output_iterator<I>
        && derived_from<iter_concept_t<I>, input_iterator_tag>
        && indirectly_readable<I>
        >> = true;
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
    constexpr bool output_iterator_impl<I, T, enable_if_t<input_or_output_iterator<I>
        && indirectly_writable<I, T>
        && sfinae_trigger_v<decltype(*declval<I&>()++ = AZStd::declval<T>())>
        >> = true;
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
    constexpr bool forward_iterator_impl<I, enable_if_t<input_iterator<I>
        && derived_from<Internal::iter_concept_t<I>, forward_iterator_tag>
        && incrementable<I>
        && sentinel_for<I, I>> > = true;
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
    constexpr bool bidirectional_iterator_impl<I, enable_if_t<forward_iterator<I>
        && derived_from<iter_concept_t<I>, bidirectional_iterator_tag>
        && same_as<decltype(--declval<I&>()), I&>
        && same_as<decltype(declval<I&>()--), I> >> = true;
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
    constexpr bool random_access_iterator_impl<I, enable_if_t<bidirectional_iterator<I>
        && derived_from<iter_concept_t<I>, random_access_iterator_tag>
        && totally_ordered<I>
        && sized_sentinel_for<I, I>
        && same_as<decltype(declval<I&>() += declval<const iter_difference_t<I>>()), I&>
        && same_as<decltype(declval<const I>() + declval<const iter_difference_t<I>>()), I>
        && same_as<decltype(declval<iter_difference_t<I>>() + declval<const I>()), I>
        && same_as<decltype(declval<I&>() -= declval<const iter_difference_t<I>>()), I&>
        && same_as<decltype(declval<const I>() - declval<const iter_difference_t<I>>()), I>
        && same_as<decltype(declval<const I&>()[declval<iter_difference_t<I>>()]), iter_reference_t<I>>>>
        = true;
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
    constexpr bool contiguous_iterator_impl<I, enable_if_t<random_access_iterator<I>
        && derived_from<iter_concept_t<I>, contiguous_iterator_tag>
        && is_lvalue_reference_v<iter_reference_t<I>>
        && indirectly_readable<I>
        && same_as<iter_value_t<I>, remove_cvref_t<iter_reference_t<I>>>
        > >
        = same_as<decltype(to_address(declval<const I&>())), add_pointer_t<iter_reference_t<I>>>;
}

namespace AZStd
{
    // contiguous iterator
    template<class I>
    /*concept*/ constexpr bool contiguous_iterator = Internal::contiguous_iterator_impl<I>;
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
    /*concept*/ constexpr bool relation = predicate<R, T, T> && predicate<R, U, U>
        && predicate<R, T, U> && predicate<R, U, T>;

    // models the equivalence_relation concept
    template <class R, class T, class U>
    /*concept*/ constexpr bool equivalence_relation = relation<R, T, U>;

    // models the strict_weak_order concept
    // Note: semantically this is different than equivalence_relation
    template <class R, class T, class U>
    /*concept*/ constexpr bool strict_weak_order = relation<R, T, U>;
}
