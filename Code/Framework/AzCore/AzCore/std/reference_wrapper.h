/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        using type = T;
    };

    template<typename T>
    struct unwrap_reference<AZStd::reference_wrapper<T>>
    {
        using type = T&;
    };

    template<typename T>
    struct unwrap_ref_decay
        : unwrap_reference<AZStd::decay_t<T>>
    {
    };

    template<class T>
    using unwrap_reference_t = typename unwrap_reference<T>::type;

    template<class T>
    using unwrap_ref_decay_t = typename unwrap_ref_decay<T>::type;

#define AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(X) \
    template<typename T>                           \
    struct is_reference_wrapper< X >               \
        : public AZStd::true_type                  \
    {                                              \
    };


    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T>)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> volatile)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const volatile)
#undef AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF
}
