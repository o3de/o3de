/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastProcessing.h"

#include <DetourDebugDraw.h>
#include <DetourNavMeshBuilder.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Misc/RecastNavigationMeshComponentController.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

AZ_DEFINE_BUDGET(Navigation);

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");
AZ_CVAR(
    float, cl_navmesh_debugRadius, 25.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Limit debug draw to within a specified distance from the active camera");
AZ_CVAR(
    AZ::u32, bg_navmesh_threads, 2, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Number of threads to use to process tiles for each RecastNavigationMeshComponentController");

namespace RecastNavigation
{
    void RecastNavigationMeshComponentController::Reflect(AZ::ReflectContext* context)
    {
        RecastNavigationMeshConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RecastNavigationMeshComponentController>()
                ->Field("Configuration", &RecastNavigationMeshComponentController::m_configuration)
                ->Version(1)
                ;
        }
    }

    void RecastNavigationMeshComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // This can be satisfied by @RecastNavigationPhysXProviderComponent or a user-defined component.
        required.push_back(AZ_CRC_CE("RecastNavigationProviderService"));
    }

    bool RecastNavigationMeshComponentController::UpdateNavigationMeshBlockUntilCompleted()
    {
        bool notInProgress = false;
        if (!m_updateInProgress.compare_exchange_strong(notInProgress, true))
        {
            return false;
        }

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        // Blocking call.
        RecastNavigationProviderRequestBus::EventResult(tiles, m_entityComponentIdPair.GetEntityId(),
            &RecastNavigationProviderRequests::CollectGeometry,
            m_configuration.m_tileSize, aznumeric_cast<float>(m_configuration.m_borderSize) * m_configuration.m_cellSize);

        RecastNavigationMeshNotificationBus::Event(m_entityComponentIdPair.GetEntityId(),
            &RecastNavigationMeshNotificationBus::Events::OnNavigationMeshBeganRecalculating, m_entityComponentIdPair.GetEntityId());

        {
            for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
            {
                {
                    NavMeshQuery::LockGuard lock(*m_navObject);
                    // If a tile at the location already exists, remove it before updating the data.
                    if (const dtTileRef tileRef = lock.GetNavMesh()->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
                    {
                        lock.GetNavMesh()->removeTile(tileRef, nullptr, nullptr);
                    }
                }

                if (tile->IsEmpty())
                {
                    continue;
                }

                // Given geometry create Recast tile structure.
                NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                    m_configuration, m_context.get());

                // A tile might have no geometry at all if no objects were found there.
                if (navigationTileData.IsValid())
                {
                    AttachNavigationTileToMesh(navigationTileData);
                }
            }
        }

        RecastNavigationMeshNotificationBus::Event(m_entityComponentIdPair.GetEntityId(),
            &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, m_entityComponentIdPair.GetEntityId());
        m_updateInProgress = false;
        return true;
    }

    bool RecastNavigationMeshComponentController::UpdateNavigationMeshAsync()
    {
        bool notInProgress = false;
        if (m_updateInProgress.compare_exchange_strong(notInProgress, true))
        {
            AZ_PROFILE_SCOPE(Navigation, "Navigation: UpdateNavigationMeshAsync");

            bool operationScheduled = false;
            RecastNavigationProviderRequestBus::EventResult(operationScheduled, m_entityComponentIdPair.GetEntityId(),
                &RecastNavigationProviderRequests::CollectGeometryAsync,
                m_configuration.m_tileSize, aznumeric_cast<float>(m_configuration.m_borderSize) * m_configuration.m_cellSize,
                [this](AZStd::shared_ptr<TileGeometry> tile)
                {
                    OnTileProcessedEvent(tile);
                });

            if (!operationScheduled)
            {
                m_updateInProgress = false;
                return false;
            }
            return true;
        }

        return false;
    }

    AZStd::shared_ptr<NavMeshQuery> RecastNavigationMeshComponentController::GetNavigationObject()
    {
        return m_navObject;
    }

    void RecastNavigationMeshComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;
        m_context = AZStd::make_unique<rcContext>();

        // It is safe to create the navigation mesh object now.
        // The actual navigation data will be passed at a later time.
        CreateNavigationMesh(m_entityComponentIdPair.GetEntityId());

        if (IsDebugDrawEnabled())
        {
            m_tickEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        }

        RecastNavigationMeshRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
        m_shouldProcessTiles = true;
    }

    void RecastNavigationMeshComponentController::Deactivate()
    {
        m_tickEvent.RemoveFromQueue();

        if (m_updateInProgress)
        {
            m_shouldProcessTiles = false;
            if (m_taskGraphEvent && m_taskGraphEvent->IsSignaled() == false)
            {
                // If the tasks are still in progress, wait until the task graph is finished.
                m_taskGraphEvent->Wait();
            }
        }

        m_context.reset();
        m_navObject.reset();
        m_taskGraphEvent.reset();
        m_updateInProgress = false;

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponentController::SetConfiguration(const RecastNavigationMeshConfig& config)
    {
        m_configuration = config;
    }

    const RecastNavigationMeshConfig& RecastNavigationMeshComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void RecastNavigationMeshComponentController::OnSendNotificationTick()
    {
        if (m_updateInProgress)
        {
            RecastNavigationMeshNotificationBus::Event(m_entityComponentIdPair.GetEntityId(),
                &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, m_entityComponentIdPair.GetEntityId());
            m_updateInProgress = false;
        }
    }

    bool RecastNavigationMeshComponentController::IsDebugDrawEnabled() const
    {
        return cl_navmesh_debug || m_configuration.m_enableDebugDraw || m_configuration.m_enableEditorPreview;
    }

    void RecastNavigationMeshComponentController::OnDebugDrawTick()
    {
        if (IsDebugDrawEnabled())
        {
            NavMeshQuery::LockGuard lock(*m_navObject);

            if (lock.GetNavMesh())
            {
                AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
                Camera::ActiveCameraRequestBus::BroadcastResult(cameraTransform, &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);
                m_customDebugDraw.SetViewableAabb(AZ::Aabb::CreateCenterRadius(cameraTransform.GetTranslation(), cl_navmesh_debugRadius));

                duDebugDrawNavMesh(&m_customDebugDraw, *lock.GetNavMesh(), DU_DRAWNAVMESH_COLOR_TILES);
            }
        }
    }

    void RecastNavigationMeshComponentController::OnReceivedAllNewTiles()
    {
        ReceivedAllNewTilesImpl(m_configuration, m_sendNotificationEvent);
    }

    void RecastNavigationMeshComponentController::OnTileProcessedEvent(AZStd::shared_ptr<TileGeometry> tile)
    {
        if (tile)
        {
            if (m_shouldProcessTiles)
            {
                // Store tile data until we received all of them.
                AZStd::lock_guard lock(m_tileProcessingMutex);
                m_tilesToBeProcessed.push_back(tile);
            }
        }
        else
        {
            // The async operation to receive all tiled has finished. Kick off processing of received tiles on the main thread.
            m_receivedAllNewTilesEvent.Enqueue(AZ::TimeMs{ 0 });
        }
    }

    NavigationTileData RecastNavigationMeshComponentController::CreateNavigationTile(TileGeometry* geom,
        const RecastNavigationMeshConfig& meshConfig, rcContext* context)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: create tile");

        RecastProcessing recast;
        recast.m_vertices = geom->m_vertices.empty() ? nullptr : geom->m_vertices.front().m_xyz;
        recast.m_vertexCount = static_cast<int>(geom->m_vertices.size());
        recast.m_triangleData = geom->m_indices.empty() ? nullptr : &geom->m_indices[0];
        recast.m_triangleCount = static_cast<int>(geom->m_indices.size()) / 3;
        recast.m_context = context;

        // Step 1. Initialize build config.
        recast.InitializeMeshConfig(geom, meshConfig);

        // Step 2. Rasterize input polygon soup.
        if (!recast.RasterizeInputPolygonSoup())
        {
            return {};
        }

        // Step 3. Filter walkable surfaces.
        recast.FilterWalkableSurfaces(meshConfig);

        // Step 4. Partition walkable surface to simple regions.
        if (!recast.PartitionWalkableSurfaceToSimpleRegions())
        {
            return {};
        }

        // Step 5. Trace and simplify region contours.
        if (!recast.TraceAndSimplifyRegionContours())
        {
            return {};
        }

        // Step 6. Build polygons mesh from contours.
        if (!recast.BuildPolygonsMeshFromContours())
        {
            return {};
        }

        // Step 7. Create detail mesh which allows to access approximate height on each polygon.
        if (!recast.CreateDetailMesh())
        {
            return {};
        }

        // Step 8. Create Detour data from Recast poly mesh.
        return recast.CreateDetourData(geom, meshConfig);
    }

    RecastNavigationMeshComponentController::RecastNavigationMeshComponentController()
        : m_taskExecutor(bg_navmesh_threads)
    {
    }

    RecastNavigationMeshComponentController::RecastNavigationMeshComponentController(const RecastNavigationMeshConfig& config)
        : m_configuration(config)
        , m_taskExecutor(bg_navmesh_threads)
    {
    }

    bool RecastNavigationMeshComponentController::CreateNavigationMesh(AZ::EntityId meshEntityId)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: create mesh");

        RecastPointer<dtNavMesh> navMesh(dtAllocNavMesh());
        if (!navMesh)
        {
            AZ_Error("Navigation", false, "Could not create Detour navmesh");
            return false;
        }

        AZ::Aabb worldVolume = AZ::Aabb::CreateNull();

        dtNavMeshParams params = {};
        RecastNavigationProviderRequestBus::EventResult(worldVolume, meshEntityId, &RecastNavigationProviderRequests::GetWorldBounds);

        const RecastVector3 worldCenter = RecastVector3::CreateFromVector3SwapYZ(worldVolume.GetMin());
        rcVcopy(params.orig, worldCenter.m_xyz);

        RecastNavigationProviderRequestBus::EventResult(params.maxTiles, meshEntityId, &RecastNavigationProviderRequests::GetNumberOfTiles, m_configuration.m_tileSize);

        // in world units
        params.tileWidth = m_configuration.m_tileSize;
        params.tileHeight = m_configuration.m_tileSize;

        dtStatus status = navMesh->init(&params);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh");
            return false;
        }

        RecastPointer<dtNavMeshQuery> navQuery(dtAllocNavMeshQuery());

        status = navQuery->init(navMesh.get(), 2048);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh query");
            return false;
        }

        const AZStd::shared_ptr<NavMeshQuery> object = GetNavigationObject();
        if (object)
        {
            NavMeshQuery::LockGuard lock(*object);
            m_navObject = AZStd::make_shared<NavMeshQuery>(navMesh.release(), navQuery.release());
        }
        else
        {
            m_navObject = AZStd::make_shared<NavMeshQuery>(navMesh.release(), navQuery.release());
        }

        m_shouldProcessTiles = false;
        m_updateInProgress = false;

        return true;
    }

    bool RecastNavigationMeshComponentController::AttachNavigationTileToMesh(NavigationTileData& navigationTileData)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: addTile");

        NavMeshQuery::LockGuard lock(*m_navObject);

        dtTileRef tileRef = 0;
        const dtStatus status = lock.GetNavMesh()->addTile(
            navigationTileData.m_data, navigationTileData.m_size,
            DT_TILE_FREE_DATA, 0, &tileRef);
        if (dtStatusFailed(status))
        {
            dtFree(navigationTileData.m_data);
            return false;
        }

        return true;
    }

    void RecastNavigationMeshComponentController::ReceivedAllNewTilesImpl(const RecastNavigationMeshConfig& config, AZ::ScheduledEvent& sendNotificationEvent)
    {
        if (m_shouldProcessTiles && (!m_taskGraphEvent || m_taskGraphEvent->IsSignaled()))
        {
            AZ_PROFILE_SCOPE(Navigation, "Navigation: OnReceivedAllNewTiles");

            m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>("RecastNavigation Tile Processing Wait");
            m_taskGraph.Reset();

            AZStd::vector<AZ::TaskToken> tileTaskTokens;

            AZStd::vector<AZStd::shared_ptr<TileGeometry>> tilesToBeProcessed;
            {
                AZStd::lock_guard lock(m_tileProcessingMutex);
                m_tilesToBeProcessed.swap(tilesToBeProcessed);
            }

            // Create tasks for each tile and a finish task.
            for (AZStd::shared_ptr<TileGeometry> tile : tilesToBeProcessed)
            {
                AZ::TaskToken token = m_taskGraph.AddTask(
                    m_taskDescriptor, [this, tile, &config]()
                    {
                        if (!m_shouldProcessTiles)
                        {
                            return;
                        }

                        AZ_PROFILE_SCOPE(Navigation, "Navigation: task - computing tile");

                        NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                            config, m_context.get());

                        {
                            NavMeshQuery::LockGuard lock(*m_navObject);
                            if (const dtTileRef tileRef = lock.GetNavMesh()->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
                            {
                                lock.GetNavMesh()->removeTile(tileRef, nullptr, nullptr);
                            }
                        }

                        if (navigationTileData.IsValid())
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: UpdateNavigationMeshAsync - tile callback");

                            if (navigationTileData.IsValid())
                            {
                                AttachNavigationTileToMesh(navigationTileData);
                            }
                        }
                    });

                tileTaskTokens.push_back(AZStd::move(token));
            }

            AZ::TaskToken finishToken = m_taskGraph.AddTask(
                m_taskDescriptor, [&sendNotificationEvent]()
                {
                    sendNotificationEvent.Enqueue(AZ::TimeMs{ 0 });
                });

            for (AZ::TaskToken& task : tileTaskTokens)
            {
                task.Precedes(finishToken);
            }

            m_taskGraph.SubmitOnExecutor(m_taskExecutor, m_taskGraphEvent.get());

            RecastNavigationMeshNotificationBus::Event(m_entityComponentIdPair.GetEntityId(),
                &RecastNavigationMeshNotificationBus::Events::OnNavigationMeshBeganRecalculating, m_entityComponentIdPair.GetEntityId());
        }
    }
} // namespace RecastNavigation
