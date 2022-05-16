/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationMeshCommon.h"

#include <DetourNavMeshBuilder.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_CVAR(
    bool, cl_navmesh_debug, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draw debug visual information about a navigation mesh");

AZ_DEFINE_BUDGET(Navigation);

namespace RecastNavigation
{
    NavigationTileData RecastNavigationMeshCommon::CreateNavigationTile(TileGeometry* geom,
        const RecastNavigationMeshConfig& meshConfig, rcContext* context)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: create tile");

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
        config.borderSize = config.walkableRadius + meshConfig.m_borderSize; // Reserve enough padding.
        config.width = config.tileSize + config.borderSize * 2;
        config.height = config.tileSize + config.borderSize * 2;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin(geom->m_worldBounds.GetMin());
        const RecastVector3 worldMax(geom->m_worldBounds.GetMax());

        rcVcopy(config.bmin, &worldMin.m_x);
        rcVcopy(config.bmax, &worldMax.m_x);
        config.bmin[0] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmin[2] -= aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[0] += aznumeric_cast<float>(config.borderSize) * config.cs;
        config.bmax[2] += aznumeric_cast<float>(config.borderSize) * config.cs;

        //
        // Step 2. Rasterize input polygon soup.
        //

        // Allocate voxel height field where we rasterize our input data to.
        solid.reset(rcAllocHeightfield());
        if (!solid)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'solid'.");
            return {};
        }
        if (!rcCreateHeightfield(context, *solid, config.width, config.height,
            config.bmin, config.bmax, config.cs, config.ch))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create solid height field.");
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
            AZ_Error("Navigation", false, "buildNavigation: Could not rasterize triangles.");
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
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'chf'.");
            return {};
        }
        if (!rcBuildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, *solid, *compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build compact data.");
            return {};
        }

        solid.reset();

        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(context, config.walkableRadius, *compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not erode.");
            return {};
        }

        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distance field.
        if (!rcBuildRegionsMonotone(context, *compactHeightfield,
            config.borderSize, config.minRegionArea, config.mergeRegionArea))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build monotone regions.");
            return {};
        }

        //
        // Step 5. Trace and simplify region contours.
        //

        // Create contours.
        contourSet.reset(rcAllocContourSet());
        if (!contourSet)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating contours.");
            return {};
        }
        if (!rcBuildContours(context, *compactHeightfield, config.maxSimplificationError,
            config.maxEdgeLen, *contourSet))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create contours.");
            return {};
        }

        //
        // Step 6. Build polygons mesh from contours.
        //

        // Build polygon nav mesh from the contours.
        polyMesh.reset(rcAllocPolyMesh());
        if (!polyMesh)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while creating poly mesh.");
            return {};
        }
        if (!rcBuildPolyMesh(context, *contourSet, config.maxVertsPerPoly, *polyMesh))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not triangulate contours.");
            return {};
        }

        //
        // Step 7. Create detail mesh which allows to access approximate height on each polygon.
        //

        polyMeshDetail.reset(rcAllocPolyMeshDetail());
        if (!polyMeshDetail)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating detail mesh.");
            return {};
        }

        if (!rcBuildPolyMeshDetail(context, *polyMesh, *compactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *polyMeshDetail))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build detail mesh.");
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

            params.cs = config.cs;
            params.ch = config.ch;
            params.buildBvTree = false;

            params.tileX = geom->m_tileX;
            params.tileY = geom->m_tileY;
            params.tileLayer = 0; // This can be used for providing vertical layers when navigation map has multiple levels.

            if (!dtCreateNavMeshData(&params, &navigationTileData.m_data, &navigationTileData.m_size))
            {
                // Empty tile
                return {};
            }

            return navigationTileData;
        }

        return {};
    }

    bool RecastNavigationMeshCommon::CreateNavigationMesh(AZ::EntityId meshEntityId, float tileSize)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: create mesh");

        m_navMesh.reset(dtAllocNavMesh());
        if (!m_navMesh)
        {
            AZ_Error("Navigation", false, "Could not create Detour navmesh");
            return false;
        }

        AZ::Aabb worldVolume = AZ::Aabb::CreateNull();

        dtNavMeshParams params = {};
        RecastNavigationSurveyorRequestBus::EventResult(worldVolume, meshEntityId, &RecastNavigationSurveyorRequests::GetWorldBounds);

        const RecastVector3 worldCenter(worldVolume.GetMin());
        rcVcopy(params.orig, &worldCenter.m_x);

        RecastNavigationSurveyorRequestBus::EventResult(params.maxTiles, meshEntityId, &RecastNavigationSurveyorRequests::GetNumberOfTiles, tileSize);

        // in world units
        params.tileWidth = tileSize;
        params.tileHeight = tileSize;

        dtStatus status = m_navMesh->init(&params);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh");
            return false;
        }

        m_navQuery.reset(dtAllocNavMeshQuery());

        status = m_navQuery->init(m_navMesh.get(), 2048);
        if (dtStatusFailed(status))
        {
            AZ_Error("Navigation", false, "Could not init Detour navmesh query");
            return false;
        }

        return true;
    }

    bool RecastNavigationMeshCommon::AttachNavigationTileToMesh(NavigationTileData& navigationTileData)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: addTile");

        dtTileRef tileRef = 0;
        const dtStatus status = m_navMesh->addTile(
            navigationTileData.m_data, navigationTileData.m_size,
            DT_TILE_FREE_DATA, 0, &tileRef);
        if (dtStatusFailed(status))
        {
            navigationTileData.Free();
            AZ_Error("Navigation", false, "Could not add navigation tile to the navmesh");
            return false;
        }

        return true;
    }
} // namespace RecastNavigation
