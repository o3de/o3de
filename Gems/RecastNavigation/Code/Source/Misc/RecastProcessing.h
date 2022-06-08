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
        rcConfig config = {};
        AZStd::vector<AZ::u8> trianglesAreas;
        RecastPointer<rcHeightfield> solid;
        RecastPointer<rcCompactHeightfield> compactHeightfield;
        RecastPointer<rcContourSet> contourSet;
        RecastPointer<rcPolyMesh> polyMesh;
        RecastPointer<rcPolyMeshDetail> polyMeshDetail;

        const float* vertices = nullptr;
        int vertexCount = 0;
        const int* triangleData = nullptr;
        int triangleCount = 0;

        rcContext* context = nullptr;

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
