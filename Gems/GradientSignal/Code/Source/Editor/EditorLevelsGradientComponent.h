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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradientmodifiers/levels-gradient-modifier";
    };
}