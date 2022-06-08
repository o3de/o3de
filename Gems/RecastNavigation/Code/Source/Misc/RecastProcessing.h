/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Recast.h>
#include <Misc/RecastHelpers.h>
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

        void InitializeMeshConfig(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig);

        [[nodiscard]] bool RasterizeInputPolygonSoup();

        void FilterWalkableSurfaces(const RecastNavigationMeshConfig& meshConfig);

        [[nodiscard]] bool PartitionWalkableSurfaceToSimpleRegions();

        [[nodiscard]] bool TraceAndSimplifyRegionContours();

        [[nodiscard]] bool BuildPolygonsMeshFromContours();

        [[nodiscard]] bool CreateDetailMesh();

        NavigationTileData CreateDetourData(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig);
    };
} // namespace RecastNavigation
