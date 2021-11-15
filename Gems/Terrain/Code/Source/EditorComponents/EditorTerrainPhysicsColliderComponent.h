/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainPhysicsColliderComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace Terrain
{
    struct DebugImageQuad
    {
        AZ::Vector3 m_point0;
        AZ::Vector3 m_point1;
        AZ::Vector3 m_point2;
        AZ::Vector3 m_point3;
    };

    class EditorTerrainPhysicsColliderComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainPhysicsColliderComponent, TerrainPhysicsColliderConfig>
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , protected AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainPhysicsColliderComponent, TerrainPhysicsColliderConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainPhysicsColliderComponent, "{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr auto s_categoryName = "Terrain";
        static constexpr auto s_componentName = "Terrain Physics Heightfield Collider";
        static constexpr auto s_componentDescription = "Provides terrain data to a physics collider in the form of a heightfield and surface->material mapping.";
        static constexpr auto s_icon = "Editor/Icons/Components/TerrainLayerSpawner.svg";
        static constexpr auto s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainLayerSpawner.svg";
        static constexpr auto s_helpUrl = "";

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    protected:

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AzFramework::Terrain::TerrainDataNotificationBus
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        bool m_visibleInEditor = true;
        AZ::Color m_drawColor = AzFramework::ViewportColors::WireColor;

        mutable AZStd::shared_mutex m_drawMutex;

        AZStd::vector<DebugImageQuad> m_debugQuads;

        void GenerateDebugDrawData();
    };
}
