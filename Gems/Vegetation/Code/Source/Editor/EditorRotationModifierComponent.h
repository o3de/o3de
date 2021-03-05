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

#include <Components/RotationModifierComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorRotationModifierComponent
        : public EditorVegetationComponentBase<RotationModifierComponent, RotationModifierConfig>
    {
    public:
        using DerivedClassType = EditorRotationModifierComponent;
        using BaseClassType = EditorVegetationComponentBase<RotationModifierComponent, RotationModifierConfig>;
        AZ_EDITOR_COMPONENT(EditorRotationModifierComponent, EditorRotationModifierComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;

        static constexpr const char* const s_categoryName = "Vegetation Modifiers";
        static constexpr const char* const s_componentName = "Vegetation Rotation Modifier";
        static constexpr const char* const s_componentDescription = "Offsets the rotation of the vegetation";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationModifier.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetationmodifiers/vegetation-rotation-modifier";
    };
}
