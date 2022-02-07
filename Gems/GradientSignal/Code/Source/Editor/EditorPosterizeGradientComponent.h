/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/PosterizeGradientComponent.h>

namespace GradientSignal
{
    class EditorPosterizeGradientComponent
        : public EditorGradientComponentBase<PosterizeGradientComponent, PosterizeGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<PosterizeGradientComponent, PosterizeGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorPosterizeGradientComponent, EditorPosterizeGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Posterize Gradient Modifier";
        static constexpr const char* const s_componentDescription = "Divides an input gradient's signal into a specified number of bands";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
    };
}
