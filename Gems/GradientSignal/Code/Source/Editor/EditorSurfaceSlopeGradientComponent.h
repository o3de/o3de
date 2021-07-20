/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/SurfaceSlopeGradientComponent.h>

namespace GradientSignal
{
    class EditorSurfaceSlopeGradientComponent
        : public EditorGradientComponentBase<SurfaceSlopeGradientComponent, SurfaceSlopeGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<SurfaceSlopeGradientComponent, SurfaceSlopeGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorSurfaceSlopeGradientComponent, EditorSurfaceSlopeGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Slope Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient based on surface angle";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
