/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <DetourNavMeshBuilder.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about Navigation Mesh");

#pragma optimize("", off)

namespace RecastNavigation
{
    RecastNavigationMeshComponent::RecastNavigationMeshComponent()
        : m_geometryCollectedEventHandler([this](AZStd::shared_ptr<BoundedGeometry> boundedGeometry)
            {
                OnGeometryCollected(boundedGeometry);
            })
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

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RecastNavigationMeshComponent>("Recast Navigation Mesh", "[Calculates the walkable navigation mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &RecastNavigationMeshComponent::m_showNavigationMesh,
                        "Show NavMesh in Game", "if true, draws a helper overlay over the navigation mesh area")
                    ->DataElement(nullptr, &RecastNavigationMeshComponent::m_meshConfig, "Config", "Navigation Mesh configuration")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationMeshRequestBus>("RecastNavigationMeshRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Event("UpdateNavigationMesh", &RecastNavigationMeshRequests::UpdateNavigationMesh)
                ->Event("FindPathToEntity", &RecastNavigationMeshRequests::FindPathToEntity)
                ->Event("FindPathToPosition", &RecastNavigationMeshRequests::FindPathToPosition)
                ;

            behaviorContext->Class<RecastNavigationMeshComponent>()->RequestBus("RecastNavigationMeshRequestBus");

            behaviorContext->EBus<RecastNavigationMeshNotificationBus>("RecastNavigationMeshNotificationBus")
                ->Handler<RecastNavigationNotificationHandler>();
        }
    }

    void RecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void RecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    RecastVector3 RecastNavigationMeshComponent::GetPolyCenter(const dtNavMesh* navMesh, dtPolyRef ref)
    {
        RecastVector3 centerRecast;

        float center[3];
        center[0] = 0;
        center[1] = 0;
        center[2] = 0;

        const dtMeshTile* tile = nullptr;
        const dtPoly* poly = nullptr;
        const dtStatus status = navMesh->getTileAndPolyByRef(ref, &tile, &poly);
        if (dtStatusFailed(status))
        {
            return {};
        }

        if (poly->vertCount == 0)
        {
            return {};
        }

        for (int i = 0; i < aznumeric_cast<int>(poly->vertCount); ++i)
        {
            const float* v = &tile->verts[poly->verts[i] * 3];
            center[0] += v[0];
            center[1] += v[1];
            center[2] += v[2];
        }
        const float s = 1.0f / aznumeric_cast<float>(poly->vertCount);
        center[0] *= s;
        center[1] *= s;
        center[2] *= s;

        centerRecast.m_x = center[0];
        centerRecast.m_y = center[1];
        centerRecast.m_z = center[2];

        return centerRecast;
    }

    void RecastNavigationMeshComponent::UpdateNavigationMesh()
    {
        if (m_waitingOnNavMeshRebuild)
        {
            return;
        }

        m_taskGraphEvent = AZStd::unique_ptr<AZ::TaskGraphEvent>();
        m_navMeshReady = false;
        m_waitingOnNavMeshRebuild = true;

        RecastNavigationSurveyorRequestBus::Event(GetEntityId(), &RecastNavigationSurveyorRequests::StartCollectingGeometry);
    }

    NavigationTileData RecastNavigationMeshComponent::CreateNavigationTile(AZStd::shared_ptr<BoundedGeometry> geom,
        const RecastNavigationMeshConfig& meshConfig, rcContext* context)
    {
        rcConfig config = {};
        AZStd::vector<AZ::u8> trianglesAreas;
        RecastPointer<rcHeightfield> solid;
        RecastPointer<rcCompactHeightfield> compactHeightfield;
        RecastPointer<rcContourSet> contourSet;
        RecastPointer<rcPolyMesh> polyMesh;
        RecastPointer<rcPolyMeshDetail> polyMeshDetail;

        const float* vertices = &geom->m_verts.front().m_x;
        const int vertexCount = static_cast<int>(geom->m_verts.size());
        const int* triangleData = &geom->m_indices[0];
        const int triangleCount = static_cast<int>(geom->m_indices.size()) / 3;

        //
        // Step 1. Initialize build config.
        //

        // Init build configuration from GUI
        memset(&config, 0, sizeof(config));
        config.cs = meshConfig.m_cellSize;
        config.ch = meshConfig.m_cellHeight;
        config.tileSize = 32; // TODO
        config.borderSize = 0; // TODO
        config.width = config.tileSize + config.borderSize * 2;
        config.height = config.tileSize + config.borderSize * 2;
        config.walkableSlopeAngle = meshConfig.m_agentMaxSlope;
        config.walkableHeight = static_cast<int>(ceilf(meshConfig.m_agentHeight / config.ch));
        config.walkableClimb = static_cast<int>(floorf(meshConfig.m_agentMaxClimb / config.ch));
        config.walkableRadius = static_cast<int>(ceilf(meshConfig.m_agentRadius / config.cs));
        config.maxEdgeLen = static_cast<int>(meshConfig.m_edgeMaxLen / meshConfig.m_cellSize);
        config.maxSimplificationError = meshConfig.m_edgeMaxError;
        config.minRegionArea = static_cast<int>(rcSqr(meshConfig.m_regionMinSize));		// Note: area = size*size
        config.mergeRegionArea = static_cast<int>(rcSqr(meshConfig.m_regionMergeSize));	// Note: area = size*size
        config.maxVertsPerPoly = static_cast<int>(meshConfig.m_maxVertsPerPoly);
        config.detailSampleDist = meshConfig.m_detailSampleDist < 0.9f ? 0 : meshConfig.m_cellSize * meshConfig.m_detailSampleDist;
        config.detailSampleMaxError = meshConfig.m_cellHeight * meshConfig.m_detailSampleMaxError;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin(geom->m_worldBounds.GetMin());
        const RecastVector3 worldMax(geom->m_worldBounds.GetMax());

        rcVcopy(config.bmin, &worldMin.m_x);
        rcVcopy(config.bmax, &worldMax.m_x);
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        context->log(RC_LOG_PROGRESS, "Building navigation:");
        context->log(RC_LOG_PROGRESS, " - %d x %d cells", config.width, config.height);
        context->log(RC_LOG_PROGRESS, " - %d verts, %d triangles", vertexCount, triangleCount);

        //
        // Step 2. Rasterize input polygon soup.
        //

        // Allocate voxel height field where we rasterize our input data to.
        solid.reset(rcAllocHeightfield());
        if (!solid)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
            return {};
        }
        if (!rcCreateHeightfield(context, *solid, config.width, config.height,
            config.bmin, config.bmax, config.cs, config.ch))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not create solid height field.");
            return {};
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
            trianglesAreas.data(), triangleCount, *solid /*, m_config.walkableClimb*/))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
            return {};
        }

        trianglesAreas.clear();

        //
        // Step 3. Filter walkable surfaces.
        //

        // Once all geometry is rasterized, we do initial pass of filtering to
        // remove unwanted overhangs caused by the conservative rasterization
        // as well as filter spans where the character cannot possibly stand.
        if (meshConfig.m_filterLowHangingObstacles)
            rcFilterLowHangingWalkableObstacles(context, config.walkableClimb, *solid);
        if (meshConfig.m_filterLedgeSpans)
            rcFilterLedgeSpans(context, config.walkableHeight, config.walkableClimb, *solid);
        if (meshConfig.m_filterWalkableLowHeightSpans)
            rcFilterWalkableLowHeightSpans(context, config.walkableHeight, *solid);

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(1.F, 0, 0, 0.1F));
        //    duDebugDrawHeightfieldSolid(&m_customDebugDraw, *m_solid);
        //}

        //
        // Step 4. Partition walkable surface to simple regions.
        //

        // Compact the height field so that it is faster to handle from now on.
        // This will result more cache coherent data as well as the neighbors
        // between walkable cells will be calculated.
        compactHeightfield.reset(rcAllocCompactHeightfield());
        if (!compactHeightfield)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
            return {};
        }
        if (!rcBuildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, *solid, *compactHeightfield))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
            return {};
        }

        solid.reset();

        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(context, config.walkableRadius, *compactHeightfield))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
            return {};
        }

        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distance field.
        if (!rcBuildRegionsMonotone(context, *compactHeightfield, 0, config.minRegionArea, config.mergeRegionArea))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
            return {};
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 1, 0, 1));
        //    duDebugDrawCompactHeightfieldSolid(&m_customDebugDraw, *m_chf);
        //}

        //
        // Step 5. Trace and simplify region contours.
        //

        // Create contours.
        contourSet.reset(rcAllocContourSet());
        if (!contourSet)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory while allocating contours.");
            return {};
        }
        if (!rcBuildContours(context, *compactHeightfield, config.maxSimplificationError,
            config.maxEdgeLen, *contourSet))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
            return {};
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 0, 1, 1));
        //    duDebugDrawContours(&m_customDebugDraw, *m_contourSet);
        //}

        //
        // Step 6. Build polygons mesh from contours.
        //

        // Build polygon navmesh from the contours.
        polyMesh.reset(rcAllocPolyMesh());
        if (!polyMesh)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
            return {};
        }
        if (!rcBuildPolyMesh(context, *contourSet, config.maxVertsPerPoly, *polyMesh))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
            return {};
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.F, 1, 1, 1));
        //    duDebugDrawPolyMesh(&m_customDebugDraw, *m_pmesh);
        //}

        //
        // Step 7. Create detail mesh which allows to access approximate height on each polygon.
        //

        polyMeshDetail.reset(rcAllocPolyMeshDetail());
        if (!polyMeshDetail)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory while allocating detail mesh.");
            return {};
        }

        if (!rcBuildPolyMeshDetail(context, *polyMesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *polyMeshDetail))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
            return {};
        }

        //if (cl_navmesh_debug)
        //{
        //    m_customDebugDraw.SetColor(AZ::Color(0.5F, 1, 1, 1));
        //    duDebugDrawPolyMeshDetail(&m_customDebugDraw, *m_detailMesh);
        //}

        compactHeightfield = nullptr;
        contourSet = nullptr;

        // At this point the navigation mesh data is ready, you can access it from m_pmesh.
        // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

        //
        // Step 8. Create Detour data from Recast poly mesh.
        //

        // The GUI may allow more max points per polygon than Detour can handle.
        // Only build the detour navmesh if we do not exceed the limit.
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
            params.buildBvTree = true;
            
		    params.tileX = 0;
		    params.tileY = 0;
		    params.tileLayer = 0;

            if (!dtCreateNavMeshData(&params, &navigationTileData.m_data, &navigationTileData.m_size))
            {
                context->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
                return {};
            }

            return navigationTileData;
        }

        return {};
    }

    void RecastNavigationMeshComponent::OnGeometryCollected(AZStd::shared_ptr<BoundedGeometry> boundedGeometry)
    {
        m_taskGraph.Reset();
        m_taskGraph.AddTask(
            m_taskDescriptor,
            [this, boundedGeometry]
            {
                if (TaskUpdateNavigationMesh(boundedGeometry))
                {
                    m_navMeshReady = true;
                    RecastNavigationMeshNotificationBus::Broadcast(&RecastNavigationMeshNotificationBus::Events::OnNavigationMeshUpdated, GetEntityId());
                }

                m_waitingOnNavMeshRebuild = false;
            });
        m_taskGraph.Submit(m_taskGraphEvent.get());
    }

    bool RecastNavigationMeshComponent::TaskUpdateNavigationMesh(AZStd::shared_ptr<BoundedGeometry> boundedGeometry)
    {
        if (boundedGeometry->IsEmpty())
        {
            return true;
        }

        NavigationTileData navigationTileData = CreateNavigationTile(boundedGeometry, m_meshConfig, m_context.get());
        if (navigationTileData.IsValid() == false)
        {
            return false;
        }

        return CreateNavigationMesh(navigationTileData);
    }

    bool RecastNavigationMeshComponent::CreateNavigationMesh(NavigationTileData& navigationTileData)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_navMeshMutex);

        m_navMesh.reset(dtAllocNavMesh());
        if (!m_navMesh)
        {
            navigationTileData.Free();
            AZ_Error("Navigation", false, "Could not create Detour navmesh");
            return false;
        }

        dtNavMeshParams params = {};
        AZ::Aabb volume = AZ::Aabb::CreateNull();
        RecastNavigationSurveyorRequestBus::EventResult(volume, GetEntityId(), &RecastNavigationSurveyorRequests::GetWorldBounds);

        const RecastVector3 worldCenter(volume.GetMin());
        rcVcopy(params.orig, &worldCenter.m_x);

        int tilesOnX = 1;
        RecastNavigationSurveyorRequestBus::EventResult(tilesOnX, GetEntityId(), &RecastNavigationSurveyorRequests::GetTilesAlongXDimension);
        int tilesOnY = 1;
        RecastNavigationSurveyorRequestBus::EventResult(tilesOnY, GetEntityId(), &RecastNavigationSurveyorRequests::GetTilesAlongYDimension);
        params.tileWidth = volume.GetXExtent() / tilesOnX;
        params.tileHeight = volume.GetYExtent() / tilesOnY;
        params.maxTiles = tilesOnX * tilesOnY;

        //params.maxPolys = 10'000; // TODO what this should be?

        dtStatus status = m_navMesh->init(&params);
        if (dtStatusFailed(status))
        {
            navigationTileData.Free();
            AZ_Error("Navigation", false, "Could not init Detour navmesh");
            return false;
        }

        dtTileRef tileRef = 0;
        status = m_navMesh->addTile(navigationTileData.m_data, navigationTileData.m_size,
            DT_TILE_FREE_DATA, 0, &tileRef);
        if (dtStatusFailed(status))
        {
            navigationTileData.Free();
            AZ_Error("Navigation", false, "Could not add navigation tile to navmesh");
            return false;
        }

        if (!m_navQuery) // query object is re-usable
        {
            m_navQuery.reset(dtAllocNavMeshQuery());
        }

        status = m_navQuery->init(m_navMesh.get(), 2048);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh query");
            return false;
        }

        return true;
    }

    AZStd::vector<AZ::Vector3> RecastNavigationMeshComponent::FindPathToEntity(AZ::EntityId fromEntity, AZ::EntityId toEntity)
    {
        if (fromEntity.IsValid() && toEntity.IsValid())
        {
            AZ::Vector3 start = AZ::Vector3::CreateZero(), end = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(start, fromEntity, &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::TransformBus::EventResult(end, toEntity, &AZ::TransformBus::Events::GetWorldTranslation);

            return FindPathToPosition(start, end);
        }

        return {};
    }

    AZStd::vector<AZ::Vector3> RecastNavigationMeshComponent::FindPathToPosition(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_navMeshMutex);

        if (m_navMeshReady == false)
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;

        {
            RecastVector3 startRecast{ fromWorldPosition }, endRecast{ targetWorldPosition };
            constexpr float halfExtents[3] = { 3.F, 3., 3 };

            dtPolyRef startPoly = 0, endPoly = 0;

            RecastVector3 nearestStartPoint, nearestEndPoint;

            const dtQueryFilter filter;

            dtStatus result = m_navQuery->findNearestPoly(startRecast.data(), halfExtents, &filter, &startPoly, nearestStartPoint.data());
            if (dtStatusFailed(result))
            {
                return {};
            }

            result = m_navQuery->findNearestPoly(endRecast.data(), halfExtents, &filter, &endPoly, nearestEndPoint.data());
            if (dtStatusFailed(result))
            {
                return {};
            }

            constexpr int maxPathLength = 100;
            dtPolyRef path[maxPathLength] = {};
            int pathLength = 0;

            // find an approximate path
            result = m_navQuery->findPath(startPoly, endPoly, nearestStartPoint.data(), nearestEndPoint.data(), &filter, path, &pathLength, maxPathLength);
            if (result != DT_SUCCESS)
            {
                return {};
            }

            //if (cl_navmesh_debug)
            //{
            //    AZ_Printf("NavMesh", "from %d to %d, findPath = %d", startPoly, endPoly, pathLength);
            //}

            AZStd::vector<RecastVector3> approximatePath;
            approximatePath.resize(pathLength);

            for (int pathIndex = 0; pathIndex < pathLength; ++pathIndex)
            {
                RecastVector3 center = GetPolyCenter(m_navMesh.get(), path[pathIndex]);
                approximatePath.push_back(center);

                //if (cl_navmesh_debug)
                //{
                //    AZ_Printf("NavMesh", "path %d = %s", pathIndex, AZ::ToString(center.AsVector3()).c_str());
                //}
            }

            constexpr int maxDetailedPathLength = 100;
            RecastVector3 detailedPath[maxDetailedPathLength] = {};
            AZ::u8 detailedPathFlags[maxDetailedPathLength] = {};
            dtPolyRef detailedPolyPathRefs[maxDetailedPathLength] = {};
            int detailedPathCount = 0;

            result = m_navQuery->findStraightPath(startRecast.data(), endRecast.data(), path, pathLength, detailedPath[0].data(), detailedPathFlags, detailedPolyPathRefs,
                &detailedPathCount, maxDetailedPathLength, DT_STRAIGHTPATH_ALL_CROSSINGS);
            if (result != DT_SUCCESS)
            {
                return {};
            }

            if (cl_navmesh_debug)
            {
                //for (size_t pathIndex = 1; pathIndex < approximatePath.size(); ++pathIndex)
                //{
                //    DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequestBus::Events::DrawLineLocationToLocation,
                //        approximatePath[pathIndex - 1].AsVector3(), approximatePath[pathIndex].AsVector3(), AZ::Color(1.F, 0, 0, 1), 30.F);
                //}

                constexpr AZ::Crc32 ViewportId = AzFramework::g_defaultSceneEntityDebugDisplayId;
                AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
                AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, ViewportId);
                AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
                if (debugDisplay)
                {
                    for (size_t detailedIndex = 1; detailedIndex < approximatePath.size(); ++detailedIndex)
                    {
                        if (detailedPath[detailedIndex].AsVector3().IsZero())
                            break;

                        const AZ::Vector4 colorVector4 = AZ::Color(0.F, 1, 0, 1).GetAsVector4();

                        debugDisplay->DrawLine(
                            detailedPath[detailedIndex - 1].AsVector3(),
                            detailedPath[detailedIndex].AsVector3(), colorVector4, colorVector4);
                    }
                }
            }

            for (int i = 0; i < detailedPathCount; ++i)
            {
                pathPoints.push_back(detailedPath[i].AsVector3());
            }
        }

        return pathPoints;
    }

    void RecastNavigationMeshComponent::Activate()
    {
        m_context = AZStd::make_unique<RecastCustomContext>();

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationMeshRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();

        RecastNavigationSurveyorRequestBus::Event(GetEntityId(), &RecastNavigationSurveyorRequests::BindGeometryCollectionEventHandler,
            m_geometryCollectedEventHandler);
    }

    void RecastNavigationMeshComponent::Deactivate()
    {
        m_geometryCollectedEventHandler.Disconnect();

        if (m_taskGraphEvent && m_taskGraphEvent->IsSignaled() == false)
        {
            m_taskGraphEvent->Wait();
            m_taskGraphEvent.reset();
        }

        m_context = {};
        m_navQuery = {};
        m_navMesh = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_navMeshReady && (cl_navmesh_debug || m_showNavigationMesh))
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }
} // namespace RecastNavigation

#pragma optimize("", on)
