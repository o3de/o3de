/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_TYPE_TRAITS_TYPEID_H
#define AZSTD_TYPE_TRAITS_TYPEID_H 1

#include <AzCore/std/base.h>

#if defined(AZ_COMPILER_MSVC)
// Some compilers have the type info even with rtti off. By default
// we don't expect people to run with rtti.
#define AZSTD_TYPE_ID_SUPPORT
#endif

#ifdef AZSTD_TYPE_ID_SUPPORT

#   include <typeinfo>

namespace AZStd
{
#       if defined(AZ_COMPILER_MSVC)
    typedef const ::type_info*      type_id;
#       else
    typedef const std::type_info*   type_id;
#       endif
}

#   define aztypeid(T) (&typeid(T))
#   define aztypeid_cmp(A, B) (*A) == (*B)
#else // AZSTD_TYPE_ID_SUPPORT

namespace AZStd
{
    typedef void*               type_id;
    template<class T>
    struct    type_id_holder
    {
        static char v_;
    };
    template<class T>
    char      type_id_holder< T >::v_;
    template<class T>
    struct    type_id_holder< T const >
        : type_id_holder< T >{};
    template<class T>
    struct    type_id_holder< T volatile >
        : type_id_holder< T >{};
    template<class T>
    struct    type_id_holder< T const volatile >
        : type_id_holder< T >{};
}

#   define aztypeid(T) (&AZStd::type_id_holder<T>::v_)
#   define aztypeid_cmp(A, B) (A) == (B)

#endif // AZSTD_TYPE_ID_SUPPORT

#endif // AZSTD_TYPE_TRAITS_TYPEID_H
#pragma once
