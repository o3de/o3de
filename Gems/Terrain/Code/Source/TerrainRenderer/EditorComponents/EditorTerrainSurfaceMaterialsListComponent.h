/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainSurfaceMaterialsListComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainSurfaceMaterialsListComponent, TerrainSurfaceMaterialsListConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainSurfaceMaterialsListComponent, TerrainSurfaceMaterialsListConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainSurfaceMaterialsListComponent, "{335CDED5-2E76-4342-8675-A60F66C471BF}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Surface Materials List";
        static constexpr const char* const s_componentDescription = "Provides a mapping between surface tags and render materials.";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainHeight.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainHeight.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
