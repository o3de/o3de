/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/remove_pointer.h>

namespace AZStd
{
    using std::is_function;
    using std::is_function_v;

    template<class T>
    struct is_function_pointer
        : AZStd::conjunction<AZStd::is_pointer<T>, AZStd::is_function<AZStd::remove_pointer_t<T>>>
    {};

    template<class T>
    constexpr bool is_function_pointer_v = is_function_pointer<T>::value;
}

