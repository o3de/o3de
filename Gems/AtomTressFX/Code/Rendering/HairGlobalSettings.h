/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            //! Used by all hair components to control the shader options flags used by the hair
            //!  rendering for lighting and various display features such as the Marschner lighting
            //!  model components.
            struct HairGlobalSettings
            {
                AZ_TYPE_INFO(AZ::Render::Hair::HairGlobalSettings, "{B4175C42-9F4D-4824-9563-457A84C4983D}");

                static void Reflect(ReflectContext* context);

                bool m_enableShadows = true;
                bool m_enableDirectionalLights = true;
                bool m_enablePunctualLights = true;
                bool m_enableAreaLights = true;
                bool m_enableIBL = true;
                HairLightingModel m_hairLightingModel = HairLightingModel::Marschner;
                bool m_enableMarschner_R = true;
                bool m_enableMarschner_TRT = true;
                bool m_enableMarschner_TT = true;
                bool m_enableLongtitudeCoeff = true;
                bool m_enableAzimuthCoeff = true;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
