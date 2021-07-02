/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <AtomLyIntegration/CommonFeatures/SkyBox/PhysicalSkyComponentConfig.h>
#include <SkyBox/PhysicalSkyComponentController.h>

namespace AZ
{
    namespace Render
    {
        class PhysicalSkyComponent final
            : public AzFramework::Components::ComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponentConfig>;
            AZ_COMPONENT(AZ::Render::PhysicalSkyComponent, PhysicalSkyComponentTypeId , BaseClass);

            PhysicalSkyComponent() = default;
            PhysicalSkyComponent(const PhysicalSkyComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
