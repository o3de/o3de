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
#ifndef AZSTD_SP_CONVERTIBLE_DELETE_H
#define AZSTD_SP_CONVERTIBLE_DELETE_H

#include <AzCore/std/base.h>

namespace AZStd
{
    namespace Internal
    {
        // sp_convertible
        template< class Y, class T >
        struct sp_convertible
        {
            typedef char (& yes) [1];
            typedef char (& no)  [2];

            static yes f(T*);
            static no  f(...);

            enum _vt
            {
                value = sizeof((f)(static_cast<Y*>(0))) == sizeof(yes)
            };
        };
        struct sp_empty {};

        template< bool >
        struct sp_enable_if_convertible_impl;
        template<>
        struct sp_enable_if_convertible_impl<true>
        {
            typedef sp_empty type;
        };
        template<>
        struct sp_enable_if_convertible_impl<false>  {};

        template< class Y, class T >
        struct sp_enable_if_convertible
            : public sp_enable_if_convertible_impl< sp_convertible< Y, T >::value >
        {};
    }
} // namespace AZStd

#endif  // #ifndef AZSTD_SP_CONVERTIBLE_DELETE_H
#pragma once
