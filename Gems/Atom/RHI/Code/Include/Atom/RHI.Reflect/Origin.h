/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
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
