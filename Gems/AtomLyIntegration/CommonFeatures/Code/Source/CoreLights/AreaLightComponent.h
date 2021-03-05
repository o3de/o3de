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

#include <AzFramework/Components/ComponentAdapter.h>
#include <CoreLights/AreaLightComponentController.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        class AreaLightComponent final
            : public AzFramework::Components::ComponentAdapter<AreaLightComponentController, AreaLightComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<AreaLightComponentController, AreaLightComponentConfig>;
            AZ_COMPONENT(AZ::Render::AreaLightComponent, AreaLightComponentTypeId, BaseClass);

            AreaLightComponent() = default;
            AreaLightComponent(const AreaLightComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
