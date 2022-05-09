/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>

namespace RecastNavigation
{
    //! See @rcConfig from Recast library for documentation on these parameters.
    //! @Reflect method provides that documentation as tooltips in the Editor.
    class RecastNavigationMeshConfig final
    {
    public:
        AZ_RTTI(RecastNavigationMeshConfig, "{472141BB-7C36-4D2C-A508-78DB0D3C07F2}");

        static void Reflect(AZ::ReflectContext* context);

        float m_tileSize = 32.f;
        float m_cellSize = 0.3f;
        float m_cellHeight = 0.2f;

        float m_agentMaxSlope = 45.0f;
        float m_agentHeight = 2.0f;
        float m_agentRadius = 0.6f;
        float m_agentMaxClimb = 0.9f;

        float m_edgeMaxError = 1.3f;
        float m_edgeMaxLen = 12.0f;

        int m_maxVerticesPerPoly = 6;
        float m_detailSampleDist = 6.0f;
        float m_detailSampleMaxError = 1.0f;

        int m_regionMinSize = 8;
        int m_regionMergeSize = 20;

        bool m_filterLowHangingObstacles = true;
        bool m_filterLedgeSpans = true;
        bool m_filterWalkableLowHeightSpans = true;
    };

} // namespace RecastNavigation
