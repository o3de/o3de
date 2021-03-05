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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetationfilters/vegetation-distance-between-filter";
    };
}
