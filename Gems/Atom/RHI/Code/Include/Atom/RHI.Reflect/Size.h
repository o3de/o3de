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

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        struct Size
        {
            AZ_TYPE_INFO(Size, "{3B8DAD61-8AFA-4BB1-BCF8-179865D8C57B}");

            static void Reflect(AZ::ReflectContext* context);

            Size() = default;
            Size(uint32_t width, uint32_t height, uint32_t depth);

            /**
             * Returns the mip level size, assuming this size is mip 0. A value of 1 is
             * half sized, 2 quarter sized, etc. Clamps at 1.
             */
            Size GetReducedMip(uint32_t mipLevel) const;

            uint32_t m_width = 1;
            uint32_t m_height = 1;
            uint32_t m_depth = 1;

            bool operator == (const Size& rhs) const;
            bool operator != (const Size& rhs) const;

            uint32_t& operator [] (uint32_t idx);
            uint32_t operator [] (uint32_t idx) const;
        };
    }
}
