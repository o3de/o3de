/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainSurfaceGradientListComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainSurfaceGradientListComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainSurfaceGradientListComponent, TerrainSurfaceGradientListConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainSurfaceGradientListComponent, TerrainSurfaceGradientListConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainSurfaceGradientListComponent, "{49831E91-A11F-4EFF-A824-6D85C284B934}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Surface Gradient List";
        static constexpr const char* const s_componentDescription = "Provides a mapping between gradients and surface tags for use by the terrain system.";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainLayerSpawner.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainLayerSpawner.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
