/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainHeightGradientListComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainHeightGradientListComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainHeightGradientListComponent, TerrainHeightGradientListConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainHeightGradientListComponent, TerrainHeightGradientListConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainHeightGradientListComponent, "{2D945B90-ADAB-4F9A-A113-39E714708068}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Height Gradient List";
        static constexpr const char* const s_componentDescription = "Provides height data for a region to the terrain system";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainHeight.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainHeight.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/terrain/height_gradient_list/";
    };
}
