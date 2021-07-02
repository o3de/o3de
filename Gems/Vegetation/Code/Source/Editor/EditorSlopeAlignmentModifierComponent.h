/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/SlopeAlignmentModifierComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorSlopeAlignmentModifierComponent
        : public EditorVegetationComponentBase<SlopeAlignmentModifierComponent, SlopeAlignmentModifierConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<SlopeAlignmentModifierComponent, SlopeAlignmentModifierConfig>;
        AZ_EDITOR_COMPONENT(EditorSlopeAlignmentModifierComponent, EditorSlopeAlignmentModifierComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Modifiers";
        static constexpr const char* const s_componentName = "Vegetation Slope Alignment Modifier";
        static constexpr const char* const s_componentDescription = "Offsets the orientation of the vegetation relative to a surface angle";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
