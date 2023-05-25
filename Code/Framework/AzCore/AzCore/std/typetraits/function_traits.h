/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/add_reference.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>
#include <AzCore/std/typetraits/is_rvalue_reference.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/void_t.h>

namespace AZStd
{
    namespace Internal
    {
        enum class qualifier_flags : uint32_t
        {
            default_ = 0,
            const_ = 1,
            volatile_ = 2,
            lvalue_ref = 4,
            rvalue_ref = 8,
            const_volatile = const_ | volatile_,
            const_lvalue_ref = const_ | lvalue_ref,
            const_rvalue_ref = const_ | rvalue_ref,
            volatile_lvalue_ref = volatile_ | lvalue_ref,
            volatile_rvalue_ref = volatile_ | rvalue_ref,
            const_volatile_lvalue_ref = const_volatile | lvalue_ref,
            const_volatile_rvalue_ref = const_volatile | rvalue_ref,
        };

        struct error_type
        {
            error_type() = delete;
        };

        template<typename T, typename = void>
        struct has_call_operator
            : false_type
        {
        };

        template<typename T>
        struct has_call_operator<T, void_t<decltype(&T::operator())>>
            : true_type
        {
        };

        template<typename T>
        constexpr bool has_call_operator_v = has_call_operator<T>::value;

        template<typename T = void>
        struct default_traits
        {
            static constexpr bool value = false;
            static constexpr size_t arity = 0;
            static constexpr size_t num_args = arity;
            static constexpr qualifier_flags qual_flags = qualifier_flags::default_;

            /// represents the template type of the function_traits
            using type = T;
            /// set to class type if a pointer to member function or pointer to member object is supplied
            using class_type = error_type;
            /// set to the qualified reference type of the class type. Suitable for use with Atd::invoke
            using invoke_type = error_type;
            /// return type of function
            using return_type = error_type;
            /// type which wraps a parameter pack suitable for expansion into std::invoke or std::apply
            /// supplies the @invoke_type member as the first argument
            using arg_types = error_type;
            /// type which wraps a parameter pack without the invoke_type as the first argument
            /// suitable for invoking calls of pointer to member functions
            using non_invoke_arg_types = error_type;
            /// function signature with the invoke_type removed from it
            using function_object_signature = error_type;
            /// callable type that represents the signature of a function that can be passed
            /// to std::invoke
            using function_type = error_type;
            /// allows expansion of the invocable args into a template
            /// useful for unpacking the @invoke type and arg_types R(C::*)(Args...) to a Container<C, Args...>
            template <template<class...> class Container>
            using expand_args = error_type;

            using class_fp_type = error_type;
            using raw_fp_type = error_type;
            using result_type = return_type;
            template<size_t index>
            using get_arg_t = error_type;
            using arg_sequence = error_type;
        };

        template <typename T>
        struct pointer_to_member_function;

        template <typename T, bool = has_call_operator_v<AZStd::remove_reference_t<T>>>
        struct function_object
            : default_traits<T>
        {
        };

        // Calculates function traits for classes with an operator() function
        template <typename Functor>
        struct function_object<Functor, true>
            : pointer_to_member_function<decltype(&AZStd::remove_reference_t<Functor>::operator())>
        {
            using base = pointer_to_member_function<decltype(&AZStd::remove_reference_t<Functor>::operator())>;
            static constexpr bool value = base::value;

            // Remove the class type from the traits of the function for function objects
            using type = Functor;
            using function_type = typename base::function_object_signature;
            using arg_types = typename base::non_invoke_arg_types;
            using class_type = error_type;
            using invoke_type = error_type;
        };

        template <typename T>
        struct pointer_to_member_function
            : default_traits<T>
        {};

        #define AZSTD_FUNCTION_TRAIT_NOARG

        #define AZSTD_FUNCTION_TRAIT_HELPER(cv_ref_qualifier, qualifier_value, noexcept_qualifier) \
            template <typename C, typename R, typename... Args> \
            struct pointer_to_member_function<R(C::*)(Args...) cv_ref_qualifier noexcept_qualifier> \
                : default_traits<> \
            { \
                static constexpr bool value = true; \
                static constexpr size_t arity = sizeof...(Args); \
                static constexpr size_t num_args = arity; \
                static constexpr qualifier_flags qual_flags = qualifier_value; \
                \
                using type = R(C::*)(Args...) cv_ref_qualifier noexcept_qualifier; \
                using class_type = C; \
                using return_type = R; \
                using invoke_type = conditional_t<is_rvalue_reference_v<C cv_ref_qualifier>, C cv_ref_qualifier, add_lvalue_reference_t<C cv_ref_qualifier>>; \
                using arg_types = Internal::pack_traits_arg_sequence<invoke_type, Args...>; \
                using non_invoke_arg_types = Internal::pack_traits_arg_sequence<Args...>; \
                using function_type = return_type(invoke_type, Args...) noexcept_qualifier; \
                using function_object_signature = return_type(Args...) noexcept_qualifier; \
                template<template<class...> class Container> \
                using expand_args = Container<Args...>; \
                \
                using class_fp_type = type; \
                using result_type = return_type; \
                using raw_fp_type = return_type(*)(Args...) noexcept_qualifier; \
                template<size_t index> \
                using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>; \
                using arg_sequence = Internal::pack_traits_arg_sequence<Args...>; \
            }; \
            template <typename C, typename R, typename... Args> \
            struct pointer_to_member_function<R(C::*)(Args..., ...) cv_ref_qualifier noexcept_qualifier > \
                : default_traits<> \
            { \
                static constexpr bool value = true; \
                static constexpr size_t arity = sizeof...(Args); \
                static constexpr size_t num_args = arity; \
                static constexpr qualifier_flags qual_flags = qualifier_value; \
                \
                using type = R(C::*)(Args..., ...) cv_ref_qualifier noexcept_qualifier; \
                using class_type = C; \
                using return_type = R; \
                using invoke_type = conditional_t<is_rvalue_reference_v<C cv_ref_qualifier>, C cv_ref_qualifier, add_lvalue_reference_t<C cv_ref_qualifier>>; \
                using arg_types = Internal::pack_traits_arg_sequence<invoke_type, Args...>; \
                using non_invoke_arg_types = Internal::pack_traits_arg_sequence<Args...>; \
                using function_type = return_type(invoke_type, Args..., ...) noexcept_qualifier; \
                using function_object_signature = return_type(Args..., ...) noexcept_qualifier; \
                template<template<class...> class Container> \
                using expand_args = Container<Args...>; \
                \
                using class_fp_type = type; \
                using result_type = return_type; \
                using raw_fp_type = return_type(*)(Args..., ...) noexcept_qualifier; \
                template<size_t index> \
                using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>; \
                using arg_sequence = Internal::pack_traits_arg_sequence<Args...>; \
            };

        // specializations for functions with const volatile ref qualifiers
        AZSTD_FUNCTION_TRAIT_HELPER(AZSTD_FUNCTION_TRAIT_NOARG, qualifier_flags::default_,);
        AZSTD_FUNCTION_TRAIT_HELPER(const, qualifier_flags::const_,);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile, qualifier_flags::volatile_,);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile, qualifier_flags::const_volatile,);
        AZSTD_FUNCTION_TRAIT_HELPER(&, qualifier_flags::lvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(const&, qualifier_flags::const_lvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&, qualifier_flags::volatile_lvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&, qualifier_flags::const_volatile_lvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(&&, qualifier_flags::rvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(const&&, qualifier_flags::const_rvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&&, qualifier_flags::volatile_rvalue_ref,);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&&, qualifier_flags::const_volatile_rvalue_ref,);
    #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must distinguished from non-noexcept overloads
        // specializations for noexcept functions with const volatile and reference qualifiers
        AZSTD_FUNCTION_TRAIT_HELPER(AZSTD_FUNCTION_TRAIT_NOARG, qualifier_flags::default_, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const, qualifier_flags::const_, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile, qualifier_flags::volatile_, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile, qualifier_flags::const_volatile, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(&, qualifier_flags::lvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const&, qualifier_flags::const_lvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&, qualifier_flags::volatile_lvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&, qualifier_flags::const_volatile_lvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(&&, qualifier_flags::rvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const&&, qualifier_flags::const_rvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&&, qualifier_flags::volatile_rvalue_ref, noexcept);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&&, qualifier_flags::const_volatile_rvalue_ref, noexcept);
    #endif
        #undef AZSTD_FUNCTION_TRAIT_HELPER
        #undef AZSTD_FUNCTION_TRAIT_NOARG

        template <typename T>
        struct pointer_to_member_data
            : default_traits<T>
        {};

        template <typename C, typename D>
        struct pointer_to_member_data<D C::*>
            : default_traits<>
        {
            static constexpr bool value = true;

            using type = D C::*;
            using class_type = C;
            using return_type = add_lvalue_reference_t<D>;
            using invoke_type = const C &;
            using function_type = return_type(invoke_type);
            using arg_types = Internal::pack_traits_arg_sequence<invoke_type>;
            using non_invoke_arg_types = Internal::pack_traits_arg_sequence<>;
            template<template<class...> class Container>
            using expand_args = Container<>;

        };

        template <typename T>
        struct raw_function
            : default_traits<T>
        {};

        template <typename R, typename ...Args>
        struct raw_function<R(Args...)>
            : default_traits<>
        {
            static constexpr bool value = true;
            static constexpr size_t arity = sizeof...(Args);
            static constexpr size_t num_args = arity;

            using type = R(Args...);
            using return_type = R;
            using arg_types = Internal::pack_traits_arg_sequence<Args...>;
            using non_invoke_arg_types = arg_types;
            using function_type = return_type(Args...);
            using function_object_signature = function_type;
            template<template<class...> class Container>
            using expand_args = Container<Args...>;

            using result_type = return_type;
            using raw_fp_type = return_type(*)(Args...);
            template<size_t index>
            using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = Internal::pack_traits_arg_sequence<Args...>;
        };

    #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must distinguished from non-noexcept overloads
        // specializations for noexcept functions with const volatile and reference qualifiers
        template <typename R, typename ...Args>
        struct raw_function<R(Args...) noexcept>
            : raw_function<R(Args...)>
        {
            using type = R(Args...) noexcept;
            using function_type = R(Args...) noexcept;

            using raw_fp_type = R(*)(Args...) noexcept;
        };
    #endif

        template <typename R, typename ...Args>
        struct raw_function<R(Args..., ...)>
            : default_traits<>
        {
            static constexpr bool value = true;
            static constexpr size_t arity = sizeof...(Args);
            static constexpr size_t num_args = arity;

            using type = R(Args..., ...);
            using return_type = R;
            using arg_types = Internal::pack_traits_arg_sequence<Args...>;
            using non_invoke_arg_types = arg_types;
            using function_type = return_type(Args..., ...);
            using function_object_signature = function_type;
            template<template<class...> class Container>
            using expand_args = Container<Args...>;

            using result_type = return_type;
            using raw_fp_type = return_type(*)(Args..., ...);
            template<size_t index>
            using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = Internal::pack_traits_arg_sequence<Args...>;
        };

    #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must distinguished from non-noexcept overloads
        // specializations for noexcept functions with const volatile and reference qualifiers
        template <typename R, typename ...Args>
        struct raw_function<R(Args..., ...) noexcept>
            : raw_function<R(Args..., ...)>
        {
            using type = R(Args..., ...) noexcept;
            using function_type = R(Args..., ...) noexcept;

            using raw_fp_type = R(*)(Args..., ...) noexcept;
        };
    #endif

        template <typename R, typename ...Args>
        struct raw_function<R(*)(Args...)>
            : default_traits<>
        {
            static constexpr bool value = true;
            static constexpr size_t arity = sizeof...(Args);
            static constexpr size_t num_args = arity;

            using type = R(*)(Args...);
            using return_type = R;
            using arg_types = Internal::pack_traits_arg_sequence<Args...>;
            using non_invoke_arg_types = arg_types;
            using function_type = return_type(Args...);
            using function_object_signature = function_type;
            template<template<class...> class Container>
            using expand_args = Container<Args...>;

            using result_type = return_type;
            using raw_fp_type = type;
            template<size_t index>
            using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = Internal::pack_traits_arg_sequence<Args...>;
        };

    #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must distinguished from non-noexcept overloads
        // specializations for noexcept functions with const volatile and reference qualifiers
        template <typename R, typename ...Args>
        struct raw_function<R(*)(Args...) noexcept>
            : raw_function<R(*)(Args...)>
        {
            using type = R(*)(Args...) noexcept;
            using function_type = R(Args...) noexcept;

            using raw_fp_type = type;
        };
    #endif

        template <typename R, typename ...Args>
        struct raw_function<R(*)(Args..., ...)>
            : default_traits<>
        {
            static constexpr bool value = true;
            static constexpr size_t arity = sizeof...(Args);
            static constexpr size_t num_args = arity;

            using type = R(*)(Args..., ...);
            using return_type = R;
            using arg_types = Internal::pack_traits_arg_sequence<Args...>;
            using non_invoke_arg_types = arg_types;
            using function_type = return_type(Args..., ...);
            using function_object_signature = function_type;
            template<template<class...> class Container>
            using expand_args = Container<Args...>;

            using result_type = return_type;
            using raw_fp_type = type;
            template<size_t index>
            using get_arg_t = Internal::pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = Internal::pack_traits_arg_sequence<Args...>;
        };

    #if __cpp_noexcept_function_type
        // C++17 makes exception specifications as part of the type in paper P0012R1
        // Therefore noexcept overloads must distinguished from non-noexcept overloads
        // specializations for noexcept functions with const volatile and reference qualifiers
        template <typename R, typename ...Args>
        struct raw_function<R(*)(Args..., ...) noexcept>
            : raw_function<R(*)(Args..., ...)>
        {
            using type = R(*)(Args..., ...) noexcept;
            using function_type = R(Args..., ...) noexcept;

            using raw_fp_type = type;
        };
    #endif

        template <typename T>
        struct raw_function<T&>
            : conditional_t<raw_function<T>::value, raw_function<T>, default_traits<T&>>
        {
            using type = T&;
        };

        template <typename T>
        using callable_traits = disjunction<
            function_object<T>,
            raw_function<T>,
            pointer_to_member_function<T>,
            pointer_to_member_data<T>,
            default_traits<T>
        >;
    }

    template <typename Function>
    struct function_traits : Internal::callable_traits<remove_cvref_t<Function>>
    {
    };

    template <typename Function>
    using function_traits_get_result_t = typename function_traits<Function>::result_type;

    template <typename Function, size_t index>
    using function_traits_get_arg_t = typename function_traits<Function>::template get_arg_t<index>;
}
