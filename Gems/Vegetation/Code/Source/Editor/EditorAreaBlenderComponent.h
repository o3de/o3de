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

#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <Components/AreaBlenderComponent.h>
#include <Vegetation/Editor/EditorAreaComponentBase.h>

namespace Vegetation
{
    class EditorAreaBlenderComponent
        : public EditorAreaComponentBase<AreaBlenderComponent, AreaBlenderConfig>
    {
    public:
        using DerivedClassType = EditorAreaBlenderComponent;
        using BaseClassType = EditorAreaComponentBase<AreaBlenderComponent, AreaBlenderConfig>;
        AZ_EDITOR_COMPONENT(EditorAreaBlenderComponent, EditorAreaBlenderComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        AZ::u32 ConfigurationChanged() override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Blender";
        static constexpr const char* const s_componentDescription = "Combines a collection of vegetation areas and applies them in a specified order";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-layer-blender";

    private:
        void ForceOneEntry();
    };
}
