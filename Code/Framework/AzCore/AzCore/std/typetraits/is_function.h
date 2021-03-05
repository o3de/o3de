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

