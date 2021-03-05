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
