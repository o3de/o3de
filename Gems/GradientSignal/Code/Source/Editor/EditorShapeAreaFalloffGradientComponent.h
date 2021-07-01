/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/ShapeAreaFalloffGradientComponent.h>

namespace GradientSignal
{
    class EditorShapeAreaFalloffGradientComponent
        : public EditorGradientComponentBase<ShapeAreaFalloffGradientComponent, ShapeAreaFalloffGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<ShapeAreaFalloffGradientComponent, ShapeAreaFalloffGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorShapeAreaFalloffGradientComponent, EditorShapeAreaFalloffGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Shape Falloff Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient based on distance from a shape";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
