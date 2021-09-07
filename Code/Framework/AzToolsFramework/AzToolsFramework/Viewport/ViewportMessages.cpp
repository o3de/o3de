/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    float ManipulatorLineBoundWidth(const AzFramework::ViewportId viewportId /*= AzFramework::InvalidViewportId*/)
    {
        float lineBoundWidth = 0.0f;
        if (viewportId != AzFramework::InvalidViewportId)
        {
            ViewportInteraction::ViewportSettingsRequestBus::EventResult(
                lineBoundWidth, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorLineBoundWidth);
        }
        else
        {
            ViewportInteraction::ViewportSettingsRequestBus::BroadcastResult(
                lineBoundWidth, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorLineBoundWidth);
        }

        return lineBoundWidth;
    }

    float ManipulatorCicleBoundWidth(const AzFramework::ViewportId viewportId /*= AzFramework::InvalidViewportId*/)
    {
        float circleBoundWidth = 0.0f;
        if (viewportId != AzFramework::InvalidViewportId)
        {
            ViewportInteraction::ViewportSettingsRequestBus::EventResult(
                circleBoundWidth, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorCircleBoundWidth);
        }
        else
        {
            ViewportInteraction::ViewportSettingsRequestBus::BroadcastResult(
                circleBoundWidth, &ViewportInteraction::ViewportSettingsRequestBus::Events::ManipulatorCircleBoundWidth);
        }

        return circleBoundWidth;
    }
} // namespace AzToolsFramework
