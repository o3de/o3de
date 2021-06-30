/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorDebug.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzToolsFramework
{
    void DrawAxis(
        AzFramework::DebugDisplayRequests& display, const AZ::Vector3& position, const AZ::Vector3& direction)
    {
        display.SetLineWidth(4.0f);
        display.SetColor(AZ::Color{ 1.0f, 1.0f, 0.0f, 1.0f });
        display.DrawLine(position, position + direction);
        display.SetLineWidth(1.0f);
    }

    void DrawTransformAxes(
        AzFramework::DebugDisplayRequests& display, const AZ::Transform& transform)
    {
        const float AxisLength = 0.5f;

        display.SetLineWidth(4.0f);
        display.SetColor(AZ::Color{ 1.0f, 0.0f, 0.0f, 1.0f });
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafe() * AxisLength);
        display.SetColor(AZ::Color{ 0.0f, 1.0f, 0.0f, 1.0f });
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafe() * AxisLength);
        display.SetColor(AZ::Color{ 0.0f, 0.0f, 1.0f, 1.0f });
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisZ().GetNormalizedSafe() * AxisLength);
        display.SetLineWidth(1.0f);
    }
} // namespace AzToolsFramework
