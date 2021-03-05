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
#include <Components/SurfaceMaskGradientComponent.h>

namespace GradientSignal
{
    class EditorSurfaceMaskGradientComponent
        : public EditorGradientComponentBase<SurfaceMaskGradientComponent, SurfaceMaskGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<SurfaceMaskGradientComponent, SurfaceMaskGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorSurfaceMaskGradientComponent, EditorSurfaceMaskGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Surface Mask Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient based on underlying surface types";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/gradients/surface-mask-gradient";
    };
}
