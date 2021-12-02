/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/DescriptorListComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorDescriptorListComponent
        : public EditorVegetationComponentBase<DescriptorListComponent, DescriptorListConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<DescriptorListComponent, DescriptorListConfig>;
        AZ_EDITOR_COMPONENT(EditorDescriptorListComponent, "{3AF9BE58-6D2D-44FB-AB4D-CA1182F6C78F}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        AZ::u32 ConfigurationChanged() override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Asset List";
        static constexpr const char* const s_componentDescription = "Provides a set of vegetation descriptors";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";

    private:
        void ForceOneEntry();
    };
}
