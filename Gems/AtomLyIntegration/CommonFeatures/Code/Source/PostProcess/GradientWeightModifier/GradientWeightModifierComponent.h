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

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConfig.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierComponentController.h>

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
