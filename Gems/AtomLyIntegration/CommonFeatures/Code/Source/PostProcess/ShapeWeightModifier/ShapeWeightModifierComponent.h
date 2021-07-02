/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConfig.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentController.h>

namespace AZ
{
    namespace Render
    {
        class ShapeWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponentConfig>;
            AZ_COMPONENT(AZ::Render::ShapeWeightModifierComponent, "{0BB6438B-6DD1-4A09-927F-7757D39C940D}", BaseClass);

            ShapeWeightModifierComponent() = default;
            ShapeWeightModifierComponent(const ShapeWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
