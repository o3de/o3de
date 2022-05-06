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

    void RecastNavigationMeshComponent::UpdateNavigationMesh()
    {
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        RecastNavigationSurveyorRequestBus::EventResult(tiles, GetEntityId(),
            &RecastNavigationSurveyorRequests::CollectGeometry,
            m_meshConfig.m_tileSize);

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

    NavigationTileData RecastNavigationMeshComponent::CreateNavigationTile(TileGeometry* geom,
        const RecastNavigationMeshConfig& meshConfig, rcContext* context)
    {
        rcConfig config = {};
        AZStd::vector<AZ::u8> trianglesAreas;
        RecastPointer<rcHeightfield> solid;
        RecastPointer<rcCompactHeightfield> compactHeightfield;
        RecastPointer<rcContourSet> contourSet;
        RecastPointer<rcPolyMesh> polyMesh;
        RecastPointer<rcPolyMeshDetail> polyMeshDetail;

        const float* vertices = &geom->m_vertices.front().m_x;
        const int vertexCount = static_cast<int>(geom->m_vertices.size());
        const int* triangleData = &geom->m_indices[0];
        const int triangleCount = static_cast<int>(geom->m_indices.size()) / 3;

        //
        // Step 1. Initialize build config.
        //

        // Init build configuration from GUI
        memset(&config, 0, sizeof(config));
        config.cs = meshConfig.m_cellSize;
        config.ch = meshConfig.m_cellHeight;
        config.walkableSlopeAngle = meshConfig.m_agentMaxSlope;
        config.walkableHeight = static_cast<int>(ceilf(meshConfig.m_agentHeight / config.ch));
        config.walkableClimb = static_cast<int>(floorf(meshConfig.m_agentMaxClimb / config.ch));
        config.walkableRadius = static_cast<int>(ceilf(meshConfig.m_agentRadius / config.cs));
        config.maxEdgeLen = static_cast<int>(meshConfig.m_edgeMaxLen / meshConfig.m_cellSize);
        config.maxSimplificationError = meshConfig.m_edgeMaxError;
        config.minRegionArea = rcSqr(meshConfig.m_regionMinSize);		// Note: area = size*size
        config.mergeRegionArea = rcSqr(meshConfig.m_regionMergeSize);	// Note: area = size*size
        config.maxVertsPerPoly = meshConfig.m_maxVerticesPerPoly;
        config.detailSampleDist = meshConfig.m_detailSampleDist < 0.9f ? 0 : meshConfig.m_cellSize * meshConfig.m_detailSampleDist;
        config.detailSampleMaxError = meshConfig.m_cellHeight * meshConfig.m_detailSampleMaxError;

        config.tileSize = static_cast<int>(meshConfig.m_tileSize / config.cs);
        config.borderSize = config.walkableRadius + 3; // Reserve enough padding.
        config.width = config.tileSize + config.borderSize * 2;
        config.height = config.tileSize + config.borderSize * 2;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin(geom->m_worldBounds.GetMin());
        const RecastVector3 worldMax(geom->m_worldBounds.GetMax());

        [[maybe_unused]] const auto extents = geom->m_worldBounds.GetExtents();

        rcVcopy(config.bmin, &worldMin.m_x);
        rcVcopy(config.bmax, &worldMax.m_x);
        config.bmin[0] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmin[2] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[0] += aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[2] += aznumeric_cast<float>(config.borderSize) * config.cs;

        context->log(RC_LOG_PROGRESS, "Building navigation:");
        context->log(RC_LOG_PROGRESS, " - %d x %d cells", config.width, config.height);
        context->log(RC_LOG_PROGRESS, " - %d vertices, %d triangles", vertexCount, triangleCount);

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
            trianglesAreas.data(), triangleCount, *solid))
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
        if (!rcBuildRegionsMonotone(context, *compactHeightfield,
            config.borderSize, config.minRegionArea, config.mergeRegionArea))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
            return {};
        }

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

        //
        // Step 6. Build polygons mesh from contours.
        //

        // Build polygon nav mesh from the contours.
        polyMesh.reset(rcAllocPolyMesh());
        if (!polyMesh)
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Out of memory while creating poly mesh.");
            return {};
        }
        if (!rcBuildPolyMesh(context, *contourSet, config.maxVertsPerPoly, *polyMesh))
        {
            context->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
            return {};
        }

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

        compactHeightfield = nullptr;
        contourSet = nullptr;

        //
        // Step 8. Create Detour data from Recast poly mesh.
        //

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

            AZ_Printf("TEST", "rcConfig              bmin %f,%f,%f; bmax %f,%f,%f;\n"
                "dtNavMeshCreateParams bmin %f,%f,%f; bmax %f,%f,%f",
                config.bmin[0], config.bmin[1], config.bmin[2],
                config.bmax[0], config.bmax[1], config.bmax[2],
                params.bmin[0], params.bmin[1], params.bmin[2],
                params.bmax[0], params.bmax[1], params.bmax[2]);

            params.cs = config.cs;
            params.ch = config.ch;
            params.buildBvTree = false;

            params.tileX = geom->m_tileX;
            params.tileY = geom->m_tileY;
            params.tileLayer = 0; // This can be used for providing vertical layers when navigation map has multiple levels.

            if (!dtCreateNavMeshData(&params, &navigationTileData.m_data, &navigationTileData.m_size))
            {
                context->log(RC_LOG_ERROR, "Could not build navigation tile.");
                return {};
            }

            return navigationTileData;
        }

        return {};
    }

    bool RecastNavigationMeshComponent::CreateNavigationMesh()
    {
        m_navMesh.reset(dtAllocNavMesh());
        if (!m_navMesh)
        {
            AZ_Error("Navigation", false, "Could not create Detour navmesh");
            return false;
        }

        AZ::Aabb worldVolume = AZ::Aabb::CreateNull();

        dtNavMeshParams params = {};
        RecastNavigationSurveyorRequestBus::EventResult(worldVolume, GetEntityId(), &RecastNavigationSurveyorRequests::GetWorldBounds);

        const RecastVector3 worldCenter(worldVolume.GetMin());
        rcVcopy(params.orig, &worldCenter.m_x);

        RecastNavigationSurveyorRequestBus::EventResult(params.maxTiles, GetEntityId(), &RecastNavigationSurveyorRequests::GetNumberOfTiles,
            m_meshConfig.m_tileSize);

        // in world units
        params.tileWidth = m_meshConfig.m_tileSize;
        params.tileHeight = m_meshConfig.m_tileSize;

        dtStatus status = m_navMesh->init(&params);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh");
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

    bool RecastNavigationMeshComponent::AttachNavigationTileToMesh(NavigationTileData& navigationTileData)
    {
        dtTileRef tileRef = 0;
        dtStatus status = m_navMesh->addTile(
            navigationTileData.m_data, navigationTileData.m_size,
            DT_TILE_FREE_DATA, 0, &tileRef);
        if (dtStatusFailed(status))
        {
            navigationTileData.Free();
            AZ_Error("Navigation", false, "Could not add navigation tile to the navmesh");
            return false;
        }

        //if (!m_navQuery) // query object is re-usable
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
        if (!m_navMesh || !m_navQuery)
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;
        RecastVector3 startRecast{ fromWorldPosition }, endRecast{ targetWorldPosition };
        constexpr float halfExtents[3] = { 3.F, 3., 3 };

        dtPolyRef startPoly = 0, endPoly = 0;

        RecastVector3 nearestStartPoint, nearestEndPoint;

        const dtQueryFilter filter;

        dtStatus result = m_navQuery->findNearestPoly(startRecast.GetData(), halfExtents, &filter, &startPoly, nearestStartPoint.GetData());
        if (dtStatusFailed(result))
        {
            return {};
        }

        result = m_navQuery->findNearestPoly(endRecast.GetData(), halfExtents, &filter, &endPoly, nearestEndPoint.GetData());
        if (dtStatusFailed(result))
        {
            return {};
        }

        constexpr int maxPathLength = 100;
        dtPolyRef path[maxPathLength] = {};
        int pathLength = 0;

        // find an approximate path
        result = m_navQuery->findPath(startPoly, endPoly, nearestStartPoint.GetData(), nearestEndPoint.GetData(), &filter, path, &pathLength, maxPathLength);
        if (dtStatusFailed(result))
        {
            return {};
        }

        constexpr int maxDetailedPathLength = 100;
        RecastVector3 detailedPath[maxDetailedPathLength] = {};
        AZ::u8 detailedPathFlags[maxDetailedPathLength] = {};
        dtPolyRef detailedPolyPathRefs[maxDetailedPathLength] = {};
        int detailedPathCount = 0;

        result = m_navQuery->findStraightPath(startRecast.GetData(), endRecast.GetData(), path, pathLength, detailedPath[0].GetData(), detailedPathFlags, detailedPolyPathRefs,
            &detailedPathCount, maxDetailedPathLength, DT_STRAIGHTPATH_ALL_CROSSINGS);
        if (dtStatusFailed(result))
        {
            return {};
        }

        for (int i = 0; i < detailedPathCount; ++i)
        {
            pathPoints.push_back(detailedPath[i].AsVector3());
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

        bool usingTiledSurveyor = false;
        RecastNavigationSurveyorRequestBus::EventResult(usingTiledSurveyor, GetEntityId(), &RecastNavigationSurveyorRequests::IsTiled);
        if (!usingTiledSurveyor)
        {
            // We are using a non-tiled surveyor. Force the tile to cover the entire area.
            AZ::Aabb entireVolume = AZ::Aabb::CreateNull();
            RecastNavigationSurveyorRequestBus::EventResult(entireVolume, GetEntityId(), &RecastNavigationSurveyorRequests::GetWorldBounds);
            m_meshConfig.m_tileSize = AZStd::max(entireVolume.GetExtents().GetX(), entireVolume.GetExtents().GetY());
        }

        CreateNavigationMesh();
    }

    void RecastNavigationMeshComponent::Deactivate()
    {
        m_context = {};
        m_navQuery = {};
        m_navMesh = {};

        RecastNavigationMeshRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void RecastNavigationMeshComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_navMesh)
        {
            return;
        }

        if (cl_navmesh_debug || m_showNavigationMesh)
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }
} // namespace RecastNavigation

#pragma optimize("", on)
