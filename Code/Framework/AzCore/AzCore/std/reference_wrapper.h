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
#pragma once

#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/decay.h>
#include <functional>

namespace AZStd
{
    using std::reference_wrapper;
    using std::ref;
    using std::cref;
    
    template<typename T>
    class is_reference_wrapper
        : public AZStd::false_type
    {
    };

    template<class T>
    struct unwrap_reference
    {
        typedef typename AZStd::decay<T>::type type;
    };

#define AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(X) \
    template<typename T>                           \
    struct is_reference_wrapper< X >               \
        : public AZStd::true_type                  \
    {                                              \
    };                                             \
                                                   \
    template<typename T>                           \
    struct unwrap_reference< X >                   \
    {                                              \
        typedef T type;                            \
    };                                             \

    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T>)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> volatile)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const volatile)
#undef AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF
}
