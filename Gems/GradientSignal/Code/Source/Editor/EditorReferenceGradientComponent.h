/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/ReferenceGradientComponent.h>

namespace GradientSignal
{
    class EditorReferenceGradientComponent
        : public EditorGradientComponentBase<ReferenceGradientComponent, ReferenceGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<ReferenceGradientComponent, ReferenceGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorReferenceGradientComponent, "{93641FFD-8D9E-49B9-B8DF-1D943318B34A}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Reference Gradient";
        static constexpr const char* const s_componentDescription = "References another gradient";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
