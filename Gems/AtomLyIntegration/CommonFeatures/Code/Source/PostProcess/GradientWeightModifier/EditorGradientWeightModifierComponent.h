/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorGradientWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponent, GradientWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponent, GradientWeightModifierComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorGradientWeightModifierComponent, EditorGradientWeightModifierComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorGradientWeightModifierComponent() = default;
            EditorGradientWeightModifierComponent(const GradientWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
