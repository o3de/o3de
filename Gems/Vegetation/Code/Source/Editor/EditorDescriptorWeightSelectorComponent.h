/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
