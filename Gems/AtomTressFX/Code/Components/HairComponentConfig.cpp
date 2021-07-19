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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Components/HairComponentConfig.h>

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
                        ;
                }
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ

