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
#include <AzCore/Debug/Budget.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationMeshComponent::RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug)
        : m_meshConfig(config), m_showNavigationMesh(drawDebug)
    {
    }

    void RecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        RecastNavigationMeshConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationMeshComponent, AZ::Component>()
                ->Field("Config", &RecastNavigationMeshComponent::m_meshConfig)
                ->Field("Show NavMesh in Game", &RecastNavigationMeshComponent::m_showNavigationMesh)
                ->Version(1)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationMeshRequestBus>("RecastNavigationMeshRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Recast Navigation")
                ->Event("Update Navigation Mesh", &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted)
                ;

            behaviorContext->Class<RecastNavigationMeshComponent>()->RequestBus("RecastNavigationMeshRequestBus");

            behaviorContext->EBus<RecastNavigationMeshNotificationBus>("RecastNavigationMeshNotificationBus")
                ->Handler<RecastNavigationNotificationHandler>();
        }
    }

    void RecastNavigationMeshComponent::UpdateNavigationMeshBlockUntilCompleted()
    {
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        // Blocking call.
        RecastNavigationProviderRequestBus::EventResult(tiles, GetEntityId(),
            &RecastNavigationProviderRequests::CollectGeometry,
            m_meshConfig.m_tileSize, aznumeric_cast<float>(m_meshConfig.m_borderSize) * m_meshConfig.m_cellSize);

        for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
        {
            if (tile->IsEmpty())
            {
                continue;
            }

            // Given geometry create Recast tile structure.
            NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                m_meshConfig, m_context.get());

            // If a tile at the location already exists, remove it before update the data.
            if (const dtTileRef tileRef = m_navObject->m_mesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
            {
                m_navObject->m_mesh->removeTile(tileRef, nullptr, nullptr);
            }

            // A tile might have no geometry at all if no objects were found there.
            if (navigationTileData.IsValid())
            {
                AttachNavigationTileToMesh(navigationTileData);
            }
        }

        RecastNavigationMeshNotificationBus::Event(GetEntityId(), &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, GetEntityId());
    }

    AZStd::shared_ptr<NavMeshQuery> RecastNavigationMeshComponent::GetNavigationObject()
    {
        return m_navObject;
    }

    void RecastNavigationMeshComponent::Activate()
    {
        m_context = AZStd::make_unique<rcContext>();

        // It is safe to create the navigation mesh object now.
        // The actual navigation data will be passed at a later time.
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
        m_navObject = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnDebugDrawTick()
    {
        if (m_navObject && m_navObject->m_mesh && (cl_navmesh_debug || m_showNavigationMesh))
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navObject->m_mesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }
} // namespace RecastNavigation
