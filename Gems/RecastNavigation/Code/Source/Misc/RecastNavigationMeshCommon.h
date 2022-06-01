/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Recast.h>
#include <AzCore/Component/EntityId.h>
#include <Misc/RecastHelpers.h>
#include <Misc/RecastNavigationDebugDraw.h>
#include <Misc/RecastNavigationMeshConfig.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Common navigation mesh logic for Recast navigation components. Recommended use is as a base class.
    //! The method provided are not thread-safe. Use the mutex from @m_navObject to synchronize as necessary at the higher level.
    class RecastNavigationMeshCommon
    {
    public:
        AZ_RTTI(RecastNavigationMeshCommon, "{D34CD5E0-8C29-4545-8734-9C7A92F03740}");
        virtual ~RecastNavigationMeshCommon() = default;

        //! Allocates and initializes Recast navigation mesh into @m_navMesh.
        //! @param meshEntityId the entity's positions will be used as the center of the navigation mesh.
        //! @param tileSize the size of each square tile that form the navigation mesh. Recommended values are power of 2.
        //! @return true if the navigation mesh object was successfully created.
        bool CreateNavigationMesh(AZ::EntityId meshEntityId, float tileSize);

        //! Given a Recast data add a tile to the navigation mesh @m_navMesh.
        //! @param navigationTileData the raw data of a Recast tile
        //! @return true if successful.
        bool AttachNavigationTileToMesh(NavigationTileData& navigationTileData);

        //! Given a set of geometry and configuration create a Recast tile that can be attached using @AttachNavigationTileToMesh.
        //! @param geom A set of geometry, triangle data.
        //! @param meshConfig Recast navigation mesh configuration.
        //! @param context Recast context object, @rcContext.
        //! @returns the tile data that can be attached to the navigation mesh using @AttachNavigationTileToMesh
        NavigationTileData CreateNavigationTile(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig, rcContext* context);

    protected:
        //! Debug draw object for Recast navigation mesh.
        RecastNavigationDebugDraw m_customDebugDraw;

        //! Recast logging functionality and other optional tools.
        AZStd::unique_ptr<rcContext> m_context;

        //! Recast navigation objects.
        AZStd::shared_ptr<NavMeshQuery> m_navObject;

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
    };

} // namespace RecastNavigation
