/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RecastNavigationDebugDraw.h"
#include "RecastNavigationMeshConfig.h"

#include <DetourNavMesh.h>
#include <Recast.h>
#include <AzCore/Component/EntityId.h>
#include <Components/RecastHelpers.h>

namespace RecastNavigation
{
    //! Common navigation mesh logic.
    class RecastNavigationMeshCommon
    {
    public:
        AZ_RTTI(RecastNavigationMeshCommon, "{D34CD5E0-8C29-4545-8734-9C7A92F03740}");
        virtual ~RecastNavigationMeshCommon() = default;
        
        bool CreateNavigationMesh(AZ::EntityId meshEntityId, float tileSize);
        bool AttachNavigationTileToMesh(NavigationTileData& navigationTileData);
        NavigationTileData CreateNavigationTile(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig, rcContext* context);
        
        RecastNavigationDebugDraw m_customDebugDraw;
        
        AZStd::unique_ptr<rcContext> m_context;
        RecastPointer<dtNavMesh> m_navMesh;
        RecastPointer<dtNavMeshQuery> m_navQuery;
    };

} // namespace RecastNavigation
