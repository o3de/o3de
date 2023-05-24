/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/invoke.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/variadic.h>

/**
* Helper to create checkers for member function inside a class.
* Explanation of how checks are done:
* 1. Direct Member check:
* This validates that the class itself has the member and that is not being detected from any base class
* The check is done by using the box_member template to enforce that the member function pointer
* is actually from the supplied class type and not from a base type
*
* 2. Member Invocable with Parameters and returns Result check:
* This validates that the member exist on the class and if it is a non-static member
* it can be invoked with a signature of (<class name instance>, Params...)
* or if it is a static member it can be invoked with a signature of (Params...)
* and returns the result type R
*/
// Used to append options to the AZStd::is_invocable_r_v below
#define AZ_PREFIX_ARG_WITH_COMMA(_Arg) , _Arg


namespace AZStd::HasMemberInternal
{
    // Function declaration only used in unevaluated context
    // in order to enforce that the member function pointer
    // is a direct member of the class
    template<class T, class MemType, MemType Member, class... Args>
    auto direct_member_box() -> AZStd::invoke_result_t<MemType, T, Args...>;
}

/**
* AZ_HAS_MEMBER detects if the member exists directly on the class and
* can be invoked with the specified function name and signature
*
* NOTE: This does not include base classes in the check
* Ex. AZ_HAS_MEMBER(IsReadyMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyMember<A>::value (== false)
*     HasIsReadyMember<B>::value (== true)
*     HasIsReadyMember<Der>::value (== false)
*/
#define AZ_HAS_MEMBER_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature, _Qual)          \
    template<class T, class = void>                                                                \
    class Has##_HasName                                                                            \
        : public AZStd::false_type                                                                 \
    {                                                                                              \
    };                                                                                             \
    template<class T>                                                                              \
    class Has##_HasName<T, AZStd::enable_if_t<AZStd::is_invocable_r_v<                             \
            _ResultType,                                                                           \
            decltype(::AZStd::HasMemberInternal::direct_member_box<AZStd::remove_cvref_t<T>,       \
                _ResultType (AZStd::remove_cvref_t<T>::*) _ParamsSignature _Qual,                  \
                &AZStd::remove_cvref_t<T>::_FunctionName                                           \
                AZ_FOR_EACH_UNWRAP(AZ_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>)>                  \
    >>                                                                                             \
         : public AZStd::true_type                                                                 \
    {                                                                                              \
    };                                                                                             \
    template <class T>                                                                             \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;

#define AZ_HAS_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature) \
    AZ_HAS_MEMBER_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature,)


/**
* AZ_HAS_MEMBER_INCLUDE_BASE detects if the class or any of its base classes
* can invoke the member function with the specified function name and signature
* AZ_HAS_MEMBER_INCLUDE_BASE(IsReadyIncludeBase,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        static void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyIncludeBase<A>::value (== false)
*     HasIsReadyIncludeBase<B>::value (== true)
*     HasIsReadyIncludeBase<Der>::value (== true)
*/
#define AZ_HAS_MEMBER_INCLUDE_BASE_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature, _Qual) \
    template<class T, class = void>                                                              \
    class Has##_HasName                                                                          \
        : public AZStd::false_type                                                               \
    {                                                                                            \
    };                                                                                           \
    template<class T>                                                                            \
    class Has##_HasName<T, AZStd::enable_if_t<AZStd::is_invocable_r_v<                           \
        _ResultType,                                                                             \
        decltype(static_cast<_ResultType (AZStd::remove_cvref_t<T>::*) _ParamsSignature _Qual>(&AZStd::remove_cvref_t<T>::_FunctionName)), \
        AZStd::remove_cvref_t<T>                                                                 \
        AZ_FOR_EACH_UNWRAP(AZ_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>                          \
    >>                                                                                           \
         : public AZStd::true_type                                                               \
    {                                                                                            \
    };                                                                                           \
    template <class T>                                                                           \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;

#define AZ_HAS_MEMBER_INCLUDE_BASE(_HasName, _FunctionName, _ResultType, _ParamsSignature) \
    AZ_HAS_MEMBER_INCLUDE_BASE_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature,)

/**
* AZ_HAS_STATIC_MEMBER detects if the class or any of its base classes
* can invoke the static member function with the specified function name and signature
* NOTE: There is no way to validate if the static member function was directly
* declared on the supplied class. This is due to static member functions NOT containing
* the class type as part of the function signature
* Ex. AZ_HAS_STATIC_MEMBER(IsReadyStaticMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        static void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyStaticMember<A>::value (== false)
*     HasIsReadyStaticMember<B>::value (== true)
*     HasIsReadyStaticMember<Der>::value (== true)
*/
#define AZ_HAS_STATIC_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)         \
    template<class T, class = void>                                                          \
    class Has##_HasName                                                                      \
        : public AZStd::false_type                                                           \
    {                                                                                        \
    };                                                                                       \
    template<class T>                                                                        \
    class Has##_HasName<T, AZStd::enable_if_t<AZStd::is_invocable_r_v<                       \
            _ResultType,                                                                     \
            decltype(static_cast<_ResultType (*) _ParamsSignature>(&AZStd::remove_cvref_t<T>::_FunctionName)) \
            AZ_FOR_EACH_UNWRAP(AZ_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>                  \
    >>                                                                                       \
         : public AZStd::true_type                                                           \
    {                                                                                        \
    };                                                                                       \
    template<class T>                                                                        \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;
