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

namespace TestImpact
{
    //! Convinience namespace for allowing bitwise operations on enum classes.
    //! @note Any types declared in this namespace will have the bitwise operator overloads declared within.
    namespace Bitwise
    {
        template<typename Flags>
        Flags operator|(Flags lhs, Flags rhs)
        {
            return static_cast<Flags>(
                static_cast<std::underlying_type<Flags>::type>(lhs) | static_cast<std::underlying_type<Flags>::type>(rhs));
        }

        template<typename Flags>
        Flags& operator |=(Flags& lhs, Flags rhs)
        {
            lhs = lhs | rhs;
            return lhs;
        }

        template<typename Flags>
        bool IsFlagSet(Flags flags, Flags flag)
        {
            return static_cast<bool>(
                static_cast<std::underlying_type<Flags>::type>(flags) & static_cast<std::underlying_type<Flags>::type>(flag));
        }
    } // namespace Bitwise
} // namespace TestImpact
