/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>

#include <AzCore/std/concepts/concepts_constructible.h>
#include <AzCore/std/concepts/concepts_copyable.h>
#include <AzCore/std/concepts/concepts_movable.h>
#include <AzCore/std/ranges/iter_move.h>
#include <AzCore/std/typetraits/common_reference.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_object.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/remove_extent.h>
#include <AzCore/std/typetraits/void_t.h>


namespace AZStd
{
    // Bring in std utility functions into AZStd namespace
    using std::forward;

    // forward declare iterator_traits to avoid circular include with iterator.h
    template <class I, class = void>
    struct iterator_traits;
}

// C++20 range traits for iteratable types
namespace AZStd::Internal
{
    // Models the can-reference concept which isn't available until C++20
    // template <class T, class = void>
    template <class T>
    constexpr bool can_reference = true;
    template <>
    inline constexpr bool can_reference<void> = false;

    // Models the dereferencable concept which isn't available until C++20
    template <class T, class = void>
    /*concept*/ constexpr bool dereferenceable = false;
    template <class T>
    constexpr bool dereferenceable<T, enable_if_t<can_reference<decltype(*declval<T>())>>> = true;

    template <class T, class = void>
    constexpr bool is_primary_template_v = false;
    template <class T>
    constexpr bool is_primary_template_v<T, enable_if_t<is_same_v<T, typename T::_is_primary_template>>> = true;

    // indirectly readable traits
    template <typename T, typename = void>
    constexpr bool has_value_type_v = false;
    template <typename T>
    constexpr bool has_value_type_v<T, void_t<typename T::value_type>> = true;
    template <typename T, typename = void>
    constexpr bool has_element_type_v = false;
    template <typename T>
    constexpr bool has_element_type_v<T, void_t<typename T::element_type>> = true;

    template <typename T, typename = void>
    struct object_type_value_requires {};
    template <typename T>
    struct object_type_value_requires<T, enable_if_t<is_object_v<T>>>
    {
        using value_type = remove_cv_t<T>;
    };
    template <typename T, typename = void>
    struct indirectly_readable_requires {};
    template <typename T>
    struct indirectly_readable_requires<T, enable_if_t<!is_primary_template_v<iterator_traits<T>>
        && is_void_v<void_t<typename iterator_traits<T>::value_type>> >>
    {
        // iterator_traits has been been specialized
        using value_type = typename iterator_traits<T>::value_type;
    };

    template <typename T>
    struct indirectly_readable_requires<T, enable_if_t<is_primary_template_v<iterator_traits<T>>
        && is_array_v<T>>>
    {
        using value_type = remove_cv_t<remove_extent_t<T>>;
    };

    template <typename T>
    struct indirectly_readable_requires<T, enable_if_t<is_primary_template_v<iterator_traits<T>>
        && has_value_type_v<T> && !has_element_type_v<T>>>
        : object_type_value_requires<typename T::value_type> {};

    template <typename T>
    struct indirectly_readable_requires<T, enable_if_t<is_primary_template_v<iterator_traits<T>>
        && has_element_type_v<T> && !has_value_type_v<T>>>
        : object_type_value_requires<typename T::element_type> {};

    template <typename T>
    struct indirectly_readable_requires<T, enable_if_t<is_primary_template_v<iterator_traits<T>>
        && has_value_type_v<T>&& has_element_type_v<T>
        && same_as<remove_cv_t<typename T::element_type>, remove_cv_t<typename T::value_type>> >>
        : object_type_value_requires<typename T::value_type> {};

    // incrementable traits
    template <typename T, typename = void>
    constexpr bool has_difference_type_v = false;
    template <typename T>
    constexpr bool has_difference_type_v<T, void_t<typename T::difference_type>> = true;

    template <typename T, typename = void>
    struct object_type_difference_requires {};
    template <typename T>
    struct object_type_difference_requires<T, enable_if_t<is_object_v<T>>>
    {
        using difference_type = ptrdiff_t;
    };

    template <typename T, typename = void>
    struct incrementable_requires {};
    // iterator_traits has been specialized
    template <typename T>
    struct incrementable_requires<T, enable_if_t<conjunction_v<
        bool_constant<!is_primary_template_v<iterator_traits<T>>>,
        bool_constant<is_void_v<void_t<typename iterator_traits<T>::difference_type>>>> >>
    {
        using difference_type = typename iterator_traits<T>::difference_type;
    };
    template <typename T>
    struct incrementable_requires<T, enable_if_t<conjunction_v<
        bool_constant<is_primary_template_v<iterator_traits<T>>>,
        bool_constant<has_difference_type_v<T>>> >>
    {
        using difference_type = typename T::difference_type;
    };
    template <typename T>
    struct incrementable_requires<T, enable_if_t<conjunction_v<
        bool_constant<is_primary_template_v<iterator_traits<T>>>,
        bool_constant<!has_difference_type_v<T>>,
        bool_constant<integral<decltype(declval<const T&>() - declval<const T&>())>>> >>
    {
        using difference_type = make_signed_t<decltype(declval<T>() - declval<T>())>;
    };
}

namespace AZStd
{
    // indirectly_readable_traits for iter_value_t
    template <typename T>
    struct indirectly_readable_traits
        : Internal::indirectly_readable_requires<T> {};
    template <typename T>
    struct indirectly_readable_traits<T*>
        : Internal::object_type_value_requires<T> {};
    template <typename T>
    struct indirectly_readable_traits<const T>
        : indirectly_readable_traits<T> {};

    template <typename T>
    using iter_value_t = typename indirectly_readable_traits<remove_cvref_t<T>>::value_type;

    template <typename T>
    using iter_reference_t = enable_if_t<Internal::dereferenceable<T>, decltype(*declval<T&>())>;

    // incrementable_traits for iter_difference_t
    template <typename T, class = void>
    struct incrementable_traits
        : Internal::incrementable_requires<T> {};
    template <typename T>
    struct incrementable_traits<T*>
        : Internal::object_type_difference_requires<T> {};
    template <typename T>
    struct incrementable_traits<const T>
        : incrementable_traits<T> {};

    template <typename T>
    using iter_difference_t = typename incrementable_traits<remove_cvref_t<T>>::difference_type;

    template <typename T>
    using iter_rvalue_reference_t = decltype(ranges::iter_move(declval<T&>()));

    namespace Internal
    {
        // model the indirectly readable concept
        template <class In, class = void>
        constexpr bool indirectly_readable_impl = false;

        template <class In>
        constexpr bool indirectly_readable_impl<In, enable_if_t<conjunction_v<
            bool_constant<same_as<decltype(*declval<In>()), iter_reference_t<In>>>,
            bool_constant<same_as<decltype(AZStd::ranges::iter_move(declval<In>())), iter_rvalue_reference_t<In>>>,
            bool_constant<common_reference_with<iter_reference_t<In>&&, iter_value_t<In>&>>,
            bool_constant<common_reference_with<iter_reference_t<In>&&, iter_rvalue_reference_t<In>&>>,
            bool_constant<common_reference_with<iter_rvalue_reference_t<In>&&, const iter_value_t<In>&>> >>> = true;
    }

    template <typename T>
    using iter_common_reference_t = enable_if_t<Internal::indirectly_readable_impl<T>,
        common_reference_t<iter_reference_t<T>, iter_value_t<T>&>>;

    template<class It>
    using iter_const_reference_t = enable_if_t<Internal::indirectly_readable_impl<It>,
        common_reference_t<const iter_value_t<It>&&, iter_reference_t<It>>>;

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
    /*concept*/ constexpr bool indirectly_movable = conjunction_v<bool_constant<indirectly_readable<In>>,
        bool_constant<indirectly_writable<Out, iter_rvalue_reference_t<In>>>>;
}

namespace AZStd::Internal
{
    template<class In, class Out, class = void>
    constexpr bool indirectly_movable_storage_impl = false;

    template<class In, class Out>
    constexpr bool indirectly_movable_storage_impl<In, Out, enable_if_t<conjunction_v<
        bool_constant<indirectly_movable<In, Out>>,
        bool_constant<indirectly_writable<Out, iter_value_t<In>>>,
        bool_constant<movable<iter_value_t<In>>>,
        bool_constant<constructible_from<iter_value_t<In>, iter_rvalue_reference_t<In>>>,
        bool_constant<assignable_from<iter_value_t<In>&, iter_rvalue_reference_t<In>>> >>> = true;
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
    constexpr bool indirectly_copyable_impl<In, Out, enable_if_t<conjunction_v<
        bool_constant<indirectly_readable<In>>,
        bool_constant<indirectly_writable<Out, iter_reference_t<In>>> >>> = true;
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
    constexpr bool indirectly_copyable_storable_impl<In, Out, enable_if_t < conjunction_v <
        bool_constant<indirectly_copyable<In, Out>>,
        bool_constant<indirectly_writable<Out, iter_value_t<In>&>>,
        bool_constant<indirectly_writable<Out, const iter_value_t<In>&>>,
        bool_constant<indirectly_writable<Out, iter_value_t<In>&&>>,
        bool_constant<indirectly_writable<Out, const iter_value_t<In>&&>>,
        bool_constant<copyable<iter_value_t<In>>>,
        bool_constant<constructible_from<iter_value_t<In>, iter_reference_t<In>>>,
        bool_constant<assignable_from<iter_value_t<In>&, iter_reference_t<In>>>
        >>> = true;
}

namespace AZStd
{
    template<class In, class Out>
    /*concept*/ constexpr bool indirectly_copyable_storable = Internal::indirectly_copyable_storable_impl<In, Out>;
}
