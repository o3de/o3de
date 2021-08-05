/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorShapeWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponent, ShapeWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponent, ShapeWeightModifierComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorShapeWeightModifierComponent, EditorShapeWeightModifierComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorShapeWeightModifierComponent() = default;
            EditorShapeWeightModifierComponent(const ShapeWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
