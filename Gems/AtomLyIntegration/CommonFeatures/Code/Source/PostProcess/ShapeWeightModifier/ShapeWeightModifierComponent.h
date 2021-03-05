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
