/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Render
    {
        struct SkyBoxFogSettings final
        {
            AZ_RTTI(AZ::Render::SkyBoxFogSettings, "{DB13027C-BA92-4E46-B428-BB77C2A80C51}");
            
            static void Reflect(ReflectContext* context);

            SkyBoxFogSettings() = default;

            bool IsFogDisabled() const;

            AZ::Color m_color = AZ::Color::CreateOne();
            bool m_enable = false;
            float m_topHeight = 0.01;
            float m_bottomHeight = 0.0;
        };
    }
}
