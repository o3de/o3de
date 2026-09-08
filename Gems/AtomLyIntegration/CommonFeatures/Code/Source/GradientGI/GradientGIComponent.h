/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientGI/GradientGIComponentController.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class GradientGIComponent final
            : public AzFramework::Components::ComponentAdapter<GradientGIComponentController, GradientGIComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<GradientGIComponentController, GradientGIComponentConfig>;
            AZ_COMPONENT(AZ::Render::GradientGIComponent, GradientGIComponentTypeId, BaseClass);

            GradientGIComponent() = default;
            GradientGIComponent(const GradientGIComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
