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
    using std::conditional;
    using std::conditional_t;

    using std::enable_if;
    using std::enable_if_t;
} // namespace AZStd

// Backwards comaptible AZStd extensions for std::conditional and std::enable_if
namespace AZStd::Utils
{
    // Use AZStd::conditional_t instead
    template<bool Condition, typename T, typename F>
    using if_c = AZStd::conditional<Condition, T, F>;
    // Use AZStd::enable_if_t instead
    template<bool Condition, typename T = void>
    using enable_if_c = AZStd::enable_if<Condition, T>;
}
