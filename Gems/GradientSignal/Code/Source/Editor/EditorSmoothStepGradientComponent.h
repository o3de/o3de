/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/SmoothStepGradientComponent.h>

namespace GradientSignal
{
    class EditorSmoothStepGradientComponent
        : public EditorGradientComponentBase<SmoothStepGradientComponent, SmoothStepGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<SmoothStepGradientComponent, SmoothStepGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorSmoothStepGradientComponent, EditorSmoothStepGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Smooth-Step Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Generates a gradient with fall off, which creates a smoother input gradient";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
