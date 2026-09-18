/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeTypes.h"

#include <AzCore/Debug/Profiler.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

AZ_DECLARE_BUDGET_SHARED(AzToolsFramework);
namespace WhiteBox
{
    void DrawEdges(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color,
        const AZStd::vector<EdgeBoundWithHandle>& edgeBoundsWithHandle, const Api::EdgeHandles& excludedEdgeHandles,
        AZStd::vector<AZ::Vector3>& lineBuffer)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        lineBuffer.clear(); // keeps the capacity, so a steady state costs no allocation
        lineBuffer.reserve(edgeBoundsWithHandle.size() * 2);
        for (const EdgeBoundWithHandle& edge : edgeBoundsWithHandle)
        {
            // if any of the edges in edgeBoundsWithHandle match
            // excludedEdgeHandles, simply continue and do not draw them
            if (AZStd::any_of(
                    excludedEdgeHandles.begin(), excludedEdgeHandles.end(),
                    [&edge](const Api::EdgeHandle edgeHandle)
                    {
                        return edgeHandle == edge.m_handle;
                    }))
            {
                continue;
            }

            lineBuffer.push_back(edge.m_bound.m_start);
            lineBuffer.push_back(edge.m_bound.m_end);
        }

        if (!lineBuffer.empty())
        {
            debugDisplay.SetColor(color);
            debugDisplay.DrawLines(lineBuffer, color);
        }
    }
} // namespace WhiteBox
