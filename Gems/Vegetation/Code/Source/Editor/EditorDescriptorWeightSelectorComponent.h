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

#include <Components/DescriptorWeightSelectorComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDescriptorWeightSelectorComponent
        : public EditorVegetationComponentBase<DescriptorWeightSelectorComponent, DescriptorWeightSelectorConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DescriptorWeightSelectorComponent, DescriptorWeightSelectorConfig>;
        AZ_EDITOR_COMPONENT(EditorDescriptorWeightSelectorComponent, EditorDescriptorWeightSelectorComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Asset Weight Selector";
        static constexpr const char* const s_componentDescription = "Selects vegetation assets based on their weight";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-asset-weight-selector";
    };
}
