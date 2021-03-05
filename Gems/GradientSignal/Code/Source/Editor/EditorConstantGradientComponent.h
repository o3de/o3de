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
#include <Components/ConstantGradientComponent.h>

namespace GradientSignal
{
    class EditorConstantGradientComponent
        : public EditorGradientComponentBase<ConstantGradientComponent, ConstantGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<ConstantGradientComponent, ConstantGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorConstantGradientComponent, EditorConstantGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Constant Gradient";
        static constexpr const char* const s_componentDescription = "Returns a specified value as a gradient when sampled";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/constant-gradient";
    };
}
