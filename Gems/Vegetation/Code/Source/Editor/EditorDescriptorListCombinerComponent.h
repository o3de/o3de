/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/DescriptorListCombinerComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDescriptorListCombinerComponent
        : public EditorVegetationComponentBase<DescriptorListCombinerComponent, DescriptorListCombinerConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DescriptorListCombinerComponent, DescriptorListCombinerConfig>;
        AZ_EDITOR_COMPONENT(EditorDescriptorListCombinerComponent, "{7477D3C6-D459-4141-AB0D-15E880341C96}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Asset List Combiner";
        static constexpr const char* const s_componentDescription = "Provides a list of vegetation descriptor providers";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
