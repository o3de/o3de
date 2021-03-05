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
    /// Implements the C++20 std::identity type trait
    /// Note: The member type is_transparent indicates to the caller that this function object is a transparent function object:
    /// it accepts arguments of arbitrary types and uses perfect forwarding, which avoids unnecessary copying and conversion
    /// when the function object is used in heterogeneous context, or with rvalue arguments.
    /// In particular, template functions such as std::set::find and std::set::lower_bound make use of this member type on their Compare types.
    struct identity
    {
        using is_transparent = void;

        template <typename T>
        T&& operator()(T&& t) const
        {
            return AZStd::forward<T>(t);
        }
    };
}
