/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";

    protected:
        AZ::u32 ConfigurationChanged() override;
    };
}
