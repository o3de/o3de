/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Rendering/HairLightingModels.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            struct HairGlobalSettings
            {
                AZ_TYPE_INFO(AZ::Render::Hair::HairGlobalSettings, "{B4175C42-9F4D-4824-9563-457A84C4983D}");

                static void Reflect(ReflectContext* context);

                HairLightingModel m_hairLightingModel = HairLightingModel::Marschner;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
