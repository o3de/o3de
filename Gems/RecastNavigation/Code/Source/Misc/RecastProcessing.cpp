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
#include <Misc/RecastProcessing.h>

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    void RecastProcessing::InitializeMeshConfig(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig)
    {
        // Init build configuration from GUI
        m_config = {};
        m_config.cs = meshConfig.m_cellSize;
        m_config.ch = meshConfig.m_cellHeight;
        m_config.walkableSlopeAngle = meshConfig.m_agentMaxSlope;
        m_config.walkableHeight = aznumeric_cast<int>(AZStd::ceil(meshConfig.m_agentHeight / m_config.ch));
        m_config.walkableClimb = aznumeric_cast<int>(AZStd::floor(meshConfig.m_agentMaxClimb / m_config.ch));
        m_config.walkableRadius = aznumeric_cast<int>(AZStd::ceil(meshConfig.m_agentRadius / m_config.cs));
        m_config.maxEdgeLen = aznumeric_cast<int>(meshConfig.m_edgeMaxLen / meshConfig.m_cellSize);
        m_config.maxSimplificationError = meshConfig.m_edgeMaxError;
        m_config.minRegionArea = rcSqr(meshConfig.m_regionMinSize);       // Note: area = size*size
        m_config.mergeRegionArea = rcSqr(meshConfig.m_regionMergeSize);   // Note: area = size*size
        m_config.maxVertsPerPoly = meshConfig.m_maxVerticesPerPoly;
        m_config.detailSampleDist = meshConfig.m_detailSampleDist < 0.9f ? 0 : meshConfig.m_cellSize * meshConfig.m_detailSampleDist;
        m_config.detailSampleMaxError = meshConfig.m_cellHeight * meshConfig.m_detailSampleMaxError;

        m_config.tileSize = aznumeric_cast<int>(meshConfig.m_tileSize / m_config.cs);
        m_config.borderSize = m_config.walkableRadius + meshConfig.m_borderSize; // Reserve enough padding.
        m_config.width = m_config.tileSize + m_config.borderSize * 2;
        m_config.height = m_config.tileSize + m_config.borderSize * 2;

        // Set the area where the navigation will be build.
        // Here the bounds of the input mesh are used, but the
        // area could be specified by an user defined box, etc.

        const RecastVector3 worldMin = RecastVector3::CreateFromVector3SwapYZ(geom->m_worldBounds.GetMin());
        const RecastVector3 worldMax = RecastVector3::CreateFromVector3SwapYZ(geom->m_worldBounds.GetMax());

        rcVcopy(m_config.bmin, worldMin.m_xyz);
        rcVcopy(m_config.bmax, worldMax.m_xyz);
        m_config.bmin[0] -= aznumeric_cast<float>(m_config.borderSize) * m_config.cs;
        m_config.bmin[2] -= aznumeric_cast<float>(m_config.borderSize) * m_config.cs;
        m_config.bmax[0] += aznumeric_cast<float>(m_config.borderSize) * m_config.cs;
        m_config.bmax[2] += aznumeric_cast<float>(m_config.borderSize) * m_config.cs;
    }

    bool RecastProcessing::RasterizeInputPolygonSoup()
    {
        // Allocate voxel height field where we rasterize our input data to.
        m_solid.reset(rcAllocHeightfield());
        if (!m_solid)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'solid'.");
            return false;
        }
        if (!rcCreateHeightfield(m_context, *m_solid, m_config.width, m_config.height,
            m_config.bmin, m_config.bmax, m_config.cs, m_config.ch))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create solid height field.");
            return false;
        }

        // Allocate array that can hold triangle area types.
        // If you have multiple meshes you need to process, allocate
        // and array which can hold the max number of triangles you need to process.
        m_trianglesAreas.resize(m_triangleCount, 0);

        // Find triangles which are walkable based on their slope and rasterize them.
        // If your input data is multiple meshes, you can transform them here, calculate
        // the are type for each of the meshes and rasterize them.
        rcMarkWalkableTriangles(m_context, m_config.walkableSlopeAngle, m_vertices,
            m_vertexCount, m_triangleData, m_triangleCount, m_trianglesAreas.data());
        if (!rcRasterizeTriangles(m_context, m_vertices, m_vertexCount, m_triangleData,
            m_trianglesAreas.data(), m_triangleCount, *m_solid))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not rasterize triangles.");
            return false;
        }

        m_trianglesAreas.clear();
        return true;
    }

    void RecastProcessing::FilterWalkableSurfaces(const RecastNavigationMeshConfig& meshConfig)
    {
        // Once all geometry is rasterized, we do initial pass of filtering to
        // remove unwanted overhangs caused by the conservative rasterization
        // as well as filter spans where the character cannot possibly stand.
        if (meshConfig.m_filterLowHangingObstacles)
        {
            rcFilterLowHangingWalkableObstacles(m_context, m_config.walkableClimb, *m_solid);
        }
        if (meshConfig.m_filterLedgeSpans)
        {
            rcFilterLedgeSpans(m_context, m_config.walkableHeight, m_config.walkableClimb, *m_solid);
        }
        if (meshConfig.m_filterWalkableLowHeightSpans)
        {
            rcFilterWalkableLowHeightSpans(m_context, m_config.walkableHeight, *m_solid);
        }
    }

    bool RecastProcessing::PartitionWalkableSurfaceToSimpleRegions()
    {
        // Compact the height field so that it is faster to handle from now on.
        // This will result more cache coherent data as well as the neighbors
        // between walkable cells will be calculated.
        m_compactHeightfield.reset(rcAllocCompactHeightfield());
        if (!m_compactHeightfield)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory 'chf'.");
            return false;
        }
        if (!rcBuildCompactHeightfield(m_context, m_config.walkableHeight, m_config.walkableClimb, *m_solid, *m_compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build compact data.");
            return false;
        }

        m_solid.reset();

        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(m_context, m_config.walkableRadius, *m_compactHeightfield))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not erode.");
            return false;
        }

        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distance field.
        if (!rcBuildRegionsMonotone(m_context, *m_compactHeightfield,
            m_config.borderSize, m_config.minRegionArea, m_config.mergeRegionArea))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build monotone regions.");
            return false;
        }

        return true;
    }

    bool RecastProcessing::TraceAndSimplifyRegionContours()
    {
        // Create contours.
        m_contourSet.reset(rcAllocContourSet());
        if (!m_contourSet)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating contours.");
            return false;
        }
        if (!rcBuildContours(m_context, *m_compactHeightfield, m_config.maxSimplificationError,
            m_config.maxEdgeLen, *m_contourSet))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not create contours.");
            return false;
        }

        return true;
    }

    bool RecastProcessing::BuildPolygonsMeshFromContours()
    {
        // Build polygon nav mesh from the contours.
        m_polyMesh.reset(rcAllocPolyMesh());
        if (!m_polyMesh)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while creating poly mesh.");
            return false;
        }
        if (!rcBuildPolyMesh(m_context, *m_contourSet, m_config.maxVertsPerPoly, *m_polyMesh))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not triangulate contours.");
            return false;
        }

        return true;
    }

    bool RecastProcessing::CreateDetailMesh()
    {
        m_polyMeshDetail.reset(rcAllocPolyMeshDetail());
        if (!m_polyMeshDetail)
        {
            AZ_Error("Navigation", false, "buildNavigation: Out of memory while allocating detail mesh.");
            return false;
        }

        if (!rcBuildPolyMeshDetail(m_context, *m_polyMesh, *m_compactHeightfield, m_config.detailSampleDist, m_config.detailSampleMaxError, *m_polyMeshDetail))
        {
            AZ_Error("Navigation", false, "buildNavigation: Could not build detail mesh.");
            return false;
        }

        m_compactHeightfield = nullptr;
        m_contourSet = nullptr;
        return true;
    }

    NavigationTileData RecastProcessing::CreateDetourData(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig)
    {
        if (m_config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
        {
            NavigationTileData navigationTileData;

            // Update poly flags from areas.
            for (int i = 0; i < m_polyMesh->npolys; ++i)
            {
                if (m_polyMesh->areas[i] == RC_WALKABLE_AREA)
                {
                    m_polyMesh->flags[i] = RC_WALKABLE_AREA;
                }
            }

            dtNavMeshCreateParams params = {};
            params.verts = m_polyMesh->verts;
            params.vertCount = m_polyMesh->nverts;
            params.polys = m_polyMesh->polys;
            params.polyAreas = m_polyMesh->areas;
            params.polyFlags = m_polyMesh->flags;
            params.polyCount = m_polyMesh->npolys;
            params.nvp = m_polyMesh->nvp;
            params.detailMeshes = m_polyMeshDetail->meshes;
            params.detailVerts = m_polyMeshDetail->verts;
            params.detailVertsCount = m_polyMeshDetail->nverts;
            params.detailTris = m_polyMeshDetail->tris;
            params.detailTriCount = m_polyMeshDetail->ntris;

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

            rcVcopy(params.bmin, m_polyMesh->bmin);
            rcVcopy(params.bmax, m_polyMesh->bmax);

            params.cs = m_config.cs;
            params.ch = m_config.ch;
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
} // namespace RecastNavigation
