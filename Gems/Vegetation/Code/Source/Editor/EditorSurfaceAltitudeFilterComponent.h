/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/SurfaceAltitudeFilterComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorSurfaceAltitudeFilterComponent
        : public EditorVegetationComponentBase<SurfaceAltitudeFilterComponent, SurfaceAltitudeFilterConfig>
    {
    public:
        using DerivedClassType = EditorSurfaceAltitudeFilterComponent;
        using BaseClassType = EditorVegetationComponentBase<SurfaceAltitudeFilterComponent, SurfaceAltitudeFilterConfig>;
        AZ_EDITOR_COMPONENT(EditorSurfaceAltitudeFilterComponent, EditorSurfaceAltitudeFilterComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Filters";
        static constexpr const char* const s_componentName = "Vegetation Altitude Filter";
        static constexpr const char* const s_componentDescription = "Limits vegetation to only place within the specified height range";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationFilter.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationFilter.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";

        AZ::u32 ConfigurationChanged() override;
    };
}
