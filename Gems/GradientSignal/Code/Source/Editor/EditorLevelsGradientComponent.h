/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/LevelsGradientComponent.h>

namespace GradientSignal
{
    class EditorLevelsGradientComponent
        : public EditorGradientComponentBase<LevelsGradientComponent, LevelsGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<LevelsGradientComponent, LevelsGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorLevelsGradientComponent, EditorLevelsGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Levels Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Modifies an input gradient's signal using low/mid/high points and allows clamping of min/max output values";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
