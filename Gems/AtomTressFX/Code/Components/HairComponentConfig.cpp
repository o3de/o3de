/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Components/HairComponentConfig.h>
#include <Rendering/HairGlobalSettingsBus.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            void HairComponentConfig::Reflect(ReflectContext* context)
            {
                AMD::TressFXSimulationSettings::Reflect(context);
                AMD::TressFXRenderingSettings::Reflect(context);

                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponentConfig, ComponentConfig>()
                        ->Version(4)
                        ->Field("HairAsset", &HairComponentConfig::m_hairAsset)
                        ->Field("SimulationSettings", &HairComponentConfig::m_simulationSettings)
                        ->Field("RenderingSettings", &HairComponentConfig::m_renderingSettings)
                        ->Field("HairGlobalSettings", &HairComponentConfig::m_hairGlobalSettings)
                        ;
                }
            }

            void HairComponentConfig::OnHairGlobalSettingsChanged()
            {
                HairGlobalSettingsRequestBus::Broadcast(&HairGlobalSettingsRequests::SetHairGlobalSettings, m_hairGlobalSettings);
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ

