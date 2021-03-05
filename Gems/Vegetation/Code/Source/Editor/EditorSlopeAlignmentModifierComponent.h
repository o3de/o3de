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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetationmodifiers/vegetation-slope-alignment-modifier";
    };
}
