/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <Components/RecastHelpers.h>
#include <Components/RecastSmartPointer.h>

namespace RecastNavigation
{
    class RecastNavigationMeshConfig final
    {
    public:
        AZ_RTTI(RecastNavigationMeshConfig, "{BFFAD108-56ED-40D7-B44F-A44803968DA5}");

        static void Reflect(AZ::ReflectContext* context);

        rcConfig m_config = {};

        float m_cellSize = 0.3F;
        float m_cellHeight = 0.2F;

        float m_agentMaxSlope = 45;
        float m_agentHeight = 2.0F;
        float m_agentRadius = 0.75F;
        float m_agentMaxClimb = 0.9F;

        float m_edgeMaxError = 1.3F;
        float m_edgeMaxLen = 12.0F;

        float m_maxVertsPerPoly = 6;
        float m_detailSampleDist = 6.0F;
        float m_detailSampleMaxError = 1.0F;

        float m_regionMinSize = 8;
        float m_regionMergeSize = 20;

        RecastPointer<rcHeightfield> m_solid;

        AZStd::vector<AZ::u8> m_trianglesAreas;

        bool m_keepInterResults = false;

        bool m_filterLowHangingObstacles = true;
        bool m_filterLedgeSpans = true;
        bool m_filterWalkableLowHeightSpans = true;

        RecastPointer<rcCompactHeightfield> m_chf;

        RecastPointer<rcContourSet> m_contourSet;

        RecastPointer<rcPolyMesh> m_pmesh;

        RecastPointer<rcPolyMeshDetail> m_detailMesh;

        RecastPointer<dtNavMesh> m_navMesh;
        RecastPointer<dtNavMeshQuery> m_navQuery;
    };

} // namespace RecastNavigation
