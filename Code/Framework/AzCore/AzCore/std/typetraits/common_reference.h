/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/common_type.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/declval.h>

namespace AZStd
{
    template <class T, class U, template<class> class TQual, template<class> class UQual>
    struct basic_common_reference
    {};
}

namespace AZStd::Internal
{
    template <class... Args>
    constexpr bool is_valid_type_v = true;

    // const volatile and reference qualifier copy templates
    template <class T, class QualType>
    struct copy_cv_qual
    {
        using type = conditional_t<is_const_v<T>, conditional_t<is_volatile_v<T>, const volatile QualType, const QualType>,
            conditional_t<is_volatile_v<T>, volatile QualType, QualType>>;
    };

    template <class T,class QualType>
    using copy_cv_qual_t = typename copy_cv_qual<T, QualType>::type;

    static_assert(is_same_v<copy_cv_qual_t<int, float>, float>);
    static_assert(is_same_v<copy_cv_qual_t<const int, float>, const float>);
    static_assert(is_same_v<copy_cv_qual_t<volatile int, float>, volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const volatile int, float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<int, const float>, const float>);
    static_assert(is_same_v<copy_cv_qual_t<const int, const float>, const float>);
    static_assert(is_same_v<copy_cv_qual_t<volatile int, const float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const volatile int, const float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<int, volatile float>, volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const int, volatile float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<volatile int, volatile float>, volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const volatile int, volatile float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<int, const volatile float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const int, const volatile float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<volatile int, const volatile float>, const volatile float>);
    static_assert(is_same_v<copy_cv_qual_t<const volatile int, const volatile float>, const volatile float>);

    template <class T, class QualType>
    struct copy_reference_qual
    {
        using type = conditional_t<is_lvalue_reference_v<T>, QualType&,
            conditional_t<is_rvalue_reference_v<T>, QualType&&, QualType>>;
    };

    template <class T, class QualType>
    using copy_reference_qual_t = typename copy_reference_qual<T, QualType>::type;

    static_assert(is_same_v<copy_reference_qual_t<int, float>, float>);
    static_assert(is_same_v<copy_reference_qual_t<int&, float>, float&>);
    static_assert(is_same_v<copy_reference_qual_t<int&&, float>, float&&>);
    static_assert(is_same_v<copy_reference_qual_t<int, float&>, float&>);
    static_assert(is_same_v<copy_reference_qual_t<int&, float&>, float&>);
    static_assert(is_same_v<copy_reference_qual_t<int&&, float&>, float&>);
    static_assert(is_same_v<copy_reference_qual_t<int, float&&>, float&&>);
    static_assert(is_same_v<copy_reference_qual_t<int&, float&&>, float&>);
    static_assert(is_same_v<copy_reference_qual_t<int&&, float&&>, float&&>);

    template <class T, class QualType>
    using copy_cvref_qual_t = copy_cv_qual_t<copy_reference_qual_t<T, QualType>, QualType>;

    template <class T>
    struct copy_qualifiers_from_t
    {
        template <class X>
        using templ = copy_cvref_qual_t<T, X>;
    };

    template <class T, class U>
    using cond_res = decltype(false ? declval<copy_cv_qual_t<remove_reference_t<T>, remove_reference_t<U>>&>()
        : declval<copy_cv_qual_t<remove_reference_t<U>, remove_reference_t<T>>&>());

    // common reference helper templates begin
    template <class T, class U, typename = void>
    struct common_reference_base_reference_test;

    // COMMON_REF is defined within the C++ standard at https://eel.is/c++draft/meta.trans.other#3.5
    template <class T, class U>
    struct common_reference_base_reference_test<T, U,
        enable_if_t<is_lvalue_reference_v<T>&& is_lvalue_reference_v<U>,
        void_t<cond_res<T,U>> >>
    {
        // Uses the ternary operator for determining the common type
        using type = cond_res<T, U>;
    };

    template <class T, class U>
    struct common_reference_base_reference_test<T, U, enable_if_t<is_rvalue_reference_v<T> && is_rvalue_reference_v<U>
        && is_valid_type_v<typename common_reference_base_reference_test<remove_reference_t<T>&, remove_reference_t<U>&>::type> >>
    {
        using C = remove_reference_t<typename common_reference_base_reference_test<remove_reference_t<T>&, remove_reference_t<U>&>::type>;
        using type = AZStd::enable_if_t<is_convertible_v<T, C> && is_convertible_v<U, C>, C>;
    };

    template <class T, class U>
    struct common_reference_base_reference_test<T, U, enable_if_t<is_rvalue_reference_v<T> && is_lvalue_reference_v<U>
        && is_valid_type_v<typename common_reference_base_reference_test<const remove_reference_t<T>&, remove_reference_t<U>&>::type> >>
    {
        // Turn rvalue references to const lvalue references
        using D = typename common_reference_base_reference_test<const remove_reference_t<T>&, remove_reference_t<U>&>::type;
        using type = AZStd::enable_if_t<is_convertible_v<T, D>, D>;
    };

    template <class T, class U>
    struct common_reference_base_reference_test<T, U, enable_if_t<is_lvalue_reference_v<T> && is_rvalue_reference_v<U>
        && is_valid_type_v<typename common_reference_base_reference_test<U, T>::type>>>
        : common_reference_base_reference_test<U, T>
    {
        // Swap the parameters to call the 3rd specialization for common_reference_base_reference_test
    };

    template <class T, class U, typename = void>
    constexpr bool has_reference_test = false;

    template <class T, class U>
    constexpr bool has_reference_test<T, U, void_t<typename common_reference_base_reference_test<T, U>::type>> = true;

    template <class T, class U, typename = void>
    struct basic_common_reference_test;

    template <class T, class U>
    struct basic_common_reference_test<T, U, void_t<typename basic_common_reference<remove_cvref_t<T>, remove_cvref_t<U>,
        copy_qualifiers_from_t<T>::template templ, copy_qualifiers_from_t<U>::template templ>::type>>
    {
        using type = typename basic_common_reference<remove_cvref_t<T>, remove_cvref_t<U>,
            copy_qualifiers_from_t<T>::template templ, copy_qualifiers_from_t<U>::template templ>::type;
    };

    template <class T, class U, typename = void>
    constexpr bool has_basic_common_reference_test = false;

    template <class T, class U>
    constexpr bool has_basic_common_reference_test<T, U,
        void_t<typename basic_common_reference_test<T, U>::type>> = true;

    template <class T, class U, typename = void>
    constexpr bool has_condition_result_test = false;

    template <class T, class U>
    constexpr bool has_condition_result_test<T, U, void_t<decltype(false ? declval<T>() : declval<U>())>> = true;

    template <class T, class U, typename = void>
    struct common_reference_base_test
    {};

    template <class T, class U>
    struct common_reference_base_test<T, U, enable_if_t<has_reference_test<T, U>>>
        : common_reference_base_reference_test<T, U>
    {};
    template <class T, class U>
    struct common_reference_base_test<T, U, enable_if_t<!has_reference_test<T, U>
        && has_basic_common_reference_test<T, U>>>
        : basic_common_reference_test<T, U>
    {};
    template <class T, class U>
    struct common_reference_base_test<T, U, enable_if_t<!has_reference_test<T, U>
        && !has_basic_common_reference_test<T, U> && has_condition_result_test<T,U>>>
    {
        using type = decltype(false ? declval<T>() : declval<U>());
    };
    template <class T, class U>
    struct common_reference_base_test<T, U, enable_if_t<!has_reference_test<T, U>
        && !has_basic_common_reference_test<T, U> && !has_condition_result_test<T, U>>>
        : common_type<T, U>
    {};

    template <class... T>
    struct common_reference_base
    {};

    template <class T>
    struct common_reference_base<T>
    {
        using type = T;
    };
    template <class T, class U>
    struct common_reference_base<T, U>
        : common_reference_base_test<T, U>
    {};

    template <class T, class U, class V, class... Rs>
    struct common_reference_base<T, U, V, Rs...>
        : common_reference_base<typename common_reference_base<T, U>::type, V, Rs...>
    {};
}
namespace AZStd
{
    template <class... T>
    struct common_reference
        : Internal::common_reference_base<T...>
    {};

    template <class... T>
    using common_reference_t = typename common_reference<T...>::type;

    // models the common reference concept
    namespace Internal
    {
        template<class T, class U, typename = void>
        constexpr bool common_reference_with_impl = false;
        template<class T, class U>
        constexpr bool common_reference_with_impl<T, U, enable_if_t<
            same_as<common_reference_t<T, U>, common_reference_t<U, T>>
            && convertible_to<T, common_reference_t<T, U>>
            && convertible_to<U, common_reference_t<T, U>>
            >> = true;
    }

    template<class T, class U>
    /*concept*/ constexpr bool common_reference_with = Internal::common_reference_with_impl<T, U>;
}
