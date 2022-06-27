/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Misc/RecastNavigationMeshConfig.h>

namespace RecastNavigation
{
    void RecastNavigationMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            using Self = RecastNavigationMeshConfig;

            serialize->Class<RecastNavigationMeshConfig>()
                ->Field("Agent Height", &Self::m_agentHeight)
                ->Field("Agent Max Climb", &Self::m_agentMaxClimb)
                ->Field("Agent Max Slope", &Self::m_agentMaxSlope)
                ->Field("Agent Radius", &Self::m_agentRadius)
                ->Field("Voxel Height", &Self::m_cellHeight)
                ->Field("Voxel Size", &Self::m_cellSize)
                ->Field("Detail Sample Distance", &Self::m_detailSampleDist)
                ->Field("Detail Sample Max Error", &Self::m_detailSampleMaxError)
                ->Field("Edge Max Error", &Self::m_edgeMaxError)
                ->Field("Edge Max Length", &Self::m_edgeMaxLen)
                ->Field("Filter Ledge Spans", &Self::m_filterLedgeSpans)
                ->Field("Filter Low Hanging Obstacles", &Self::m_filterLowHangingObstacles)
                ->Field("Filter Walkable Low Height Spans", &Self::m_filterWalkableLowHeightSpans)
                ->Field("Max Vertices Per Poly", &Self::m_maxVerticesPerPoly)
                ->Field("Region Merge Size", &Self::m_regionMergeSize)
                ->Field("Region Min Size", &Self::m_regionMinSize)
                ->Field("Tile Size", &Self::m_tileSize)
                ->Field("Border Size", &Self::m_borderSize)
                ->Field("Debug Draw", &Self::m_enableDebugDraw)
                ->Field("Editor Preview", &Self::m_enableEditorPreview)
                ->Version(1)
                ;
        }
    }
} // namespace RecastNavigation
