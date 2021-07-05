/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/ThresholdGradientComponent.h>

namespace GradientSignal
{
    class EditorThresholdGradientComponent
        : public EditorGradientComponentBase<ThresholdGradientComponent, ThresholdGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<ThresholdGradientComponent, ThresholdGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorThresholdGradientComponent, EditorThresholdGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Threshold Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Converts input gradient to be 0 if below the threshold or 1 if above the threshold";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
