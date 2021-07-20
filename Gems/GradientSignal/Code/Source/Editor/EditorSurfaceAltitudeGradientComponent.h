/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/SurfaceAltitudeGradientComponent.h>

namespace GradientSignal
{
    class EditorSurfaceAltitudeGradientComponent
        : public EditorGradientComponentBase<SurfaceAltitudeGradientComponent, SurfaceAltitudeGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<SurfaceAltitudeGradientComponent, SurfaceAltitudeGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorSurfaceAltitudeGradientComponent, EditorSurfaceAltitudeGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Altitude Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient based on height within a range";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        AZ::u32 ConfigurationChanged() override;

    private:
        void UpdateFromShape();
    };
}
