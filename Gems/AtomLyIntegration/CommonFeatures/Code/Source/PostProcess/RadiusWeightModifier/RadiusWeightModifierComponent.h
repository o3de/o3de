/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
