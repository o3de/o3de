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
#include <Components/DitherGradientComponent.h>

namespace GradientSignal
{
    class EditorDitherGradientComponent
        : public EditorGradientComponentBase<DitherGradientComponent, DitherGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<DitherGradientComponent, DitherGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorDitherGradientComponent, EditorDitherGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Dither Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Applies ordered dithering to the input gradient";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradientmodifiers/dither-gradient-modifier";

    protected:
        AZ::u32 ConfigurationChanged() override;
    };
}
