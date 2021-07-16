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
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConfig.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierController.h>

namespace AZ
{
    namespace Render
    {
        class GradientWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponentConfig>;
            AZ_COMPONENT(AZ::Render::GradientWeightModifierComponent, "{4DE2AD79-85BE-49FF-9DC5-D709720B013E}", BaseClass);

            GradientWeightModifierComponent() = default;
            GradientWeightModifierComponent(const GradientWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
