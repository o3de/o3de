/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZSTD_TYPE_TRAITS_HAS_MEMBER_FUNCTION_INCLUDED
#define AZSTD_TYPE_TRAITS_HAS_MEMBER_FUNCTION_INCLUDED

#include <AzCore/std/typetraits/is_class.h>

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
#define AZ_HAS_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)                                      \
    template<bool V>                                                                                               \
    struct _HasName##ResultType { typedef AZStd::true_type type; };                                                \
    template<>                                                                                                     \
    struct _HasName##ResultType<false> { typedef AZStd::false_type type; };                                        \
    template<class T, bool isClass = AZStd::is_class<T>::value >                                                   \
    class Has##_HasName {                                                                                          \
        typedef char Yes;                                                                                          \
        typedef long No;                                                                                           \
        template<class U, U>                                                                                       \
        struct TypeCheck;                                                                                          \
        template<class U>                                                                                          \
        struct Helper { typedef _ResultType (U::* mfp) _ParamsSignature; };                                        \
        template<class C>                                                                                          \
        static Yes ClassCheck(TypeCheck< typename Helper<C>::mfp, & C::_FunctionName>*);                           \
        template<class C>                                                                                          \
        static No  ClassCheck(...);                                                                                \
    public:                                                                                                        \
        static const bool value = (sizeof(ClassCheck< typename AZStd::remove_const<T>::type >(0)) == sizeof(Yes)); \
        typedef typename _HasName##ResultType<value>::type type;                                                   \
    };                                                                                                             \
    template<class T >                                                                                             \
    struct Has##_HasName<T, false> {                                                                               \
        static const bool value = false;                                                                           \
        typedef AZStd::false_type type;                                                                            \
    };

#define AZ_HAS_STATIC_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)                               \
    template<bool V, bool dummy = true>                                                                            \
    struct _HasName##ResultType { typedef AZStd::true_type type; };                                                \
    template<bool dummy>                                                                                           \
    struct _HasName##ResultType<false, dummy> { typedef AZStd::false_type type; };                                 \
    template<class T, bool isClass = AZStd::is_class<T>::value >                                                   \
    class Has##_HasName {                                                                                          \
        typedef char Yes;                                                                                          \
        typedef long No;                                                                                           \
        template<class U, U>                                                                                       \
        struct TypeCheck;                                                                                          \
        template<class U>                                                                                          \
        struct Helper { typedef _ResultType (* mfp) _ParamsSignature; };                                           \
        template<class C>                                                                                          \
        static Yes ClassCheck(TypeCheck< typename Helper<C>::mfp, & C::_FunctionName>*);                           \
        template<class C>                                                                                          \
        static No  ClassCheck(...);                                                                                \
    public:                                                                                                        \
        static const bool value = (sizeof(ClassCheck< typename AZStd::remove_const<T>::type >(0)) == sizeof(Yes)); \
        typedef typename _HasName##ResultType<value>::type type;                                                   \
    };                                                                                                             \
    template<class T >                                                                                             \
    struct Has##_HasName<T, false> {                                                                               \
        static const bool value = false;                                                                           \
        typedef AZStd::false_type type;                                                                            \
    };

#endif // AZSTD_TYPE_TRAITS_HAS_MEMBER_FUNCTION_INCLUDED
#pragma once
