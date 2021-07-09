/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
