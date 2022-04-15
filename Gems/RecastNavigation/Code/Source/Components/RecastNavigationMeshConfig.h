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
        AZ_RTTI(RecastNavigationMeshConfig, "{472141BB-7C36-4D2C-A508-78DB0D3C07F2}");

        static void Reflect(AZ::ReflectContext* context);

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

        bool m_filterLowHangingObstacles = true;
        bool m_filterLedgeSpans = true;
        bool m_filterWalkableLowHeightSpans = true;
    };

} // namespace RecastNavigation
