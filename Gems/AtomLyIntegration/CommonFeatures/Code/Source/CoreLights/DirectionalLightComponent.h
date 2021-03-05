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

#include <CoreLights/DirectionalLightComponentController.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightComponentConfig.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class DirectionalLightComponent final
            : public AzFramework::Components::ComponentAdapter<DirectionalLightComponentController, DirectionalLightComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DirectionalLightComponentController, DirectionalLightComponentConfig>;

            AZ_COMPONENT(AZ::Render::DirectionalLightComponent, DirectionalLightComponentTypeId);

            DirectionalLightComponent() = default;
            DirectionalLightComponent(const DirectionalLightComponentConfig& config);
            virtual ~DirectionalLightComponent() override = default;

            static void Reflect(ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
