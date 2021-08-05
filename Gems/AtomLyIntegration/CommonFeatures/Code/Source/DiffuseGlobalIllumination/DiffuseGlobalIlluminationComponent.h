/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentConfig.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentConstants.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentController.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseGlobalIlluminationComponent final
            : public AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>;
            AZ_COMPONENT(AZ::Render::DiffuseGlobalIlluminationComponent, DiffuseGlobalIlluminationComponentTypeId , BaseClass);

            DiffuseGlobalIlluminationComponent() = default;
            DiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
