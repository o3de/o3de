/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainMacroMaterialComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainMacroMaterialComponent, TerrainMacroMaterialConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainMacroMaterialComponent, TerrainMacroMaterialConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainMacroMaterialComponent, "{24D87D5F-6845-4F1F-81DC-05B4CEBA3EF4}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Macro Material";
        static constexpr const char* const s_componentDescription = "Provides a macro material for a region to the terrain renderer";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainLayerRenderer.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainLayerRenderer.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
