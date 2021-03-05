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
#include <AtomLyIntegration/CommonFeatures/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConfig.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentController.h>

namespace AZ
{
    namespace Render
    {
        class RadiusWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponentConfig>;
            AZ_COMPONENT(AZ::Render::RadiusWeightModifierComponent, "{8F0FC718-50E1-425E-A1E7-9C0425879CEB}", BaseClass);

            RadiusWeightModifierComponent() = default;
            RadiusWeightModifierComponent(const RadiusWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
