/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "EditorWhiteBoxComponentModeTypes.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace WhiteBox
{
    void DrawEdges(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color,
        const AZStd::vector<EdgeBoundWithHandle>& edgeBoundsWithHandle, const Api::EdgeHandles& excludedEdgeHandles)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        debugDisplay.SetColor(color);
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

            debugDisplay.DrawLine(edge.m_bound.m_start, edge.m_bound.m_end);
        }
    }
} // namespace WhiteBox
