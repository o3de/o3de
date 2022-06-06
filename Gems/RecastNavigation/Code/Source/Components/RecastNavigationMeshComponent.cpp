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
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");
AZ_CVAR(
    float, cl_navmesh_debugRadius, 25.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Limit debug draw to within a specified distance from the active camera");

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationMeshComponent::RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug)
        : m_meshConfig(config)
        , m_showNavigationMesh(drawDebug)
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
                ->Event("Update Navigation Mesh Async", &RecastNavigationMeshRequests::UpdateNavigationMeshAsync)
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

        {

            for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
            {
                if (tile->IsEmpty())
                {
                    continue;
                }

                // Given geometry create Recast tile structure.
                NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                    m_meshConfig, m_context.get());

                {
                    NavMeshQuery::LockGuard lock(*m_navObject);
                    // If a tile at the location already exists, remove it before update the data.
                    if (const dtTileRef tileRef = lock.GetNavMesh()->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
                    {
                        lock.GetNavMesh()->removeTile(tileRef, nullptr, nullptr);
                    }
                }

                // A tile might have no geometry at all if no objects were found there.
                if (navigationTileData.IsValid())
                {
                    AttachNavigationTileToMesh(navigationTileData);
                }
            }
        }

        RecastNavigationMeshNotificationBus::Event(GetEntityId(), &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, GetEntityId());
    }

    void RecastNavigationMeshComponent::UpdateNavigationMeshAsync()
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: UpdateNavigationMeshAsync");

        RecastNavigationProviderRequestBus::Event(GetEntityId(),
            &RecastNavigationProviderRequests::CollectGeometryAsync,
            m_meshConfig.m_tileSize, aznumeric_cast<float>(m_meshConfig.m_borderSize) * m_meshConfig.m_cellSize,
            [this](AZStd::shared_ptr<TileGeometry> tile)
            {
                OnTileProcessedEvent(tile);
            });
    }

    AZStd::shared_ptr<NavMeshQuery> RecastNavigationMeshComponent::GetNavigationObject()
    {
        return m_navObject;
    }

    void RecastNavigationMeshComponent::Activate()
    {
        RecastNavigationMeshCommon::OnActivate();
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
        RecastNavigationMeshCommon::OnDeactivate();
        m_tickEvent.RemoveFromQueue();

        m_context = {};
        m_navObject = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnSendNotificationTick()
    {
        RecastNavigationMeshNotificationBus::Event(GetEntityId(),
            &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, GetEntityId());
    }
    void RecastNavigationMeshComponent::OnDebugDrawTick()
    {
        NavMeshQuery::LockGuard lock(*m_navObject);

        if (lock.GetNavMesh() && (cl_navmesh_debug || m_showNavigationMesh))
        {
            AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
            Camera::ActiveCameraRequestBus::BroadcastResult(cameraTransform, &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);
            m_customDebugDraw.SetViewableAabb(AZ::Aabb::CreateCenterRadius(cameraTransform.GetTranslation(), cl_navmesh_debugRadius));

            duDebugDrawNavMesh(&m_customDebugDraw, *lock.GetNavMesh(), DU_DRAWNAVMESH_COLOR_TILES);
        }
    }

    void RecastNavigationMeshComponent::OnReceivedAllNewTiles()
    {
        ReceivedAllNewTilesImpl(m_meshConfig, m_sendNotificationEvent);        
    }

    void RecastNavigationMeshComponent::OnTileProcessedEvent(AZStd::shared_ptr<TileGeometry> tile)
    {
        if (tile)
        {
            // Store tile data until we received all of them.
            AZStd::lock_guard lock(m_tileProcessingMutex);
            m_tilesToBeProcessed.push_back(tile);
        }
        else
        {
            // The async operation to receive all tiled has finished. Kick off processing of received tiles on the main thread.
            m_receivedAllNewTilesEvent.Enqueue(AZ::TimeMs{ 0 });
        }
    }
} // namespace RecastNavigation
