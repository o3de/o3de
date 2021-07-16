/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/ScaleModifierComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorScaleModifierComponent
        : public EditorVegetationComponentBase<ScaleModifierComponent, ScaleModifierConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<ScaleModifierComponent, ScaleModifierConfig>;
        AZ_EDITOR_COMPONENT(EditorScaleModifierComponent, EditorScaleModifierComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Modifiers";
        static constexpr const char* const s_componentName = "Vegetation Scale Modifier";
        static constexpr const char* const s_componentDescription = "Offsets the scale of the vegetation";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetatioModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
