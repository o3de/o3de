/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");

namespace RecastNavigation
{
    RecastNavigationMeshComponent::RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug)
        : m_meshConfig(config), m_showNavigationMesh(drawDebug)
    {
    }

    void RecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        RecastNavigationMeshConfig::Reflect(context);

        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationMeshComponent, AZ::Component>()
                ->Field("Config", &RecastNavigationMeshComponent::m_meshConfig)
                ->Field("Show NavMesh in Game", &RecastNavigationMeshComponent::m_showNavigationMesh)
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationMeshRequestBus>("RecastNavigationMeshRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Recast Navigation")
                ->Event("UpdateNavigationMesh", &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted)
                ;

            behaviorContext->Class<RecastNavigationMeshComponent>()->RequestBus("RecastNavigationMeshRequestBus");

            behaviorContext->EBus<RecastNavigationMeshNotificationBus>("RecastNavigationMeshNotificationBus")
                ->Handler<RecastNavigationNotificationHandler>();
        }
    }

    void RecastNavigationMeshComponent::UpdateNavigationMeshBlockUntilCompleted()
    {
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        RecastNavigationSurveyorRequestBus::EventResult(tiles, GetEntityId(),
            &RecastNavigationSurveyorRequests::CollectGeometry,
            m_meshConfig.m_tileSize, m_meshConfig.m_borderSize * m_meshConfig.m_cellSize);

        for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
        {
            if (tile->IsEmpty())
            {
                continue;
            }

            NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                m_meshConfig, m_context.get());

            m_navMesh->removeTile(m_navMesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0), nullptr, nullptr);

            if (navigationTileData.IsValid())
            {
                AttachNavigationTileToMesh(navigationTileData);
            }
        }

        RecastNavigationMeshNotificationBus::Broadcast(&RecastNavigationMeshNotifications::OnNavigationMeshUpdated, GetEntityId());
    }

    void RecastNavigationMeshComponent::Activate()
    {
        m_context = AZStd::make_unique<RecastCustomContext>();

        CreateNavigationMesh(GetEntityId(), m_meshConfig.m_tileSize);

        if (cl_navmesh_debug || m_showNavigationMesh)
        {
            m_tickEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        }

        RecastNavigationMeshRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationMeshComponent::Deactivate()
    {
        m_tickEvent.RemoveFromQueue();

        m_context = {};
        m_navQuery = {};
        m_navMesh = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnDebugDrawTick()
    {
        if (m_navMesh && (cl_navmesh_debug || m_showNavigationMesh))
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }
} // namespace RecastNavigation
