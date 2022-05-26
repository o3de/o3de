/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/is_same.h>

/**
* Helper to create checkers for member function inside a class.
*
* Ex. AZ_HAS_MEMBER(IsReadyMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        void IsReady();
*     };
*     HasIsReadyMember<A>::value (== false)
*     HasIsReadyMember<B>::value (== true)
*/
#define AZ_HAS_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)        \
    template<class T, class = void>                                                  \
    class Has##_HasName                                                              \
        : public AZStd::false_type                                                   \
    {                                                                                \
    };                                                                               \
    template<class T>                                                                \
    class Has##_HasName<T, AZStd::enable_if_t<AZStd::is_same_v<                      \
        _ResultType (AZStd::remove_cvref_t<T>::*) _ParamsSignature,                  \
         decltype(&AZStd::remove_cvref_t<T>::_FunctionName)>>>                       \
         : public AZStd::true_type                                                   \
    {                                                                                \
    };                                                                               \
    template <class T>                                                               \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;

#define AZ_HAS_STATIC_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature) \
    template<class T, class = void>                                                  \
    class Has##_HasName                                                              \
        : public AZStd::false_type                                                   \
    {                                                                                \
    };                                                                               \
    template<class T>                                                                \
    class Has##_HasName<T, AZStd::enable_if_t<AZStd::is_same_v<                      \
        _ResultType (*) _ParamsSignature,                                            \
        decltype(&AZStd::remove_cvref_t<T>::_FunctionName)>>>                        \
        : public AZStd::true_type                                                    \
    {                                                                                \
    };                                                                               \
    template<class T>                                                                \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value

