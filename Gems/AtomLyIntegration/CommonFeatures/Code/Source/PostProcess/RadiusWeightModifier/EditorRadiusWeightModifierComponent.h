/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorRadiusWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponent, RadiusWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponent, RadiusWeightModifierComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorRadiusWeightModifierComponent, EditorRadiusWeightModifierComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorRadiusWeightModifierComponent() = default;
            EditorRadiusWeightModifierComponent(const RadiusWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
