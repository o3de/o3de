/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";

    private:
        void ForceOneEntry();
    };
}
