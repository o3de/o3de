/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/InvertGradientComponent.h>

namespace GradientSignal
{
    class EditorInvertGradientComponent
        : public EditorGradientComponentBase<InvertGradientComponent, InvertGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<InvertGradientComponent, InvertGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorInvertGradientComponent, EditorInvertGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Invert Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Inverts a gradient's values";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
