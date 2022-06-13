/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Recast.h>
#include <RecastNavigation/RecastHelpers.h>
#include <Misc/RecastNavigationDebugDraw.h>
#include <Misc/RecastNavigationMeshConfig.h>

namespace RecastNavigation
{
    struct RecastProcessing
    {
        rcConfig m_config = {};
        AZStd::vector<AZ::u8> m_trianglesAreas;
        RecastPointer<rcHeightfield> m_solid;
        RecastPointer<rcCompactHeightfield> m_compactHeightfield;
        RecastPointer<rcContourSet> m_contourSet;
        RecastPointer<rcPolyMesh> m_polyMesh;
        RecastPointer<rcPolyMeshDetail> m_polyMeshDetail;

        const float* m_vertices = nullptr;
        int m_vertexCount = 0;
        const int* m_triangleData = nullptr;
        int m_triangleCount = 0;

        rcContext* m_context = nullptr;

        //! First step in building a navigation mesh.
        //! @param geom input geometry
        //! @param meshConfig navigation mesh configuration
        void InitializeMeshConfig(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig);

        //! Second step in building a navigation mesh.
        //! @return true if successful
        [[nodiscard]] bool RasterizeInputPolygonSoup();

        //! Thirst step in building a navigation mesh.
        //! Once all geometry is rasterized, we do initial pass of filtering to
        //! remove unwanted overhangs caused by the conservative rasterization
        //! as well as filter spans where the character cannot possibly stand.
        //! @param meshConfig navigation mesh configuration, must be the same as provided in @InitializeMeshConfig
        void FilterWalkableSurfaces(const RecastNavigationMeshConfig& meshConfig);

        //! Fourth step in building a navigation mesh.
        //! Compact the height field so that it is faster to handle from now on.
        //! This will result more cache coherent data as well as the neighbors
        //! between walkable cells will be calculated.
        //! @return true if successful
        [[nodiscard]] bool PartitionWalkableSurfaceToSimpleRegions();

        //! Fifth step in building a navigation mesh.
        //! @return true if successful
        [[nodiscard]] bool TraceAndSimplifyRegionContours();

        //! Sixth step in building a navigation mesh.
        //! @return true if successful
        [[nodiscard]] bool BuildPolygonsMeshFromContours();

        //! Seventh step in building a navigation mesh.
        //! @return true if successful
        [[nodiscard]] bool CreateDetailMesh();

        //! Eight and last step in building a navigation mesh.
        //! @param geom input geometry, must be the same as provided in @InitializeMeshConfig
        //! @param meshConfig navigation mesh configuration
        //! @return navigation tile data in Recast-binary format
        NavigationTileData CreateDetourData(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig);
    };
} // namespace RecastNavigation
