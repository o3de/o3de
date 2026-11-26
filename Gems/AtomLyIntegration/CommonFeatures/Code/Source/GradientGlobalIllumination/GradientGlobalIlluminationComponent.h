/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientGlobalIllumination/GradientGlobalIlluminationComponentController.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ::Render
{
    class GradientGlobalIlluminationComponent final
        : public AzFramework::Components::ComponentAdapter<GradientGlobalIlluminationComponentController, GradientGlobalIlluminationComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<GradientGlobalIlluminationComponentController, GradientGlobalIlluminationComponentConfig>;
        AZ_COMPONENT(AZ::Render::GradientGlobalIlluminationComponent, GradientGlobalIlluminationComponentTypeId, BaseClass);

        GradientGlobalIlluminationComponent() = default;
        GradientGlobalIlluminationComponent(const GradientGlobalIlluminationComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AZ::Render
