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
        static constexpr const char* const s_icon = "Editor/Icons/Components/Box_Shape.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Box_Shape.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/ocean-surface-tag-emitter";
    };
}
