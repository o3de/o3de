/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/DistanceBetweenFilterComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDistanceBetweenFilterComponent
        : public EditorVegetationComponentBase<DistanceBetweenFilterComponent, DistanceBetweenFilterConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DistanceBetweenFilterComponent, DistanceBetweenFilterConfig>;
        AZ_EDITOR_COMPONENT(EditorDistanceBetweenFilterComponent, EditorDistanceBetweenFilterComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Filters";
        static constexpr const char* const s_componentName = "Vegetation Distance Between Filter";
        static constexpr const char* const s_componentDescription = "Defines the minimum distance required between vegetation instances";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationFilter.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationFilter.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
