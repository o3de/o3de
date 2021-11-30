/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationModifier.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
