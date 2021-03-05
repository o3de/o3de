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
        struct Origin
        {
            AZ_TYPE_INFO(Origin, "{323EB88E-A2BF-421F-9CD4-B668CA246AAF}");

            static void Reflect(AZ::ReflectContext* context);

            Origin() = default;
            Origin(uint32_t left, uint32_t top, uint32_t front);

            uint32_t m_left = 0;
            uint32_t m_top = 0;
            uint32_t m_front = 0;

            bool operator == (const Origin& rhs) const;
            bool operator != (const Origin& rhs) const;
        };
    }
}
