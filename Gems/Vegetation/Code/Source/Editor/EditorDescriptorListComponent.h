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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-asset-list";

    private:
        void ForceOneEntry();
    };
}
