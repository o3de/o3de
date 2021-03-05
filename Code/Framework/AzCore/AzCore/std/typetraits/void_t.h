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

namespace AZStd
{
    // Implementation of the C++ standard is void_t meta function
    // http://en.cppreference.com/w/cpp/types/void_t
    // This utility function maps any type to void
    // It can be used to detect ill-formed which are not mappable to void
    template<typename... Args> struct make_void { using type = void; };
    template<typename... Args> using void_t = typename make_void<Args...>::type;
}