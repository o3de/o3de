/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainLayerSpawnerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainLayerSpawnerComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainLayerSpawnerComponent, TerrainLayerSpawnerConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainLayerSpawnerComponent, TerrainLayerSpawnerConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainLayerSpawnerComponent, "{9403FC94-FA38-4387-BEFD-A728C7D850C1}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Layer Spawner";
        static constexpr const char* const s_componentDescription = "Defines a terrain region for use by the terrain system";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainLayerSpawner.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainLayerSpawner.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/terrain/layer_spawner/";
    };
}
