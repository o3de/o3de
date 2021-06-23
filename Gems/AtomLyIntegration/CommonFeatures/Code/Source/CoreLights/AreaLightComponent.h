/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
