/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DetourNavMeshBuilder.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Misc/RecastNavigationMeshCommon.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

AZ_DEFINE_BUDGET(Navigation);

AZ_CVAR(
    AZ::u32, bg_navmesh_threads, 2, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Number of threads to use to process tiles for each RecastNavigationMeshComponent");

namespace RecastNavigation
{
    NavigationTileData RecastNavigationMeshCommon::CreateNavigationTile(TileGeometry* geom,
        const RecastNavigationMeshConfig& meshConfig, rcContext* context)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: create tile");

        RecastProcessing recast;
        recast.vertices = geom->m_vertices.empty() ? nullptr : geom->m_vertices.front().m_xyz;
        recast.vertexCount = static_cast<int>(geom->m_vertices.size());
        recast.triangleData = geom->m_indices.empty() ? nullptr : &geom->m_indices[0];
        recast.triangleCount = static_cast<int>(geom->m_indices.size()) / 3;
        recast.context = context;
        
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

    void RecastNavigationMeshCommon::RecastProcessing::InitializeMeshConfig(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig)
    {
        // Init build configuration from GUI
        config = {};
        config.cs = meshConfig.m_cellSize;
        config.ch = meshConfig.m_cellHeight;
        config.walkableSlopeAngle = meshConfig.m_agentMaxSlope;
        config.walkableHeight = aznumeric_cast<int>(AZStd::ceil(meshConfig.m_agentHeight / config.ch));
        config.walkableClimb = aznumeric_cast<int>(AZStd::floor(meshConfig.m_agentMaxClimb / config.ch));
        config.walkableRadius = aznumeric_cast<int>(AZStd::ceil(meshConfig.m_agentRadius / config.cs));
        config.maxEdgeLen = aznumeric_cast<int>(meshConfig.m_edgeMaxLen / meshConfig.m_cellSize);
        config.maxSimplificationError = meshConfig.m_edgeMaxError;
        config.minRegionArea = rcSqr(meshConfig.m_regionMinSize);       // Note: area = size*size
        config.mergeRegionArea = rcSqr(meshConfig.m_regionMergeSize);   // Note: area = size*size
        config.maxVertsPerPoly = meshConfig.m_maxVerticesPerPoly;
        config.detailSampleDist = meshConfig.m_detailSampleDist < 0.9f ? 0 : meshConfig.m_cellSize * meshConfig.m_detailSampleDist;
        config.detailSampleMaxError = meshConfig.m_cellHeight * meshConfig.m_detailSampleMaxError;

        config.tileSize = aznumeric_cast<int>(meshConfig.m_tileSize / config.cs);
        config.borderSize = config.walkableRadius + meshConfig.m_borderSize; // Reserve enough padding.
        config.width = config.tileSize + config.borderSize * 2;
        config.height = config.tileSize + config.borderSize * 2;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin = RecastVector3::CreateFromVector3SwapYZ(geom->m_worldBounds.GetMin());
        const RecastVector3 worldMax = RecastVector3::CreateFromVector3SwapYZ(geom->m_worldBounds.GetMax());

        rcVcopy(config.bmin, worldMin.m_xyz);
        rcVcopy(config.bmax, worldMax.m_xyz);
        config.bmin[0] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmin[2] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[0] += aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[2] += aznumeric_cast<float>(config.borderSize) * config.cs;
    }

    bool RecastNavigationMeshCommon::RecastProcessing::RasterizeInputPolygonSoup()
    {
        // Allocate voxel height field where we rasterize our input data to.
        solid.reset(rcAllocHeightfield());
        if (!solid)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'solid'.");
            return false;
        }
        if (!rcCreateHeightfield(context, *solid, config.width, config.height,
            config.bmin, config.bmax, config.cs, config.ch))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create solid height field.");
            return false;
        }

        // Allocate array that can hold triangle area types.
        // If you have multiple meshes you need to process, allocate
        // and array which can hold the max number of triangles you need to process.
        trianglesAreas.resize(triangleCount, 0);

        // Find triangles which are walkable based on their slope and rasterize them.
        // If your input data is multiple meshes, you can transform them here, calculate
        // the are type for each of the meshes and rasterize them.
        rcMarkWalkableTriangles(context, config.walkableSlopeAngle, vertices,
            vertexCount, triangleData, triangleCount, trianglesAreas.data());
        if (!rcRasterizeTriangles(context, vertices, vertexCount, triangleData,
            trianglesAreas.data(), triangleCount, *solid))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not rasterize triangles.");
            return false;
        }

        trianglesAreas.clear();
        return true;
    }

    void RecastNavigationMeshCommon::RecastProcessing::FilterWalkableSurfaces(const RecastNavigationMeshConfig& meshConfig)
    {
        // Once all geometry is rasterized, we do initial pass of filtering to
        // remove unwanted overhangs caused by the conservative rasterization
        // as well as filter spans where the character cannot possibly stand.
        if (meshConfig.m_filterLowHangingObstacles)
        {
            rcFilterLowHangingWalkableObstacles(context, config.walkableClimb, *solid);
        }
        if (meshConfig.m_filterLedgeSpans)
        {
            rcFilterLedgeSpans(context, config.walkableHeight, config.walkableClimb, *solid);
        }
        if (meshConfig.m_filterWalkableLowHeightSpans)
        {
            rcFilterWalkableLowHeightSpans(context, config.walkableHeight, *solid);
        }
    }

    bool RecastNavigationMeshCommon::RecastProcessing::PartitionWalkableSurfaceToSimpleRegions()
    {
        // Compact the height field so that it is faster to handle from now on.
        // This will result more cache coherent data as well as the neighbors
        // between walkable cells will be calculated.
        compactHeightfield.reset(rcAllocCompactHeightfield());
        if (!compactHeightfield)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'chf'.");
            return false;
        }
        if (!rcBuildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, *solid, *compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build compact data.");
            return false;
        }

        solid.reset();

        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(context, config.walkableRadius, *compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not erode.");
            return false;
        }

        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distance field.
        if (!rcBuildRegionsMonotone(context, *compactHeightfield,
            config.borderSize, config.minRegionArea, config.mergeRegionArea))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build monotone regions.");
            return false;
        }

        return true;
    }

    bool RecastNavigationMeshCommon::RecastProcessing::TraceAndSimplifyRegionContours()
    {
        // Create contours.
        contourSet.reset(rcAllocContourSet());
        if (!contourSet)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating contours.");
            return false;
        }
        if (!rcBuildContours(context, *compactHeightfield, config.maxSimplificationError,
            config.maxEdgeLen, *contourSet))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create contours.");
            return false;
        }

        return true;
    }

    bool RecastNavigationMeshCommon::RecastProcessing::BuildPolygonsMeshFromContours()
    {
        // Build polygon nav mesh from the contours.
        polyMesh.reset(rcAllocPolyMesh());
        if (!polyMesh)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while creating poly mesh.");
            return false;
        }
        if (!rcBuildPolyMesh(context, *contourSet, config.maxVertsPerPoly, *polyMesh))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not triangulate contours.");
            return false;
        }

        return true;
    }

    bool RecastNavigationMeshCommon::RecastProcessing::CreateDetailMesh()
    {
        polyMeshDetail.reset(rcAllocPolyMeshDetail());
        if (!polyMeshDetail)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating detail mesh.");
            return false;
        }

        if (!rcBuildPolyMeshDetail(context, *polyMesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *polyMeshDetail))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build detail mesh.");
            return false;
        }

        compactHeightfield = nullptr;
        contourSet = nullptr;
        return true;
    }

    NavigationTileData RecastNavigationMeshCommon::RecastProcessing::CreateDetourData(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig)
    {
        if (config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
        {
            NavigationTileData navigationTileData;

            // Update poly flags from areas.
            for (int i = 0; i < polyMesh->npolys; ++i)
            {
                if (polyMesh->areas[i] == RC_WALKABLE_AREA)
                {
                    polyMesh->flags[i] = RC_WALKABLE_AREA;
                }
            }

            dtNavMeshCreateParams params = {};
            params.verts = polyMesh->verts;
            params.vertCount = polyMesh->nverts;
            params.polys = polyMesh->polys;
            params.polyAreas = polyMesh->areas;
            params.polyFlags = polyMesh->flags;
            params.polyCount = polyMesh->npolys;
            params.nvp = polyMesh->nvp;
            params.detailMeshes = polyMeshDetail->meshes;
            params.detailVerts = polyMeshDetail->verts;
            params.detailVertsCount = polyMeshDetail->nverts;
            params.detailTris = polyMeshDetail->tris;
            params.detailTriCount = polyMeshDetail->ntris;

            params.offMeshConVerts = nullptr;
            params.offMeshConRad = nullptr;
            params.offMeshConDir = nullptr;
            params.offMeshConAreas = nullptr;
            params.offMeshConFlags = nullptr;
            params.offMeshConUserID = nullptr;
            params.offMeshConCount = 0;

            params.walkableHeight = meshConfig.m_agentHeight;
            params.walkableRadius = meshConfig.m_agentRadius;
            params.walkableClimb = meshConfig.m_agentMaxClimb;

            rcVcopy(params.bmin, polyMesh->bmin);
            rcVcopy(params.bmax, polyMesh->bmax);

            params.cs = config.cs;
            params.ch = config.ch;
            params.buildBvTree = false;

            params.tileX = geom->m_tileX;
            params.tileY = geom->m_tileY;
            params.tileLayer = 0; // This can be used to provide vertical layers when navigation map has multiple levels.

            if (!dtCreateNavMeshData(&params, &navigationTileData.m_data, &navigationTileData.m_size))
            {
                // Empty tile
                return {};
            }

            return navigationTileData;
        }

        return {};
    }

    RecastNavigationMeshCommon::RecastNavigationMeshCommon()
        : m_taskExecutor(bg_navmesh_threads)
    {
    }

    void RecastNavigationMeshCommon::OnActivate()
    {
        m_shouldProcessTiles = true;
    }

    void RecastNavigationMeshCommon::OnDeactivate()
    {
        m_shouldProcessTiles = false;
        if (m_taskGraphEvent && m_taskGraphEvent->IsSignaled() == false)
        {
            // If the tasks are still in progress, wait until the task graph is finished.
            m_taskGraphEvent->Wait();
        }
    }

    bool RecastNavigationMeshCommon::CreateNavigationMesh(AZ::EntityId meshEntityId, float tileSize)
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

        RecastNavigationProviderRequestBus::EventResult(params.maxTiles, meshEntityId, &RecastNavigationProviderRequests::GetNumberOfTiles, tileSize);

        // in world units
        params.tileWidth = tileSize;
        params.tileHeight = tileSize;

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

        m_navObject = AZStd::make_shared<NavMeshQuery>(navMesh.release(), navQuery.release());

        return true;
    }

    bool RecastNavigationMeshCommon::AttachNavigationTileToMesh(NavigationTileData& navigationTileData)
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

    void RecastNavigationMeshCommon::ReceivedAllNewTilesImpl(const RecastNavigationMeshConfig& config, AZ::ScheduledEvent& sendNotificationEvent)
    {
        if (!m_taskGraphEvent || m_taskGraphEvent->IsSignaled())
        {
            AZ_PROFILE_SCOPE(Navigation, "Navigation: OnReceivedAllNewTiles");

            m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>();
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

                        if (navigationTileData.IsValid())
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: UpdateNavigationMeshAsync - tile callback");

                            {
                                NavMeshQuery::LockGuard lock(*m_navObject);
                                if (const dtTileRef tileRef = lock.GetNavMesh()->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
                                {
                                    lock.GetNavMesh()->removeTile(tileRef, nullptr, nullptr);
                                }
                            }
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
        }
    }
} // namespace RecastNavigation
