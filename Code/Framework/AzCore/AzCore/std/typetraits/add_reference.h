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
#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    using std::add_lvalue_reference;
    template< class T >
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    using std::add_rvalue_reference;
    template< class T >
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;
}
