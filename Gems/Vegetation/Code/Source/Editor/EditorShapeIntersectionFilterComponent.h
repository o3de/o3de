/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/ShapeIntersectionFilterComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorShapeIntersectionFilterComponent
        : public EditorVegetationComponentBase<ShapeIntersectionFilterComponent, ShapeIntersectionFilterConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<ShapeIntersectionFilterComponent, ShapeIntersectionFilterConfig>;
        AZ_EDITOR_COMPONENT(EditorShapeIntersectionFilterComponent, EditorShapeIntersectionFilterComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Filters";
        static constexpr const char* const s_componentName = "Vegetation Shape Intersection Filter";
        static constexpr const char* const s_componentDescription = "Enable or disable placing vegetation if the entity intersects a shape";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationFilter.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationFilter.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
