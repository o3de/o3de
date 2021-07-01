/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/HDRiSkyboxComponentConfig.h>
#include <SkyBox/HDRiSkyboxComponentController.h>

namespace AZ
{
    namespace Render
    {
        class HDRiSkyboxComponent final
            : public AzFramework::Components::ComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponentConfig>;
            AZ_COMPONENT(AZ::Render::HDRiSkyboxComponent, HDRiSkyboxComponentTypeId , BaseClass);

            HDRiSkyboxComponent() = default;
            HDRiSkyboxComponent(const HDRiSkyboxComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
