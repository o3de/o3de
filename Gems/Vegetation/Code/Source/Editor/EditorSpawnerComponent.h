/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <Components/SpawnerComponent.h>
#include <Vegetation/Editor/EditorAreaComponentBase.h>

namespace Vegetation
{
    class EditorSpawnerComponent
        : public EditorAreaComponentBase<SpawnerComponent, SpawnerConfig>
   {
    public:
        using DerivedClassType = EditorSpawnerComponent;
        using BaseClassType = EditorAreaComponentBase<SpawnerComponent, SpawnerConfig>;
        AZ_EDITOR_COMPONENT(EditorSpawnerComponent, EditorSpawnerComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Spawner";
        static constexpr const char* const s_componentDescription = "Creates dynamic vegetation in a specified area";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/vegetation-layer-spawner/";
    };
}
