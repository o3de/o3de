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

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        struct Interval
        {
            AZ_TYPE_INFO(Interval, "{B121C9FE-1C23-4721-9C3E-6BE036612743}");
            static void Reflect(AZ::ReflectContext* context);

            Interval() = default;
            Interval(uint32_t min, uint32_t max);

            uint32_t m_min = 0;
            uint32_t m_max = 0;

            bool operator == (const Interval& rhs) const;
            bool operator != (const Interval& rhs) const;
        };
    }
}